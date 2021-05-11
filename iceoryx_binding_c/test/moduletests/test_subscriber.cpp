// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_single_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/testing/roudi_environment/roudi_environment.hpp"
#include "mocks/wait_set_mock.hpp"

using namespace iox;
using namespace iox::popo;
using namespace iox::capro;
using namespace iox::mepoo;
using namespace iox::cxx;
using namespace iox::posix;

extern "C" {
#include "iceoryx_binding_c/chunk.h"
#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/wait_set.h"
}

#include "test.hpp"

using namespace ::testing;

class iox_sub_test : public Test
{
  public:
    const iox::capro::ServiceDescription TEST_SERVICE_DESCRIPTION{"a", "b", "c"};

  protected:
    iox_sub_test()
    {
        m_mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolconf, m_memoryAllocator, m_memoryAllocator);
        m_subscriber->m_portData = &m_portPtr;
    }

    void SetUp()
    {
        m_triggerCallbackLatestArgument = nullptr;
    }

    void TearDown()
    {
    }

    void Subscribe(SubscriberPortData* ptr)
    {
        iox_sub_subscribe(m_sut);

        SubscriberPortSingleProducer(ptr).tryGetCaProMessage();
        iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::ACK, TEST_SERVICE_DESCRIPTION);
        SubscriberPortSingleProducer(ptr).dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    }

    static void triggerCallback(iox_sub_t sub)
    {
        m_triggerCallbackLatestArgument = sub;
    }

    iox::mepoo::SharedChunk getChunkFromMemoryManager()
    {
        constexpr uint32_t USER_PAYLOAD_SIZE{100U};

        auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
        EXPECT_FALSE(chunkSettingsResult.has_error());
        if (chunkSettingsResult.has_error())
        {
            return nullptr;
        }
        auto& chunkSettings = chunkSettingsResult.value();

        return m_memoryManager.getChunk(chunkSettings);
    }

    static iox_sub_t m_triggerCallbackLatestArgument;
    static constexpr size_t MEMORY_SIZE = 1024 * 1024 * 100;
    uint8_t m_memory[MEMORY_SIZE];
    static constexpr uint32_t NUM_CHUNKS_IN_POOL = MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY + 2U;
    static constexpr uint32_t CHUNK_SIZE = 128U;

    Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    MePooConfig m_mempoolconf;
    MemoryManager m_memoryManager;

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};
    iox::popo::SubscriberOptions subscriberOptions{MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, 0U};
    iox::popo::SubscriberPortData m_portPtr{TEST_SERVICE_DESCRIPTION,
                                            "myApp",
                                            iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer,
                                            subscriberOptions};
    ChunkQueuePusher<SubscriberPortData::ChunkQueueData_t> m_chunkPusher{&m_portPtr.m_chunkReceiverData};
    std::unique_ptr<cpp2c_Subscriber> m_subscriber{new cpp2c_Subscriber};
    iox_sub_t m_sut = m_subscriber.get();

    ConditionVariableData m_condVar{"myApp"};
    std::unique_ptr<WaitSetMock> m_waitSet{new WaitSetMock(m_condVar)};
};

iox_sub_t iox_sub_test::m_triggerCallbackLatestArgument = nullptr;

TEST_F(iox_sub_test, initSubscriberWithNullptrForStorageReturnsNullptr)
{
    iox_sub_options_t options;
    iox_sub_options_init(&options);

    EXPECT_EQ(iox_sub_init(nullptr, "all", "glory", "hypnotoad", &options), nullptr);
}

// this crashes if the fixture is used, therefore a test without a fixture
TEST(iox_sub_test_DeathTest, initSubscriberWithNotInitializedPublisherOptionsTerminates)
{
    iox_sub_options_t options;
    iox_sub_storage_t storage;

    EXPECT_DEATH({ iox_sub_init(&storage, "a", "b", "c", &options); }, ".*");
}

