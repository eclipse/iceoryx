// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"

#include "test.hpp"

#include <chrono>
#include <stdlib.h>
#include <thread>

using namespace ::testing;
using namespace iox::popo;
using namespace iox::cxx;
using namespace iox::mepoo;
using namespace iox::posix;
using ::testing::Return;

struct DummySample
{
    uint64_t m_dummy{42};
};

static constexpr uint32_t NUM_CHUNKS_IN_POOL = 9 * iox::MAX_SUBSCRIBER_QUEUE_CAPACITY;
static constexpr uint32_t SMALL_CHUNK = 128;
static constexpr uint32_t CHUNK_META_INFO_SIZE = 256;
static constexpr size_t MEMORY_SIZE = NUM_CHUNKS_IN_POOL * (SMALL_CHUNK + CHUNK_META_INFO_SIZE);
alignas(64) static uint8_t g_memory[MEMORY_SIZE];
static constexpr uint32_t ITERATIONS = 10000;
static constexpr uint32_t MAX_NUMBER_QUEUES = 128;

struct ChunkDistributorConfig
{
    static constexpr uint32_t MAX_QUEUES = MAX_NUMBER_QUEUES;
    static constexpr uint64_t MAX_HISTORY_CAPACITY = iox::MAX_PUBLISHER_HISTORY;
};

struct ChunkQueueConfig
{
    static constexpr uint64_t MAX_QUEUE_CAPACITY = NUM_CHUNKS_IN_POOL / 3;
};

using ChunkQueueData_t = ChunkQueueData<ChunkQueueConfig, ThreadSafePolicy>;
using ChunkDistributorData_t =
    ChunkDistributorData<ChunkDistributorConfig, ThreadSafePolicy, ChunkQueuePusher<ChunkQueueData_t>>;
using ChunkDistributor_t = ChunkDistributor<ChunkDistributorData_t>;
using ChunkQueuePopper_t = ChunkQueuePopper<ChunkQueueData_t>;
using ChunkSenderData_t =
    ChunkSenderData<iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY, ChunkDistributorData_t>;
using ChunkReceiverData_t = ChunkReceiverData<iox::MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, ChunkQueueData_t>;

class ChunkBuildingBlocks_IntegrationTest : public Test
{
  public:
    ChunkBuildingBlocks_IntegrationTest()
    {
        m_mempoolConfig.addMemPool({SMALL_CHUNK, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolConfig, m_memoryAllocator, m_memoryAllocator);
    }
    virtual ~ChunkBuildingBlocks_IntegrationTest()
    {
        /// @note One chunk is on hold due to the fact that chunkSender and chunkDistributor hold last chunk
        EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
    }

    void SetUp()
    {
        ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());
        ASSERT_FALSE(m_chunkDistributor.tryAddQueue(&m_chunkReceiverData).has_error());
    }
    void TearDown(){};

    void publish()
    {
        for (size_t i = 0; i < ITERATIONS; i++)
        {
            m_chunkSender
                .tryAllocate(iox::UniquePortId(),
                             sizeof(DummySample),
                             iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT,
                             iox::CHUNK_NO_USER_HEADER_SIZE,
                             iox::CHUNK_NO_USER_HEADER_ALIGNMENT)
                .and_then([&](auto chunkHeader) {
                    auto sample = chunkHeader->userPayload();
                    new (sample) DummySample();
                    static_cast<DummySample*>(sample)->m_dummy = i;
                    m_chunkSender.send(chunkHeader);
                    m_sendCounter++;
                })
                .or_else([](AllocationError) {
                    // Errors shall never occur
                    FAIL();
                });

            /// Add some jitter to make thread breathe
            std::this_thread::sleep_for(std::chrono::microseconds(rand() % 100));
        }
        // Signal the next threads we're done
        m_publisherRun = false;
    }

