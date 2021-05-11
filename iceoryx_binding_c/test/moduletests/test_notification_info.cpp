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

#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_single_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/popo/notification_info.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "mocks/wait_set_mock.hpp"

using namespace iox;
using namespace iox::popo;
using namespace iox::capro;
using namespace iox::mepoo;
using namespace iox::cxx;
using namespace iox::posix;


extern "C" {
#include "iceoryx_binding_c/notification_info.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/wait_set.h"
}

#include "test.hpp"

using namespace ::testing;

class iox_notification_info_test : public Test
{
  public:
    const iox::capro::ServiceDescription TEST_SERVICE_DESCRIPTION{"a", "b", "c"};

    void SetUp()
    {
        m_lastNotificationCallbackArgument = nullptr;
        m_mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolconf, m_memoryAllocator, m_memoryAllocator);
        m_subscriber.m_portData = &m_portPtr;
    }

    void TearDown()
    {
    }

    void Subscribe(SubscriberPortData* ptr)
    {
        iox_sub_subscribe(m_subscriberHandle);

        SubscriberPortSingleProducer(ptr).tryGetCaProMessage();
        iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::ACK, TEST_SERVICE_DESCRIPTION);
        SubscriberPortSingleProducer(ptr).dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    }

    static void notificationCallback(UserTrigger* arg)
    {
        m_lastNotificationCallbackArgument = arg;
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


    static UserTrigger* m_lastNotificationCallbackArgument;
    ConditionVariableData m_condVar{"myApp"};
    WaitSetMock m_waitSet{m_condVar};
    UserTrigger m_userTrigger;

    static constexpr uint32_t NUM_CHUNKS_IN_POOL = MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY + 2;
    static constexpr uint32_t CHUNK_SIZE = 128U;
    static constexpr size_t MEMORY_SIZE = 1024 * 1024 * 100;
    uint8_t m_memory[MEMORY_SIZE];
    Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    MePooConfig m_mempoolconf;
    MemoryManager m_memoryManager;
    SubscriberOptions m_subscriberOptions{MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, 0U};
    iox::popo::SubscriberPortData m_portPtr{TEST_SERVICE_DESCRIPTION,
                                            "myApp",
                                            iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer,
                                            m_subscriberOptions};
    ChunkQueuePusher<SubscriberPortData::ChunkQueueData_t> m_chunkPusher{&m_portPtr.m_chunkReceiverData};
    cpp2c_Subscriber m_subscriber;
    iox_sub_t m_subscriberHandle = &m_subscriber;
};

UserTrigger* iox_notification_info_test::m_lastNotificationCallbackArgument = nullptr;

TEST_F(iox_notification_info_test, notificationInfoHasCorrectId)
{
    constexpr uint64_t ARBITRARY_EVENT_ID = 123U;
    ASSERT_FALSE(m_waitSet.attachEvent(m_userTrigger, ARBITRARY_EVENT_ID).has_error());
    m_userTrigger.trigger();

    auto notificationInfoVector = m_waitSet.wait();

    ASSERT_THAT(notificationInfoVector.size(), Eq(1U));
    EXPECT_EQ(iox_notification_info_get_notification_id(notificationInfoVector[0]), ARBITRARY_EVENT_ID);
}

TEST_F(iox_notification_info_test, notificationOriginIsUserTriggerPointerWhenItsOriginatingFromThem)
{
    constexpr uint64_t ARBITRARY_EVENT_ID = 124U;
    ASSERT_FALSE(m_waitSet.attachEvent(m_userTrigger, ARBITRARY_EVENT_ID).has_error());
    m_userTrigger.trigger();

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(notificationInfoVector[0U], &m_userTrigger));
    EXPECT_FALSE(iox_notification_info_does_originate_from_subscriber(notificationInfoVector[0U], &m_subscriber));
}

TEST_F(iox_notification_info_test, notificationOriginIsSubscriberPointerWhenItsOriginatingFromThemStateBased)
{
    iox_ws_attach_subscriber_state(&m_waitSet, m_subscriberHandle, SubscriberState_HAS_DATA, 587U, NULL);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_TRUE(iox_notification_info_does_originate_from_subscriber(notificationInfoVector[0U], m_subscriberHandle));
    EXPECT_FALSE(iox_notification_info_does_originate_from_user_trigger(notificationInfoVector[0U], &m_userTrigger));
}

