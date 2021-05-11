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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "test.hpp"

#include <memory>

using namespace ::testing;

struct DummySample
{
    uint64_t dummy{42};
};

class ChunkSender_test : public Test
{
  protected:
    ChunkSender_test()
    {
        m_mempoolconf.addMemPool({SMALL_CHUNK, NUM_CHUNKS_IN_POOL});
        m_mempoolconf.addMemPool({BIG_CHUNK, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolconf, m_memoryAllocator, m_memoryAllocator);
    }

    ~ChunkSender_test()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    static constexpr size_t MEMORY_SIZE = 1024 * 1024;
    uint8_t m_memory[MEMORY_SIZE];
    static constexpr uint32_t NUM_CHUNKS_IN_POOL = 20;
    static constexpr uint32_t SMALL_CHUNK = 128;
    static constexpr uint32_t BIG_CHUNK = 256;
    static constexpr uint64_t HISTORY_CAPACITY = 4;
    static constexpr uint32_t MAX_NUMBER_QUEUES = 128;

    static constexpr uint32_t USER_PAYLOAD_ALIGNMENT = iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT;
    static constexpr uint32_t USER_HEADER_SIZE = iox::CHUNK_NO_USER_HEADER_SIZE;
    static constexpr uint32_t USER_HEADER_ALIGNMENT = iox::CHUNK_NO_USER_HEADER_ALIGNMENT;

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};
    iox::posix::Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    iox::mepoo::MePooConfig m_mempoolconf;
    iox::mepoo::MemoryManager m_memoryManager;

    struct ChunkDistributorConfig
    {
        static constexpr uint32_t MAX_QUEUES = MAX_NUMBER_QUEUES;
        static constexpr uint64_t MAX_HISTORY_CAPACITY = iox::MAX_PUBLISHER_HISTORY;
    };

    struct ChunkQueueConfig
    {
        static constexpr uint64_t MAX_QUEUE_CAPACITY = NUM_CHUNKS_IN_POOL;
    };

    using ChunkQueueData_t = iox::popo::ChunkQueueData<ChunkQueueConfig, iox::popo::ThreadSafePolicy>;
    using ChunkDistributorData_t = iox::popo::ChunkDistributorData<ChunkDistributorConfig,
                                                                   iox::popo::ThreadSafePolicy,
                                                                   iox::popo::ChunkQueuePusher<ChunkQueueData_t>>;
    using ChunkDistributor_t = iox::popo::ChunkDistributor<ChunkDistributorData_t>;
    using ChunkSenderData_t =
        iox::popo::ChunkSenderData<iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY, ChunkDistributorData_t>;

    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    ChunkSenderData_t m_chunkSenderData{
        &m_memoryManager, iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA, 0}; // must be 0 for test
    ChunkSenderData_t m_chunkSenderDataWithHistory{
        &m_memoryManager, iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA, HISTORY_CAPACITY};

    iox::popo::ChunkSender<ChunkSenderData_t> m_chunkSender{&m_chunkSenderData};
    iox::popo::ChunkSender<ChunkSenderData_t> m_chunkSenderWithHistory{&m_chunkSenderDataWithHistory};
};

TEST_F(ChunkSender_test, allocate_OneChunkWithoutUserHeaderAndSmallUserPayloadAlignmentResultsInSmallChunk)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{SMALL_CHUNK / 2};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, allocate_OneChunkWithoutUserHeaderAndLargeUserPayloadAlignmentResultsInLargeChunk)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{SMALL_CHUNK / 2};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{SMALL_CHUNK};
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, allocate_OneChunkWithLargeUserHeaderResultsInLargeChunk)
{
    constexpr uint32_t LARGE_HEADER_SIZE{SMALL_CHUNK};
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), LARGE_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, allocate_ChunkHasOriginIdSet)
{
    iox::UniquePortId uniqueId;
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        uniqueId, sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT((*maybeChunkHeader)->originId(), Eq(uniqueId));
}

TEST_F(ChunkSender_test, allocate_MultipleChunks)
{
    auto chunk1 = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunk2 = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_FALSE(chunk1.has_error());
    ASSERT_FALSE(chunk2.has_error());
    // must be different chunks
    EXPECT_THAT(*chunk1, Ne(*chunk2));
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(2U));
}

