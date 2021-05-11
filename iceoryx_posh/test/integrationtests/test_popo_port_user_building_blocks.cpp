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
#include "iceoryx_posh/internal/popo/ports/publisher_port_roudi.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_multi_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_single_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/concurrent/smart_lock.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "test.hpp"

#include <chrono>
#include <sstream>
#include <thread>

using namespace ::testing;
using namespace iox::popo;
using namespace iox::capro;
using namespace iox::cxx;
using namespace iox::mepoo;
using namespace iox::posix;
using ::testing::Return;

struct DummySample
{
    uint64_t m_dummy{42U};
};

static const ServiceDescription TEST_SERVICE_DESCRIPTION("x", "y", "z");
static const iox::RuntimeName_t TEST_SUBSCRIBER_RUNTIME_NAME("mySubscriberApp");
static const iox::RuntimeName_t TEST_PUBLISHER_RUNTIME_NAME("myPublisherApp");

static constexpr uint32_t NUMBER_OF_PUBLISHERS = 17U;
static constexpr uint32_t ITERATIONS = 1000U;

static constexpr uint32_t NUM_CHUNKS_IN_POOL = NUMBER_OF_PUBLISHERS * ITERATIONS;
static constexpr uint32_t SMALL_CHUNK = 128U;
static constexpr uint32_t CHUNK_META_INFO_SIZE = 256U;
static constexpr size_t MEMORY_SIZE = NUM_CHUNKS_IN_POOL * (SMALL_CHUNK + CHUNK_META_INFO_SIZE);
alignas(64) static uint8_t g_memory[MEMORY_SIZE];

