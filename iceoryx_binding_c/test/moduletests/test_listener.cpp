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

#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_single_producer.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"

using namespace iox;
using namespace iox::popo;

extern "C" {
#include "iceoryx_binding_c/listener.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/user_trigger.h"
}

#include "test.hpp"

#include <atomic>
#include <thread>

using namespace ::testing;
using namespace iox::posix;
using namespace iox::mepoo;


namespace
{
iox_user_trigger_t g_userTriggerCallbackArgument = nullptr;
iox_sub_t g_subscriberCallbackArgument = nullptr;
void* g_contextData = nullptr;

void userTriggerCallback(iox_user_trigger_t userTrigger)
{
    g_userTriggerCallbackArgument = userTrigger;
}

void userTriggerCallbackWithContextData(iox_user_trigger_t userTrigger, void* const contextData)
{
    g_userTriggerCallbackArgument = userTrigger;
    g_contextData = contextData;
}

void subscriberCallback(iox_sub_t subscriber)
{
    g_subscriberCallbackArgument = subscriber;
}

void subscriberCallbackWithContextData(iox_sub_t subscriber, void* const contextData)
{
    g_subscriberCallbackArgument = subscriber;
    g_contextData = contextData;
}

class iox_listener_test : public Test
{
  public:
    class TestListener : public Listener
    {
      public:
        TestListener(ConditionVariableData& condVar)
            : Listener(condVar)
        {
        }
    };

    void SetUp() override
    {
        g_userTriggerCallbackArgument = nullptr;
        g_subscriberCallbackArgument = nullptr;
        g_contextData = nullptr;

        m_mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolconf, m_memoryAllocator, m_memoryAllocator);

        m_subscriberPortData.resize(MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U,
                                    TEST_SERVICE_DESCRIPTION,
                                    "myApp",
                                    iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer,
                                    subscriberOptions);
        m_subscriber.resize(MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U);
        for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U; ++i)
        {
            m_userTrigger.emplace_back(iox_user_trigger_init(&m_userTriggerStorage[i]));
            m_subscriber[i].m_portData = &m_subscriberPortData[i];
            m_chunkPusher.emplace_back(&m_subscriberPortData[i].m_chunkReceiverData);
        }
    }

    void Subscribe(cpp2c_Subscriber& subscriber)
    {
        iox_sub_subscribe(&subscriber);

        SubscriberPortSingleProducer(subscriber.m_portData).tryGetCaProMessage();
        iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::ACK, TEST_SERVICE_DESCRIPTION);
        SubscriberPortSingleProducer(subscriber.m_portData).dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    }

    void TearDown() override
    {
        for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U; ++i)
        {
            iox_user_trigger_deinit(m_userTrigger[i]);
        }
    }

    void AttachAllUserTrigger()
    {
        for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
        {
            EXPECT_THAT(iox_listener_attach_user_trigger_event(&m_sut, m_userTrigger[i], &userTriggerCallback),
                        Eq(iox_ListenerResult::ListenerResult_SUCCESS));
            EXPECT_THAT(iox_listener_size(&m_sut), Eq(i + 1U));
        }
    }

    void AttachAllSubscriber()
    {
        for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
        {
            EXPECT_THAT(
                iox_listener_attach_subscriber_event(
                    &m_sut, &m_subscriber[i], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
            EXPECT_THAT(iox_listener_size(&m_sut), Eq(i + 1U));
        }
    }

    ConditionVariableData m_condVar{"hypnotoadKnueppeltRetour"};
    TestListener m_sut{m_condVar};

    iox_user_trigger_storage_t m_userTriggerStorage[MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U];
    cxx::vector<iox_user_trigger_t, MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1U> m_userTrigger;

    static constexpr uint32_t NUM_CHUNKS_IN_POOL = MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY + 2U;
    static constexpr uint32_t CHUNK_SIZE = 128U;
    static constexpr uint64_t MEMORY_SIZE = 1024U * 1024U * 100U;
    uint8_t m_memory[MEMORY_SIZE];
    Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    MePooConfig m_mempoolconf;
    MemoryManager m_memoryManager;

    const iox::capro::ServiceDescription TEST_SERVICE_DESCRIPTION{"a", "b", "c"};

    iox::popo::SubscriberOptions subscriberOptions{MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, 0U};

    cxx::vector<iox::popo::SubscriberPortData, MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1> m_subscriberPortData;
    cxx::vector<cpp2c_Subscriber, MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1> m_subscriber;
    cxx::vector<ChunkQueuePusher<SubscriberPortData::ChunkQueueData_t>, MAX_NUMBER_OF_EVENTS_PER_LISTENER + 1>
        m_chunkPusher;
    static constexpr std::chrono::milliseconds TIMEOUT = std::chrono::milliseconds(100);
};
constexpr std::chrono::milliseconds iox_listener_test::TIMEOUT;


} // namespace