TEST_F(iox_sub_test, initSubscriberWithDefaultOptionsWorks)
{
    iox::roudi::RouDiEnvironment roudiEnv;

    iox_runtime_init("hypnotoad");

    iox_sub_options_t options;
    iox_sub_options_init(&options);
    iox_sub_storage_t storage;

    EXPECT_NE(iox_sub_init(&storage, "a", "b", "c", &options), nullptr);
}

TEST_F(iox_sub_test, initialStateNotSubscribed)
{
    EXPECT_EQ(iox_sub_get_subscription_state(m_sut), SubscribeState_NOT_SUBSCRIBED);
}

TEST_F(iox_sub_test, offerLeadsToSubscibeRequestedState)
{
    iox_sub_subscribe(m_sut);

    SubscriberPortSingleProducer(&m_portPtr).tryGetCaProMessage();

    EXPECT_EQ(iox_sub_get_subscription_state(m_sut), SubscribeState_SUBSCRIBE_REQUESTED);
}

TEST_F(iox_sub_test, NACKResponseLeadsToSubscribeWaitForOfferState)
{
    iox_sub_subscribe(m_sut);

    SubscriberPortSingleProducer(&m_portPtr).tryGetCaProMessage();
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::NACK, TEST_SERVICE_DESCRIPTION);
    SubscriberPortSingleProducer(&m_portPtr).dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_EQ(iox_sub_get_subscription_state(m_sut), SubscribeState_WAIT_FOR_OFFER);
}

TEST_F(iox_sub_test, ACKResponseLeadsToSubscribedState)
{
    iox_sub_subscribe(m_sut);

    SubscriberPortSingleProducer(&m_portPtr).tryGetCaProMessage();
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::ACK, TEST_SERVICE_DESCRIPTION);
    SubscriberPortSingleProducer(&m_portPtr).dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_EQ(iox_sub_get_subscription_state(m_sut), SubscribeState_SUBSCRIBED);
}

TEST_F(iox_sub_test, UnsubscribeLeadsToUnscribeRequestedState)
{
    iox_sub_subscribe(m_sut);

    SubscriberPortSingleProducer(&m_portPtr).tryGetCaProMessage();
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::ACK, TEST_SERVICE_DESCRIPTION);
    SubscriberPortSingleProducer(&m_portPtr).dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    iox_sub_unsubscribe(m_sut);

    SubscriberPortSingleProducer(&m_portPtr).tryGetCaProMessage();

    EXPECT_EQ(iox_sub_get_subscription_state(m_sut), SubscribeState_UNSUBSCRIBE_REQUESTED);
}

TEST_F(iox_sub_test, initialStateNoChunksAvailable)
{
    const void* chunk = nullptr;
    EXPECT_EQ(iox_sub_take_chunk(m_sut, &chunk), ChunkReceiveResult_NO_CHUNK_AVAILABLE);
}

TEST_F(iox_sub_test, receiveChunkWhenThereIsOne)
{
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    const void* chunk = nullptr;
    EXPECT_EQ(iox_sub_take_chunk(m_sut, &chunk), ChunkReceiveResult_SUCCESS);
}

TEST_F(iox_sub_test, receiveChunkWithContent)
{
    this->Subscribe(&m_portPtr);
    struct data_t
    {
        int value;
    };

    auto sharedChunk = getChunkFromMemoryManager();
    static_cast<data_t*>(sharedChunk.getUserPayload())->value = 1234;
    m_chunkPusher.push(sharedChunk);

    const void* chunk = nullptr;

    ASSERT_EQ(iox_sub_take_chunk(m_sut, &chunk), ChunkReceiveResult_SUCCESS);
    EXPECT_THAT(static_cast<const data_t*>(chunk)->value, Eq(1234));
}