TEST_F(ChunkSender_test, allocate_Overflow)
{
    std::vector<iox::mepoo::ChunkHeader*> chunks;

    // tryAllocate chunks until MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY level
    for (size_t i = 0; i < iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        if (!maybeChunkHeader.has_error())
        {
            chunks.push_back(*maybeChunkHeader);
        }
    }

    for (size_t i = 0; i < iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY; i++)
    {
        EXPECT_THAT(chunks[i], Ne(nullptr));
    }
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks,
                Eq(iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY));

    // Allocate one more sample for overflow
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_TRUE(maybeChunkHeader.has_error());
    EXPECT_THAT(maybeChunkHeader.get_error(), Eq(iox::popo::AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL));
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks,
                Eq(iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY));
}

TEST_F(ChunkSender_test, freeChunk)
{
    std::vector<iox::mepoo::ChunkHeader*> chunks;

    // tryAllocate chunks until MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY level
    for (size_t i = 0; i < iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        if (!maybeChunkHeader.has_error())
        {
            chunks.push_back(*maybeChunkHeader);
        }
    }

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks,
                Eq(iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY));

    // release them all
    for (size_t i = 0; i < iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY; i++)
    {
        m_chunkSender.release(chunks[i]);
    }

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}

TEST_F(ChunkSender_test, freeInvalidChunk)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    auto errorHandlerCalled{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
        });

    ChunkMock<bool> myCrazyChunk;
    m_chunkSender.release(myCrazyChunk.chunkHeader());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, sendWithoutReceiver)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    if (!maybeChunkHeader.has_error())
    {
        auto sample = *maybeChunkHeader;
        m_chunkSender.send(sample);
        // chunk is still used because last chunk is stored
        EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
    }
}

TEST_F(ChunkSender_test, sendMultipleWithoutReceiverAndAlwaysLast)
{
    for (size_t i = 0; i < 100; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        ASSERT_FALSE(maybeChunkHeader.has_error());
        auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
        if (i > 0)
        {
            ASSERT_TRUE(maybeLastChunk.has_value());
            // We get the last chunk again
            EXPECT_TRUE(*maybeChunkHeader == *maybeLastChunk);
            EXPECT_TRUE((*maybeChunkHeader)->userPayload() == (*maybeLastChunk)->userPayload());
        }
        else
        {
            EXPECT_FALSE(maybeLastChunk.has_value());
        }
        auto sample = (*maybeChunkHeader)->userPayload();
        new (sample) DummySample();
        m_chunkSender.send(*maybeChunkHeader);
    }

    // Exactly one chunk is used because last chunk is stored
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, sendMultipleWithoutReceiverWithHistoryNoLastReuse)
{
    for (size_t i = 0; i < 10 * HISTORY_CAPACITY; i++)
    {
        auto maybeChunkHeader = m_chunkSenderWithHistory.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        ASSERT_FALSE(maybeChunkHeader.has_error());
        auto maybeLastChunk = m_chunkSenderWithHistory.tryGetPreviousChunk();
        if (i > 0)
        {
            ASSERT_TRUE(maybeLastChunk.has_value());
            // We don't get the last chunk again
            EXPECT_FALSE(*maybeChunkHeader == *maybeLastChunk);
            EXPECT_FALSE((*maybeChunkHeader)->userPayload() == (*maybeLastChunk)->userPayload());
        }
        else
        {
            EXPECT_FALSE(maybeLastChunk.has_value());
        }
        auto sample = (*maybeChunkHeader)->userPayload();
        new (sample) DummySample();
        m_chunkSenderWithHistory.send(*maybeChunkHeader);
    }

    // Used chunks == history size
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(HISTORY_CAPACITY));
}

TEST_F(ChunkSender_test, sendOneWithReceiver)
{
    ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());

    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    if (!maybeChunkHeader.has_error())
    {
        auto sample = (*maybeChunkHeader)->userPayload();
        new (sample) DummySample();
        m_chunkSender.send(*maybeChunkHeader);

        // consume the sample
        {
            iox::popo::ChunkQueuePopper<ChunkQueueData_t> myQueue(&m_chunkQueueData);
            EXPECT_FALSE(myQueue.empty());
            auto popRet = myQueue.tryPop();
            EXPECT_TRUE(popRet.has_value());
            auto dummySample = *reinterpret_cast<DummySample*>(popRet->getUserPayload());
            EXPECT_THAT(dummySample.dummy, Eq(42U));
        }
    }
}