class PortUser_IntegrationTest : public Test
{
  public:
    PortUser_IntegrationTest()
    {
        m_mempoolConfig.addMemPool({SMALL_CHUNK, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolConfig, m_memoryAllocator, m_memoryAllocator);
    }

    ~PortUser_IntegrationTest()
    {
    }

    void SetUp()
    {
        for (uint32_t i = 0U; i < NUMBER_OF_PUBLISHERS; i++)
        {
            std::stringstream publisherRuntimeName;
            publisherRuntimeName << TEST_PUBLISHER_RUNTIME_NAME << i;

            iox::RuntimeName_t runtimeName(TruncateToCapacity, publisherRuntimeName.str().c_str());

            m_publisherPortDataVector.emplace_back(
                TEST_SERVICE_DESCRIPTION, runtimeName, &m_memoryManager, PublisherOptions());
            m_publisherPortUserVector.emplace_back(&m_publisherPortDataVector.back());
            m_publisherPortRouDiVector.emplace_back(&m_publisherPortDataVector.back());
        }
    }

    void TearDown()
    {
        for (uint32_t i = 0U; i < NUMBER_OF_PUBLISHERS; i++)
        {
            m_publisherPortUserVector[i].stopOffer();
            static_cast<void>(m_publisherPortRouDiVector[i].tryGetCaProMessage());
        }

        m_subscriberPortUserSingleProducer.unsubscribe();
        m_subscriberPortUserMultiProducer.unsubscribe();

        static_cast<void>(m_subscriberPortRouDiSingleProducer.tryGetCaProMessage());
        static_cast<void>(m_subscriberPortRouDiMultiProducer.tryGetCaProMessage());
    }

    GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0U); },
                                [] { iox::popo::internal::unsetUniqueRouDiId(); }};

    uint64_t m_receiveCounter{0U};
    std::atomic<uint64_t> m_sendCounter{0U};
    std::atomic<uint64_t> m_publisherRunFinished{0U};

    // Memory objects
    Allocator m_memoryAllocator{g_memory, MEMORY_SIZE};
    MePooConfig m_mempoolConfig;
    MemoryManager m_memoryManager;

    using ConcurrentCaproMessageVector_t = iox::concurrent::smart_lock<vector<CaproMessage, 1>>;
    ConcurrentCaproMessageVector_t m_concurrentCaproMessageExchange;
    ConcurrentCaproMessageVector_t m_concurrentCaproMessageRx;

    // subscriber port for single producer
    SubscriberPortData m_subscriberPortDataSingleProducer{TEST_SERVICE_DESCRIPTION,
                                                          TEST_SUBSCRIBER_RUNTIME_NAME,
                                                          VariantQueueTypes::SoFi_SingleProducerSingleConsumer,
                                                          SubscriberOptions()};
    SubscriberPortUser m_subscriberPortUserSingleProducer{&m_subscriberPortDataSingleProducer};
    SubscriberPortSingleProducer m_subscriberPortRouDiSingleProducer{&m_subscriberPortDataSingleProducer};

    // subscriber port for multi producer
    SubscriberPortData m_subscriberPortDataMultiProducer{TEST_SERVICE_DESCRIPTION,
                                                         TEST_SUBSCRIBER_RUNTIME_NAME,
                                                         VariantQueueTypes::SoFi_MultiProducerSingleConsumer,
                                                         SubscriberOptions()};
    SubscriberPortUser m_subscriberPortUserMultiProducer{&m_subscriberPortDataMultiProducer};
    SubscriberPortMultiProducer m_subscriberPortRouDiMultiProducer{&m_subscriberPortDataMultiProducer};

    // publisher port
    vector<PublisherPortData, NUMBER_OF_PUBLISHERS> m_publisherPortDataVector;
    vector<PublisherPortUser, NUMBER_OF_PUBLISHERS> m_publisherPortUserVector;
    vector<PublisherPortRouDi, NUMBER_OF_PUBLISHERS> m_publisherPortRouDiVector;

    vector<std::thread, NUMBER_OF_PUBLISHERS> m_publisherThreadVector;

    inline CaproMessage waitForCaproMessage(const ConcurrentCaproMessageVector_t& concurrentCaproMessageVector,
                                            const CaproMessageType& caproMessageType)
    {
        bool finished{false};
        CaproMessage caproMessage;

        // Wait until the expected CaPro message has arrived in the shared message vector between subscriber and the
        // first publisher thread
        do
        {
            // Add delay to allow other thread accessing the shared resource
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            {
                auto guardedVector = concurrentCaproMessageVector.getScopeGuard();
                if (guardedVector->size() != 0U)
                {
                    caproMessage = guardedVector->back();

                    if (caproMessage.m_type == caproMessageType)
                    {
                        guardedVector->pop_back();
                        finished = true;
                    }
                }
            }
        } while (!finished);

        return caproMessage;
    }

    template <typename SubscriberPortType>
    void subscriberThread(uint32_t numberOfPublishers,
                          SubscriberPortType& subscriberPortRouDi,
                          SubscriberPortUser& subscriberPortUser)
    {
        bool finished{false};

        // Wait for publisher to be ready
        auto caproMessage = waitForCaproMessage(m_concurrentCaproMessageExchange, CaproMessageType::OFFER);

        // Subscribe to publisher
        subscriberPortUser.subscribe();
        auto maybeCaproMessage = subscriberPortRouDi.tryGetCaProMessage();
        if (maybeCaproMessage.has_value())
        {
            caproMessage = maybeCaproMessage.value();
            m_concurrentCaproMessageExchange->push_back(caproMessage);
        }
        else
        {
            // Error shall never occur
            FAIL() << "Error in subscriber SUB CaPro message!";
        }

        // Wait for subscription ACK from publisher
        caproMessage = waitForCaproMessage(m_concurrentCaproMessageExchange, CaproMessageType::ACK);

        // Let RouDi change state to finish subscription
        static_cast<void>(subscriberPortRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage));

        // Subscription done and ready to receive samples
        while (!finished)
        {
            // Try to receive chunk
            subscriberPortUser.tryGetChunk()
                .and_then([&](auto& chunkHeader) {
                    m_receiveCounter++;
                    subscriberPortUser.releaseChunk(chunkHeader);
                })
                .or_else([&](auto& result) {
                    if (result == ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                    {
                        // Nothing received -> check if publisher(s) still running
                        if (m_publisherRunFinished.load(std::memory_order_relaxed) == numberOfPublishers)
                        {
                            finished = true;
                        }
                    }
                    else
                    {
                        // Errors shall never occur
                        FAIL() << "Error in tryGetChunk(): " << static_cast<uint32_t>(result);
                    }
                });
        }
    }

    void publisherThread(uint32_t publisherThreadIndex,
                         PublisherPortRouDi& publisherPortRouDi,
                         PublisherPortUser& publisherPortUser)
    {
        CaproMessage caproMessage;

        // Publisher offers its service
        publisherPortUser.offer();

        // Let RouDi change state and send OFFER to subscriber
        auto maybeCaproMessage = publisherPortRouDi.tryGetCaProMessage();

        if (publisherThreadIndex == 0U)
        {
            // First publisher thread will sync with subscriber
            if (maybeCaproMessage.has_value())
            {
                caproMessage = maybeCaproMessage.value();
                m_concurrentCaproMessageExchange->push_back(caproMessage);
            }
            else
            {
                // Error shall never occur
                FAIL() << "Error in publisher OFFER CaPro message!";
            }

            // Wait for subscriber to subscribe
            caproMessage = waitForCaproMessage(m_concurrentCaproMessageExchange, CaproMessageType::SUB);
            m_concurrentCaproMessageRx->push_back(caproMessage);

            // Send ACK to subscriber
            maybeCaproMessage =
                publisherPortRouDi.dispatchCaProMessageAndGetPossibleResponse(m_concurrentCaproMessageRx->back());
            if (maybeCaproMessage.has_value())
            {
                caproMessage = maybeCaproMessage.value();
                m_concurrentCaproMessageExchange->push_back(caproMessage);
            }
            else
            {
                // Error shall never occur
                FAIL() << "Error in publisher ACK CaPro message!";
            }
        }
        else
        {
            // All other publisher threads wait for the first thread to be synced with subscriber (indicated by
            // receiving a SUB message) to continue
            CaproMessage caproMessageRouDi(CaproMessageType::UNSUB, TEST_SERVICE_DESCRIPTION);
            do
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));

                if (m_concurrentCaproMessageRx->size() != 0U)
                {
                    caproMessageRouDi = m_concurrentCaproMessageRx->back();
                }

            } while (caproMessageRouDi.m_type != CaproMessageType::SUB);

            static_cast<void>(publisherPortRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessageRouDi));
        }

        // Subscriber is ready to receive -> start sending samples
        for (size_t i = 0U; i < ITERATIONS; i++)
        {
            publisherPortUser
                .tryAllocateChunk(sizeof(DummySample),
                                  alignof(DummySample),
                                  iox::CHUNK_NO_USER_HEADER_SIZE,
                                  iox::CHUNK_NO_USER_HEADER_ALIGNMENT)
                .and_then([&](auto chunkHeader) {
                    auto sample = chunkHeader->userPayload();
                    new (sample) DummySample();
                    static_cast<DummySample*>(sample)->m_dummy = i;
                    publisherPortUser.sendChunk(chunkHeader);
                    m_sendCounter++;
                })
                .or_else([](auto error) {
                    // Errors shall never occur
                    FAIL() << "Error in tryAllocateChunk(): " << static_cast<uint32_t>(error);
                });

            /// Add some jitter to make thread breathe
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
        }

        // Signal the subscriber thread we're done
        m_publisherRunFinished++;
    }
};