TEST_F(iox_sub_test, constChunkHeaderCanBeObtainedFromChunkAfterTake)
{
    this->Subscribe(&m_portPtr);
    auto sharedChunk = getChunkFromMemoryManager();
    m_chunkPusher.push(sharedChunk);

    const void* chunk = nullptr;

    ASSERT_EQ(iox_sub_take_chunk(m_sut, &chunk), ChunkReceiveResult_SUCCESS);
    auto chunkHeader = iox_chunk_header_from_user_payload_const(chunk);
    ASSERT_NE(chunkHeader, nullptr);
    auto userPayloadFromRoundTrip = iox_chunk_header_to_user_payload_const(chunkHeader);
    EXPECT_EQ(userPayloadFromRoundTrip, chunk);
}

TEST_F(iox_sub_test, receiveChunkWhenToManyChunksAreHold)
{
    this->Subscribe(&m_portPtr);
    const void* chunk = nullptr;
    for (uint64_t i = 0U; i < MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY + 1U; ++i)
    {
        m_chunkPusher.push(getChunkFromMemoryManager());
        iox_sub_take_chunk(m_sut, &chunk);
    }

    m_chunkPusher.push(getChunkFromMemoryManager());
    EXPECT_EQ(iox_sub_take_chunk(m_sut, &chunk), ChunkReceiveResult_TOO_MANY_CHUNKS_HELD_IN_PARALLEL);
}

TEST_F(iox_sub_test, releaseChunkWorks)
{
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    const void* chunk = nullptr;
    iox_sub_take_chunk(m_sut, &chunk);

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
    iox_sub_release_chunk(m_sut, chunk);
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}

TEST_F(iox_sub_test, releaseChunkQueuedChunksWorks)
{
    this->Subscribe(&m_portPtr);
    for (uint64_t i = 0U; i < MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY; ++i)
    {
        m_chunkPusher.push(getChunkFromMemoryManager());
    }

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY));
    iox_sub_release_queued_chunks(m_sut);
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}

TEST_F(iox_sub_test, initialStateHasNewChunksFalse)
{
    EXPECT_FALSE(iox_sub_has_chunks(m_sut));
}

TEST_F(iox_sub_test, receivingChunkLeadsToHasNewChunksTrue)
{
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    EXPECT_TRUE(iox_sub_has_chunks(m_sut));
}

TEST_F(iox_sub_test, initialStateHasNoLostChunks)
{
    EXPECT_FALSE(iox_sub_has_lost_chunks(m_sut));
}

TEST_F(iox_sub_test, sendingTooMuchLeadsToOverflow)
{
    this->Subscribe(&m_portPtr);
    for (uint64_t i = 0U; i < DefaultChunkQueueConfig::MAX_QUEUE_CAPACITY; ++i)
    {
        EXPECT_TRUE(m_chunkPusher.push(getChunkFromMemoryManager()));
    }
    EXPECT_FALSE(m_chunkPusher.push(getChunkFromMemoryManager()));
    m_chunkPusher.lostAChunk();

    EXPECT_TRUE(iox_sub_has_lost_chunks(m_sut));
}

TEST_F(iox_sub_test, attachingToWaitSetWorks)
{
    EXPECT_EQ(iox_ws_attach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA, 0U, NULL),
              WaitSetResult_SUCCESS);
    EXPECT_EQ(m_waitSet->size(), 1U);
}

TEST_F(iox_sub_test, attachingToAnotherWaitsetCleansupAtOriginalWaitset)
{
    WaitSetMock m_waitSet2{m_condVar};
    iox_ws_attach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA, 0U, NULL);

    EXPECT_EQ(iox_ws_attach_subscriber_state(&m_waitSet2, m_sut, SubscriberState_HAS_DATA, 0U, NULL),
              WaitSetResult_SUCCESS);
    EXPECT_EQ(m_waitSet->size(), 0U);
    EXPECT_EQ(m_waitSet2.size(), 1U);
}

TEST_F(iox_sub_test, detachingFromWaitSetWorks)
{
    iox_ws_attach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA, 0U, NULL);
    iox_ws_detach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA);
    EXPECT_EQ(m_waitSet->size(), 0U);
}