TEST_F(ChunkSender_test, sendMultipleWithReceiver)
{
    ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());
    iox::popo::ChunkQueuePopper<ChunkQueueData_t> checkQueue(&m_chunkQueueData);
    EXPECT_TRUE(NUM_CHUNKS_IN_POOL <= checkQueue.getCurrentCapacity());

    for (size_t i = 0; i < NUM_CHUNKS_IN_POOL; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        EXPECT_FALSE(maybeChunkHeader.has_error());

        if (!maybeChunkHeader.has_error())
        {
            auto sample = (*maybeChunkHeader)->userPayload();
            new (sample) DummySample();
            static_cast<DummySample*>(sample)->dummy = i;
            m_chunkSender.send(*maybeChunkHeader);
        }
    }

    for (size_t i = 0; i < NUM_CHUNKS_IN_POOL; i++)
    {
        iox::popo::ChunkQueuePopper<ChunkQueueData_t> myQueue(&m_chunkQueueData);
        EXPECT_FALSE(myQueue.empty());
        auto popRet = myQueue.tryPop();
        EXPECT_TRUE(popRet.has_value());
        auto dummySample = *reinterpret_cast<DummySample*>(popRet->getUserPayload());
        EXPECT_THAT(dummySample.dummy, Eq(i));
        EXPECT_THAT(popRet->getChunkHeader()->sequenceNumber(), Eq(i));
    }
}

TEST_F(ChunkSender_test, sendTillRunningOutOfChunks)
{
    ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());
    iox::popo::ChunkQueuePopper<ChunkQueueData_t> checkQueue(&m_chunkQueueData);
    EXPECT_TRUE(NUM_CHUNKS_IN_POOL <= checkQueue.getCurrentCapacity());

    for (size_t i = 0; i < NUM_CHUNKS_IN_POOL; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        EXPECT_FALSE(maybeChunkHeader.has_error());

        if (!maybeChunkHeader.has_error())
        {
            auto sample = (*maybeChunkHeader)->userPayload();
            new (sample) DummySample();
            static_cast<DummySample*>(sample)->dummy = i;
            m_chunkSender.send(*maybeChunkHeader);
        }
    }

    auto errorHandlerCalled{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
        });

    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_TRUE(maybeChunkHeader.has_error());
    EXPECT_THAT(maybeChunkHeader.get_error(), Eq(iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS));
}

TEST_F(ChunkSender_test, sendInvalidChunk)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    auto errorHandlerCalled{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
        });

    ChunkMock<bool> myCrazyChunk;
    m_chunkSender.send(myCrazyChunk.chunkHeader());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, pushToHistory)
{
    for (size_t i = 0; i < 10 * HISTORY_CAPACITY; i++)
    {
        auto maybeChunkHeader = m_chunkSenderWithHistory.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        EXPECT_FALSE(maybeChunkHeader.has_error());
        m_chunkSenderWithHistory.pushToHistory(*maybeChunkHeader);
    }

    // Used chunks == history size
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(HISTORY_CAPACITY));
}

TEST_F(ChunkSender_test, pushInvalidChunkToHistory)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    auto errorHandlerCalled{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
        });

    ChunkMock<bool> myCrazyChunk;
    m_chunkSender.pushToHistory(myCrazyChunk.chunkHeader());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(ChunkSender_test, sendMultipleWithReceiverNoLastReuse)
{
    ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());

    for (size_t i = 0; i < NUM_CHUNKS_IN_POOL; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        ASSERT_FALSE(maybeChunkHeader.has_error());
        auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
        if (i > 0)
        {
            ASSERT_TRUE(maybeLastChunk.has_value());
            // No last chunk for us :-(
            EXPECT_FALSE(*maybeChunkHeader == *maybeLastChunk);
            EXPECT_FALSE((*maybeChunkHeader)->userPayload() == (*maybeLastChunk)->userPayload());
        }
        else
        {
            EXPECT_FALSE(maybeLastChunk.has_value());
        }
        auto sample = (*maybeChunkHeader)->userPayload();
        new (sample) DummySample();
        m_chunkSender.send(*maybeChunkHeader);
    }

    // All Chunks used now
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, NUM_CHUNKS_IN_POOL);
}