TIMING_TEST_F(PortUser_IntegrationTest, SingleProducer, Repeat(5), [&] {
    constexpr uint32_t NUMBER_OF_PUBLISHERS_SINGLE_PRODUCER = 1U;
    constexpr uint32_t INDEX_OF_PUBLISHER_SINGLE_PRODUCER = 0U;

    std::thread subscribingThread(std::bind(&PortUser_IntegrationTest::subscriberThread<SubscriberPortSingleProducer>,
                                            this,
                                            NUMBER_OF_PUBLISHERS_SINGLE_PRODUCER,
                                            std::ref(PortUser_IntegrationTest::m_subscriberPortRouDiSingleProducer),
                                            std::ref(PortUser_IntegrationTest::m_subscriberPortUserSingleProducer)));
    std::thread publishingThread(std::bind(&PortUser_IntegrationTest::publisherThread,
                                           this,
                                           INDEX_OF_PUBLISHER_SINGLE_PRODUCER,
                                           std::ref(PortUser_IntegrationTest::m_publisherPortRouDiVector.front()),
                                           std::ref(PortUser_IntegrationTest::m_publisherPortUserVector.front())));

    if (subscribingThread.joinable())
    {
        subscribingThread.join();
    }

    if (publishingThread.joinable())
    {
        publishingThread.join();
    }

    TIMING_TEST_EXPECT_TRUE(m_sendCounter.load(std::memory_order_relaxed) == m_receiveCounter);
    TIMING_TEST_EXPECT_FALSE(PortUser_IntegrationTest::m_subscriberPortUserMultiProducer.hasLostChunksSinceLastCall());
});

TIMING_TEST_F(PortUser_IntegrationTest, MultiProducer, Repeat(5), [&] {
    std::thread subscribingThread(std::bind(&PortUser_IntegrationTest::subscriberThread<SubscriberPortMultiProducer>,
                                            this,
                                            NUMBER_OF_PUBLISHERS,
                                            std::ref(PortUser_IntegrationTest::m_subscriberPortRouDiMultiProducer),
                                            std::ref(PortUser_IntegrationTest::m_subscriberPortUserMultiProducer)));


    for (uint32_t i = 0U; i < NUMBER_OF_PUBLISHERS; i++)
    {
        m_publisherThreadVector.emplace_back(
            std::bind(&PortUser_IntegrationTest::publisherThread,
                      this,
                      i,
                      std::ref(PortUser_IntegrationTest::m_publisherPortRouDiVector[i]),
                      std::ref(PortUser_IntegrationTest::m_publisherPortUserVector[i])));
    }

    if (subscribingThread.joinable())
    {
        subscribingThread.join();
    }

    for (uint32_t i = 0U; i < NUMBER_OF_PUBLISHERS; i++)
    {
        if (m_publisherThreadVector[i].joinable())
        {
            m_publisherThreadVector[i].join();
        }
    }

    TIMING_TEST_EXPECT_TRUE(m_sendCounter.load(std::memory_order_relaxed) == m_receiveCounter);
    TIMING_TEST_EXPECT_FALSE(PortUser_IntegrationTest::m_subscriberPortUserMultiProducer.hasLostChunksSinceLastCall());
});