TEST_F(iox_listener_test, InitListenerWithNullptrForStorageReturnsNullptr)
{
    EXPECT_EQ(iox_listener_init(nullptr), nullptr);
}

TEST_F(iox_listener_test, CapacityIsCorrect)
{
    EXPECT_THAT(iox_listener_capacity(&m_sut), Eq(MAX_NUMBER_OF_EVENTS_PER_LISTENER));
}

TEST_F(iox_listener_test, SizeIsZeroWhenCreated)
{
    EXPECT_THAT(iox_listener_size(&m_sut), Eq(0U));
}

TEST_F(iox_listener_test, SizeIsOneWhenOneClassIsAttached)
{
    EXPECT_THAT(iox_listener_attach_user_trigger_event(&m_sut, m_userTrigger[0U], &userTriggerCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
    EXPECT_THAT(iox_listener_size(&m_sut), Eq(1U));
}

TEST_F(iox_listener_test, SizeEqualsCapacityWhenMaximumIsAttached)
{
    AttachAllUserTrigger();
    EXPECT_THAT(iox_listener_size(&m_sut), Eq(iox_listener_capacity(&m_sut)));
}

TEST_F(iox_listener_test, SizeDecreasesWhenUserTriggersAreDetached)
{
    AttachAllUserTrigger();

    for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        iox_listener_detach_user_trigger_event(&m_sut, m_userTrigger[i]);
        EXPECT_THAT(iox_listener_size(&m_sut), Eq(iox_listener_capacity(&m_sut) - i - 1U));
    }
}

TEST_F(iox_listener_test, FullListenerReturnsLISTENER_FULLWhenAnotherUserTriggerIsAttached)
{
    AttachAllUserTrigger();

    EXPECT_THAT(iox_listener_attach_user_trigger_event(
                    &m_sut, m_userTrigger[MAX_NUMBER_OF_EVENTS_PER_LISTENER], &userTriggerCallback),
                Eq(iox_ListenerResult::ListenerResult_LISTENER_FULL));
}

TEST_F(iox_listener_test, AttachingTheSameUserTriggerTwiceLeadsToEVENT_ALREADY_ATTACHED)
{
    EXPECT_THAT(iox_listener_attach_user_trigger_event(&m_sut, m_userTrigger[0U], &userTriggerCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
    EXPECT_THAT(iox_listener_attach_user_trigger_event(&m_sut, m_userTrigger[0U], &userTriggerCallback),
                Eq(iox_ListenerResult::ListenerResult_EVENT_ALREADY_ATTACHED));
}

TEST_F(iox_listener_test, AttachingSubscriberEventWorks)
{
    EXPECT_THAT(iox_listener_attach_subscriber_event(
                    &m_sut, &m_subscriber[0U], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
}

TEST_F(iox_listener_test, AttachingSubscriberTillListenerFullWorks)
{
    AttachAllSubscriber();
}

TEST_F(iox_listener_test, FullListenerReturnsLISTENER_FULLWhenAnotherSubscriberIsAttached)
{
    AttachAllSubscriber();
    EXPECT_THAT(iox_listener_attach_subscriber_event(&m_sut,
                                                     &m_subscriber[MAX_NUMBER_OF_EVENTS_PER_LISTENER],
                                                     iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED,
                                                     &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_LISTENER_FULL));
}

TEST_F(iox_listener_test, DetachingSubscriberTillListenerEmptyWorks)
{
    AttachAllSubscriber();
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        iox_listener_detach_subscriber_event(
            &m_sut, &m_subscriber[i], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED);
        EXPECT_THAT(iox_listener_size(&m_sut), Eq(MAX_NUMBER_OF_EVENTS_PER_LISTENER - i - 1U));
    }
}

TEST_F(iox_listener_test, AttachingSubscriberEventTwiceFailsWithEVENT_ALREADY_ATTACHED)
{
    EXPECT_THAT(iox_listener_attach_subscriber_event(
                    &m_sut, &m_subscriber[0U], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
    EXPECT_THAT(iox_listener_attach_subscriber_event(
                    &m_sut, &m_subscriber[0U], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_EVENT_ALREADY_ATTACHED));
}

TIMING_TEST_F(iox_listener_test, UserTriggerCallbackIsCalledWhenTriggered, Repeat(5), [&] {
    EXPECT_THAT(iox_listener_attach_user_trigger_event(&m_sut, m_userTrigger[0U], &userTriggerCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
    iox_user_trigger_trigger(m_userTrigger[0U]);
    std::this_thread::sleep_for(TIMEOUT);
    EXPECT_THAT(g_userTriggerCallbackArgument, Eq(m_userTrigger[0U]));
});

TIMING_TEST_F(iox_listener_test, UserTriggerCallbackWithContextDataIsCalledWhenTriggered, Repeat(5), [&] {
    int someContextData;
    EXPECT_THAT(iox_listener_attach_user_trigger_event_with_context_data(
                    &m_sut, m_userTrigger[0U], &userTriggerCallbackWithContextData, &someContextData),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));
    iox_user_trigger_trigger(m_userTrigger[0U]);
    std::this_thread::sleep_for(TIMEOUT);
    EXPECT_THAT(g_userTriggerCallbackArgument, Eq(m_userTrigger[0U]));
    EXPECT_THAT(g_contextData, Eq(static_cast<void*>(&someContextData)));
});

TIMING_TEST_F(iox_listener_test, SubscriberCallbackIsCalledSampleIsReceived, Repeat(5), [&] {
    EXPECT_THAT(iox_listener_attach_subscriber_event(
                    &m_sut, &m_subscriber[0U], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, &subscriberCallback),
                Eq(iox_ListenerResult::ListenerResult_SUCCESS));

    Subscribe(m_subscriber[0U]);
    constexpr uint32_t USER_PAYLOAD_SIZE{100U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    m_chunkPusher[0U].push(m_memoryManager.getChunk(chunkSettings));

    std::this_thread::sleep_for(TIMEOUT);
    EXPECT_THAT(g_subscriberCallbackArgument, Eq(&m_subscriber[0U]));
});

TIMING_TEST_F(iox_listener_test, SubscriberCallbackWithContextDataIsCalledSampleIsReceived, Repeat(5), [&] {
    int someContextData;
    EXPECT_THAT(
        iox_listener_attach_subscriber_event_with_context_data(&m_sut,
                                                               &m_subscriber[0U],
                                                               iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED,
                                                               &subscriberCallbackWithContextData,
                                                               &someContextData),
        Eq(iox_ListenerResult::ListenerResult_SUCCESS));

    Subscribe(m_subscriber[0U]);
    constexpr uint32_t USER_PAYLOAD_SIZE{100U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    m_chunkPusher[0U].push(m_memoryManager.getChunk(chunkSettings));

    std::this_thread::sleep_for(TIMEOUT);
    EXPECT_THAT(g_subscriberCallbackArgument, Eq(&m_subscriber[0U]));
    EXPECT_THAT(g_contextData, Eq(static_cast<void*>(&someContextData)));
});