TEST_F(ChunkSender_test, sendMultipleWithReceiverLastReuseBecauseAlreadyConsumed)
{
    ASSERT_FALSE(m_chunkSender.tryAddQueue(&m_chunkQueueData).has_error());

    for (size_t i = 0; i < NUM_CHUNKS_IN_POOL; i++)
    {
        auto maybeChunkHeader = m_chunkSender.tryAllocate(
            iox::UniquePortId(), sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        ASSERT_FALSE(maybeChunkHeader.has_error());
        auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
        if (i > 0)
        {
            ASSERT_TRUE(maybeLastChunk.has_value());
            // We get the last chunk again
            EXPECT_TRUE(*maybeChunkHeader == *maybeLastChunk);
            EXPECT_TRUE((*maybeChunkHeader)->userPayload() == (*maybeLastChunk)->userPayload());
        }
        else
        {
            EXPECT_FALSE(maybeLastChunk.has_value());
        }
        auto sample = (*maybeChunkHeader)->userPayload();
        new (sample) DummySample();
        m_chunkSender.send(*maybeChunkHeader);

        iox::popo::ChunkQueuePopper<ChunkQueueData_t> myQueue(&m_chunkQueueData);
        EXPECT_FALSE(myQueue.empty());
        auto popRet = myQueue.tryPop();
        EXPECT_TRUE(popRet.has_value());
    }

    // All consumed but the lastChunk
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, 1);
}

TEST_F(ChunkSender_test, ReuseLastIfSmaller)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), BIG_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));

    auto chunkHeader = *maybeChunkHeader;
    m_chunkSender.send(chunkHeader);

    auto chunkSmaller = m_chunkSender.tryAllocate(
        iox::UniquePortId(), SMALL_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(chunkSmaller.has_error());

    // no small chunk used as big one is recycled
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));

    auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
    ASSERT_TRUE(maybeLastChunk.has_value());
    // We get the last chunk again
    EXPECT_TRUE(*chunkSmaller == *maybeLastChunk);
    EXPECT_TRUE((*chunkSmaller)->userPayload() == (*maybeLastChunk)->userPayload());
}

TEST_F(ChunkSender_test, NoReuseOfLastIfBigger)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), SMALL_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    auto chunkHeader = *maybeChunkHeader;
    m_chunkSender.send(chunkHeader);

    auto chunkBigger = m_chunkSender.tryAllocate(
        iox::UniquePortId(), BIG_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(chunkBigger.has_error());

    // no reuse, we hav a small and a big chunk in use
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));

    auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
    ASSERT_TRUE(maybeLastChunk.has_value());
    // not the last chunk
    EXPECT_FALSE(*chunkBigger == *maybeLastChunk);
    EXPECT_FALSE((*chunkBigger)->userPayload() == (*maybeLastChunk)->userPayload());
}

TEST_F(ChunkSender_test, ReuseOfLastIfBiggerButFitsInChunk)
{
    auto maybeChunkHeader = m_chunkSender.tryAllocate(
        iox::UniquePortId(), SMALL_CHUNK - 10, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));

    auto chunkHeader = *maybeChunkHeader;
    m_chunkSender.send(chunkHeader);

    auto chunkBigger = m_chunkSender.tryAllocate(
        iox::UniquePortId(), SMALL_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(chunkBigger.has_error());

    // reuse as it still fits in the small chunk
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(0U));

    auto maybeLastChunk = m_chunkSender.tryGetPreviousChunk();
    ASSERT_TRUE(maybeLastChunk.has_value());
    // not the last chunk
    EXPECT_TRUE(*chunkBigger == *maybeLastChunk);
    EXPECT_TRUE((*chunkBigger)->userPayload() == (*maybeLastChunk)->userPayload());
}

TEST_F(ChunkSender_test, Cleanup)
{
    EXPECT_TRUE((HISTORY_CAPACITY + iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY) <= NUM_CHUNKS_IN_POOL);

    for (size_t i = 0; i < HISTORY_CAPACITY; i++)
    {
        auto maybeChunkHeader = m_chunkSenderWithHistory.tryAllocate(
            iox::UniquePortId(), SMALL_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        EXPECT_FALSE(maybeChunkHeader.has_error());
        m_chunkSenderWithHistory.send(*maybeChunkHeader);
    }

    for (size_t i = 0; i < iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY; i++)
    {
        auto maybeChunkHeader = m_chunkSenderWithHistory.tryAllocate(
            iox::UniquePortId(), SMALL_CHUNK, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        EXPECT_FALSE(maybeChunkHeader.has_error());
    }

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks,
                Eq(HISTORY_CAPACITY + iox::MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY));

    m_chunkSenderWithHistory.releaseAll();

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}