TEST_F(iox_notification_info_test, notificationOriginIsSubscriberPointerWhenItsOriginatingFromThemEventBased)
{
    iox_ws_attach_subscriber_event(&m_waitSet, m_subscriberHandle, SubscriberEvent_DATA_RECEIVED, 587U, NULL);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_TRUE(iox_notification_info_does_originate_from_subscriber(notificationInfoVector[0U], m_subscriberHandle));
    EXPECT_FALSE(iox_notification_info_does_originate_from_user_trigger(notificationInfoVector[0U], &m_userTrigger));
}

TEST_F(iox_notification_info_test, getOriginReturnsPointerToUserTriggerWhenOriginatingFromThem)
{
    constexpr uint64_t ARBITRARY_EVENT_ID = 89121U;
    ASSERT_FALSE(m_waitSet.attachEvent(m_userTrigger, ARBITRARY_EVENT_ID).has_error());
    m_userTrigger.trigger();

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_EQ(iox_notification_info_get_user_trigger_origin(notificationInfoVector[0U]), &m_userTrigger);
    EXPECT_EQ(iox_notification_info_get_subscriber_origin(notificationInfoVector[0U]), nullptr);
}

TEST_F(iox_notification_info_test, getOriginReturnsPointerToSubscriberWhenOriginatingFromThemStateBased)
{
    iox_ws_attach_subscriber_state(&m_waitSet, m_subscriberHandle, SubscriberState_HAS_DATA, 587U, NULL);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_EQ(iox_notification_info_get_user_trigger_origin(notificationInfoVector[0U]), nullptr);
    EXPECT_EQ(iox_notification_info_get_subscriber_origin(notificationInfoVector[0U]), m_subscriberHandle);
}

TEST_F(iox_notification_info_test, getOriginReturnsPointerToSubscriberWhenOriginatingFromThemEventBased)
{
    iox_ws_attach_subscriber_event(&m_waitSet, m_subscriberHandle, SubscriberEvent_DATA_RECEIVED, 587U, NULL);
    this->Subscribe(&m_portPtr);
    m_chunkPusher.push(getChunkFromMemoryManager());

    auto notificationInfoVector = m_waitSet.wait();

    EXPECT_EQ(iox_notification_info_get_user_trigger_origin(notificationInfoVector[0U]), nullptr);
    EXPECT_EQ(iox_notification_info_get_subscriber_origin(notificationInfoVector[0U]), m_subscriberHandle);
}

TEST_F(iox_notification_info_test, callbackCanBeCalledOnce)
{
    constexpr uint64_t ARBITRARY_EVENT_ID = 80U;
    ASSERT_FALSE(m_waitSet
                     .attachEvent(m_userTrigger,
                                  ARBITRARY_EVENT_ID,
                                  createNotificationCallback(iox_notification_info_test::notificationCallback))
                     .has_error());
    m_userTrigger.trigger();

    auto notificationInfoVector = m_waitSet.wait();

    iox_notification_info_call(notificationInfoVector[0U]);
    EXPECT_EQ(m_lastNotificationCallbackArgument, &m_userTrigger);
}

TEST_F(iox_notification_info_test, callbackCanBeCalledMultipleTimes)
{
    constexpr uint64_t ARBITRARY_EVENT_ID = 180U;
    ASSERT_FALSE(m_waitSet
                     .attachEvent(m_userTrigger,
                                  ARBITRARY_EVENT_ID,
                                  createNotificationCallback(iox_notification_info_test::notificationCallback))
                     .has_error());
    m_userTrigger.trigger();
    auto notificationInfoVector = m_waitSet.wait();

    iox_notification_info_call(notificationInfoVector[0U]);
    m_lastNotificationCallbackArgument = nullptr;
    iox_notification_info_call(notificationInfoVector[0U]);
    m_lastNotificationCallbackArgument = nullptr;
    iox_notification_info_call(notificationInfoVector[0U]);
    m_lastNotificationCallbackArgument = nullptr;
    iox_notification_info_call(notificationInfoVector[0U]);

    EXPECT_EQ(m_lastNotificationCallbackArgument, &m_userTrigger);
}