TEST_F(iox_sub_test, hasDataTriggersWaitSetWithCorrectNotificationId)
{
    iox_ws_attach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA, 587U, NULL);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto triggerVector = m_waitSet->wait();

    ASSERT_EQ(triggerVector.size(), 1U);
    EXPECT_EQ(triggerVector[0]->getNotificationId(), 587U);
}

TEST_F(iox_sub_test, hasDataTriggersWaitSetWithCorrectCallback)
{
    iox_ws_attach_subscriber_state(m_waitSet.get(), m_sut, SubscriberState_HAS_DATA, 0U, iox_sub_test::triggerCallback);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto triggerVector = m_waitSet->wait();

    ASSERT_EQ(triggerVector.size(), 1U);
    (*triggerVector[0U])();
    EXPECT_EQ(m_triggerCallbackLatestArgument, m_sut);
}

TEST_F(iox_sub_test, deinitSubscriberDetachesTriggerFromWaitSet)
{
    // malloc is used since iox_sub_deinit calls the d'tor of cpp2c_Subscriber
    auto subscriber = new (malloc(sizeof(cpp2c_Subscriber))) cpp2c_Subscriber();
    subscriber->m_portData = &m_portPtr;

    iox_ws_attach_subscriber_state(
        m_waitSet.get(), subscriber, SubscriberState_HAS_DATA, 0U, iox_sub_test::triggerCallback);

    iox_sub_deinit(subscriber);

    EXPECT_EQ(m_waitSet->size(), 0U);

    free(subscriber);
}

TEST_F(iox_sub_test, correctServiceDescriptionReturned)
{
    auto serviceDescription = iox_sub_get_service_description(m_sut);

    EXPECT_THAT(serviceDescription.serviceId, Eq(iox::capro::InvalidID));
    EXPECT_THAT(serviceDescription.instanceId, Eq(iox::capro::InvalidID));
    EXPECT_THAT(serviceDescription.eventId, Eq(iox::capro::InvalidID));
    EXPECT_THAT(std::string(serviceDescription.serviceString), Eq("a"));
    EXPECT_THAT(std::string(serviceDescription.instanceString), Eq("b"));
    EXPECT_THAT(std::string(serviceDescription.eventString), Eq("c"));
}

TEST(iox_sub_options_test, subscriberOptionsAreInitializedCorrectly)
{
    iox_sub_options_t sut;
    sut.queueCapacity = 37;
    sut.historyRequest = 73;
    sut.nodeName = "Dr.Gonzo";
    sut.subscribeOnCreate = false;
    sut.queueFullPolicy = QueueFullPolicy_BLOCK_PUBLISHER;

    SubscriberOptions options;
    // set subscribeOnCreate to the opposite of the expected default to check if it gets overwritten to default
    sut.subscribeOnCreate = (options.subscribeOnCreate == false) ? true : false;

    iox_sub_options_init(&sut);
    EXPECT_EQ(sut.queueCapacity, options.queueCapacity);
    EXPECT_EQ(sut.historyRequest, options.historyRequest);
    EXPECT_EQ(sut.nodeName, nullptr);
    EXPECT_EQ(sut.subscribeOnCreate, options.subscribeOnCreate);
    EXPECT_EQ(sut.queueFullPolicy, cpp2c::queueFullPolicy(options.queueFullPolicy));
    EXPECT_TRUE(iox_sub_options_is_initialized(&sut));
}

TEST(iox_sub_options_test, subscriberOptionsInitializationCheckReturnsTrueAfterDefaultInit)
{
    iox_sub_options_t sut;
    iox_sub_options_init(&sut);
    EXPECT_TRUE(iox_sub_options_is_initialized(&sut));
}

TEST(iox_sub_options_test, subscriberOptionsInitializationCheckReturnsFalseWithoutDefaultInit)
{
    iox_sub_options_t sut;
    EXPECT_FALSE(iox_sub_options_is_initialized(&sut));
}

TEST(iox_sub_options_test, subscriberOptionInitializationWithNullptrDoesNotCrash)
{
    EXPECT_EXIT(
        {
            iox_sub_options_init(nullptr);
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}