    void forward()
    {
        uint64_t forwardCounter{0};
        bool finished{false};
        // this is to prevent a race condition on thread shutdown; there must be two consecutive empty pops after the
        // publish thread finished
        bool newChunkReceivedInLastIteration{true};
        while (!finished)
        {
            m_popper.tryPop()
                .and_then([&](auto& chunk) {
                    auto dummySample = *reinterpret_cast<DummySample*>(chunk.getUserPayload());
                    // Check if monotonically increasing
                    EXPECT_THAT(dummySample.m_dummy, Eq(forwardCounter));
                    m_chunkDistributor.deliverToAllStoredQueues(chunk);
                    forwardCounter++;
                    newChunkReceivedInLastIteration = true;
                })
                .or_else([&] {
                    if (!m_publisherRun.load(std::memory_order_relaxed))
                    {
                        if (newChunkReceivedInLastIteration)
                        {
                            newChunkReceivedInLastIteration = false;
                        }
                        else
                        {
                            finished = true;
                        }
                    }
                });
        }
        // Signal the next threads we're done
        m_forwarderRun = false;
    }

    void subscribe()
    {
        bool finished{false};
        // this is to prevent a race condition on thread shutdown; there must be two consecutive empty pops after the
        // forward thread finished
        bool newChunkReceivedInLastIteration{true};

        while (!finished)
        {
            m_chunkReceiver.tryGet()
                .and_then([&](auto& chunkHeader) {
                    auto dummySample = *reinterpret_cast<const DummySample*>(chunkHeader->userPayload());
                    // Check if monotonically increasing
                    EXPECT_THAT(dummySample.m_dummy, Eq(m_receiveCounter));
                    m_receiveCounter++;
                    m_chunkReceiver.release(chunkHeader);
                    newChunkReceivedInLastIteration = true;
                })
                .or_else([&](auto& result) {
                    if (result == ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                    {
                        if (!m_forwarderRun.load(std::memory_order_relaxed))
                        {
                            if (newChunkReceivedInLastIteration)
                            {
                                newChunkReceivedInLastIteration = false;
                            }
                            else
                            {
                                finished = true;
                            }
                        }
                    }
                    else
                    {
                        // Errors shall never occur
                        FAIL();
                    }
                });
        }
    }

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};
    uint64_t m_sendCounter{0};
    uint64_t m_receiveCounter{0};
    std::atomic<bool> m_publisherRun{true};
    std::atomic<bool> m_forwarderRun{true};

    // Memory objects
    Allocator m_memoryAllocator{g_memory, MEMORY_SIZE};
    MePooConfig m_mempoolConfig;
    MemoryManager m_memoryManager;

    // Objects used by publishing thread
    ChunkSenderData_t m_chunkSenderData{&m_memoryManager, SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA};
    ChunkSender<ChunkSenderData_t> m_chunkSender{&m_chunkSenderData};

    // Objects used by forwarding thread
    ChunkDistributorData_t m_chunkDistributorData{SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA};
    ChunkDistributor_t m_chunkDistributor{&m_chunkDistributorData};
    ChunkQueueData_t m_chunkQueueData{
        QueueFullPolicy::DISCARD_OLDEST_DATA,
        iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer}; // SoFi intentionally not used
    ChunkQueuePopper_t m_popper{&m_chunkQueueData};

    // Objects used by subscribing thread
    ChunkReceiverData_t m_chunkReceiverData{iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer,
                                            QueueFullPolicy::DISCARD_OLDEST_DATA}; // SoFi intentionally not used
    ChunkReceiver<ChunkReceiverData_t> m_chunkReceiver{&m_chunkReceiverData};
};

TEST_F(ChunkBuildingBlocks_IntegrationTest, TwoHopsThreeThreadsNoSoFi)
{
    std::thread subscribingThread(&ChunkBuildingBlocks_IntegrationTest::subscribe, this);
    std::thread forwardingThread(&ChunkBuildingBlocks_IntegrationTest::forward, this);
    std::thread publishingThread(&ChunkBuildingBlocks_IntegrationTest::publish, this);

    if (publishingThread.joinable())
    {
        publishingThread.join();
    }

    if (forwardingThread.joinable())
    {
        forwardingThread.join();
    }

    if (subscribingThread.joinable())
    {
        subscribingThread.join();
    }

    ASSERT_FALSE(m_popper.hasLostChunks());
    ASSERT_FALSE(m_chunkReceiver.hasLostChunks());
    EXPECT_EQ(m_sendCounter, m_receiveCounter);
}
