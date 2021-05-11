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

#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "mocks/wait_set_mock.hpp"

using namespace iox;
using namespace iox::popo;

extern "C" {
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/user_trigger.h"
#include "iceoryx_binding_c/wait_set.h"
}

#include "test.hpp"

#include <atomic>
#include <thread>

using namespace ::testing;

namespace
{
class iox_ws_test : public Test
{
  public:
    void SetUp() override
    {
        m_callbackOrigin = nullptr;
        m_contextData = nullptr;

        for (uint64_t i = 0U; i <= MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            m_portDataVector.emplace_back(TEST_SERVICE_DESCRIPTION,
                                          "someAppName",
                                          iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer,
                                          m_subscriberOptions);
            m_subscriberVector.emplace_back();
            m_subscriberVector[i].m_portData = &m_portDataVector[i];
        }

        for (uint64_t i = 0U; i <= MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            m_userTrigger.emplace_back(iox_user_trigger_init(&m_userTriggerStorage[i]));
        }
    }

    void TearDown() override
    {
        delete m_sut;

        for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            iox_user_trigger_deinit(m_userTrigger[i]);
        }
    }

    const iox::capro::ServiceDescription TEST_SERVICE_DESCRIPTION{"a", "b", "c"};
    iox::popo::SubscriberOptions m_subscriberOptions{MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, 0U};
    cxx::vector<iox::popo::SubscriberPortData, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1U> m_portDataVector;
    cxx::vector<cpp2c_Subscriber, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1U> m_subscriberVector;

    ConditionVariableData m_condVar{"Horscht"};
    WaitSetMock* m_sut = new WaitSetMock{m_condVar};

    iox_user_trigger_storage_t m_userTriggerStorage[MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1];
    cxx::vector<iox_user_trigger_t, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1> m_userTrigger;

    iox_notification_info_t m_eventInfoStorage[MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET];
    uint64_t m_missedElements = 0U;
    uint64_t m_numberOfTriggeredConditions = 0U;
    timespec m_timeout{0, 0};

    static void* m_callbackOrigin;
    static void* m_contextData;
};

void* iox_ws_test::m_callbackOrigin = nullptr;
void* iox_ws_test::m_contextData = nullptr;

void subscriberCallback(iox_sub_t subscriber)
{
    iox_ws_test::m_callbackOrigin = subscriber;
}

void subscriberCallbackWithContextData(iox_sub_t subscriber, void* const contextData)
{
    iox_ws_test::m_callbackOrigin = subscriber;
    iox_ws_test::m_contextData = contextData;
}

void userTriggerCallback(iox::popo::UserTrigger* userTrigger)
{
    iox_ws_test::m_callbackOrigin = userTrigger;
}

void userTriggerCallbackWithContextData(iox::popo::UserTrigger* userTrigger, void* const contextData)
{
    iox_ws_test::m_callbackOrigin = userTrigger;
    iox_ws_test::m_contextData = contextData;
}

} // namespace

TEST_F(iox_ws_test, InitWaitSetWithNullptrForStorageReturnsNullptr)
{
    EXPECT_EQ(iox_ws_init(nullptr), nullptr);
}

TEST_F(iox_ws_test, CapacityIsCorrect)
{
    EXPECT_EQ(iox_ws_capacity(m_sut), MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET);
}

TEST_F(iox_ws_test, SizeIsZeroWhenConstructed)
{
    EXPECT_EQ(iox_ws_size(m_sut), 0U);
}

TEST_F(iox_ws_test, SizeIsOneWhenOneUserTriggerIsAttached)
{
    EXPECT_EQ(iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL),
              iox_WaitSetResult::WaitSetResult_SUCCESS);
    EXPECT_EQ(iox_ws_size(m_sut), 1U);
}

TEST_F(iox_ws_test, SizeIsOneWhenOneSubscriberStateIsAttached)
{
    constexpr uint64_t customId = 123U;
    EXPECT_EQ(iox_ws_attach_subscriber_state(m_sut, &m_subscriberVector[0U], SubscriberState_HAS_DATA, customId, NULL),
              iox_WaitSetResult::WaitSetResult_SUCCESS);
    EXPECT_EQ(iox_ws_size(m_sut), 1U);
}

TEST_F(iox_ws_test, SizeIsOneWhenOneSubscriberEventIsAttached)
{
    constexpr uint64_t customId = 123U;
    EXPECT_EQ(
        iox_ws_attach_subscriber_event(m_sut, &m_subscriberVector[0U], SubscriberEvent_DATA_RECEIVED, customId, NULL),
        iox_WaitSetResult::WaitSetResult_SUCCESS);
    EXPECT_EQ(iox_ws_size(m_sut), 1U);
}

TEST_F(iox_ws_test, AttachingMoreUserTriggerThanCapacityAvailableFails)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        EXPECT_EQ(iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL),
                  iox_WaitSetResult::WaitSetResult_SUCCESS);
        EXPECT_EQ(iox_ws_size(m_sut), i + 1U);
    }
    EXPECT_EQ(iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET], 0U, NULL),
              iox_WaitSetResult::WaitSetResult_WAIT_SET_FULL);
}

TEST_F(iox_ws_test, AttachingMoreSubscriberStatesThanCapacityAvailableFails)
{
    constexpr uint64_t customId = 123U;
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        EXPECT_EQ(
            iox_ws_attach_subscriber_state(m_sut, &m_subscriberVector[i], SubscriberState_HAS_DATA, customId, NULL),
            iox_WaitSetResult::WaitSetResult_SUCCESS);
        EXPECT_EQ(iox_ws_size(m_sut), i + 1U);
    }
    EXPECT_EQ(iox_ws_attach_subscriber_state(m_sut,
                                             &m_subscriberVector[MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET],
                                             SubscriberState_HAS_DATA,
                                             customId,
                                             NULL),
              iox_WaitSetResult::WaitSetResult_WAIT_SET_FULL);
}

TEST_F(iox_ws_test, AttachingMoreSubscriberEventsThanCapacityAvailableFails)
{
    constexpr uint64_t customId = 123U;
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        EXPECT_EQ(iox_ws_attach_subscriber_event(
                      m_sut, &m_subscriberVector[i], SubscriberEvent_DATA_RECEIVED, customId, NULL),
                  iox_WaitSetResult::WaitSetResult_SUCCESS);
        EXPECT_EQ(iox_ws_size(m_sut), i + 1U);
    }
    EXPECT_EQ(iox_ws_attach_subscriber_event(m_sut,
                                             &m_subscriberVector[MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET],
                                             SubscriberEvent_DATA_RECEIVED,
                                             customId,
                                             NULL),
              iox_WaitSetResult::WaitSetResult_WAIT_SET_FULL);
}

TEST_F(iox_ws_test, SizeDecreasesWhenAttachedUserTriggerIsDeinitialized)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);
    iox_ws_detach_user_trigger_event(m_sut, m_userTrigger[0U]);
    EXPECT_EQ(iox_ws_size(m_sut), 0U);
}

TEST_F(iox_ws_test, SizeDecreasesWhenAttachedSubscriberStateIsDeinitialized)
{
    constexpr uint64_t customId = 123U;
    EXPECT_EQ(iox_ws_attach_subscriber_state(m_sut, &m_subscriberVector[0U], SubscriberState_HAS_DATA, customId, NULL),
              iox_WaitSetResult::WaitSetResult_SUCCESS);
    iox_ws_detach_subscriber_state(m_sut, &m_subscriberVector[0U], SubscriberState_HAS_DATA);
    EXPECT_EQ(iox_ws_size(m_sut), 0U);
}

TEST_F(iox_ws_test, SizeDecreasesWhenAttachedSubscriberEventIsDeinitialized)
{
    constexpr uint64_t customId = 123U;
    EXPECT_EQ(
        iox_ws_attach_subscriber_event(m_sut, &m_subscriberVector[0U], SubscriberEvent_DATA_RECEIVED, customId, NULL),
        iox_WaitSetResult::WaitSetResult_SUCCESS);
    iox_ws_detach_subscriber_event(m_sut, &m_subscriberVector[0U], SubscriberEvent_DATA_RECEIVED);
    EXPECT_EQ(iox_ws_size(m_sut), 0U);
}

TEST_F(iox_ws_test, NumberOfTriggeredConditionsIsOneWhenOneWasTriggered)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);
    iox_user_trigger_trigger(m_userTrigger[0U]);

    EXPECT_EQ(iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements), 1U);
}

TEST_F(iox_ws_test, NumberOfTriggeredConditionsIsCorrectWhenMultipleWereTriggered)
{
    for (uint64_t i = 0U; i < 10U; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    EXPECT_EQ(iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements), 10U);
}

TEST_F(iox_ws_test, NumberOfTriggeredConditionsIsCorrectWhenAllWereTriggered)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    EXPECT_EQ(iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements),
              MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET);
}

TEST_F(iox_ws_test, SingleTriggerCaseWaitReturnsCorrectTrigger)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0], 5678U, NULL);
    iox_user_trigger_trigger(m_userTrigger[0U]);

    iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    iox_notification_info_t eventInfo = m_eventInfoStorage[0U];

    EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 5678U);
    EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[0U]));
}

TEST_F(iox_ws_test, MultiTriggerCaseWaitReturnsCorrectTrigger)
{
    for (uint64_t i = 0U; i < 8; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 1337U + i, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    for (uint64_t i = 0U; i < 8; ++i)
    {
        iox_notification_info_t eventInfo = m_eventInfoStorage[i];
        EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 1337U + i);
        EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[i]));
    }
}

TEST_F(iox_ws_test, MaxTriggerCaseWaitReturnsCorrectTrigger)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 42U * i + 1U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_notification_info_t eventInfo = m_eventInfoStorage[i];
        EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 42U * i + 1U);
        EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[i]));
    }
}

TEST_F(iox_ws_test, TimedWaitNumberOfTriggeredConditionsIsOneWhenOneWasTriggered)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);
    iox_user_trigger_trigger(m_userTrigger[0U]);

    EXPECT_EQ(iox_ws_timed_wait(
                  m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements),
              1U);
}

TEST_F(iox_ws_test, TimedWaitNumberOfTriggeredConditionsIsCorrectWhenMultipleWereTriggered)
{
    for (uint64_t i = 0U; i < 10U; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    EXPECT_EQ(iox_ws_timed_wait(
                  m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements),
              10U);
}

TEST_F(iox_ws_test, TimedWaitNumberOfTriggeredConditionsIsCorrectWhenAllWereTriggered)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    EXPECT_EQ(iox_ws_timed_wait(
                  m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements),
              MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET);
}

TEST_F(iox_ws_test, SingleTriggerCaseTimedWaitReturnsCorrectTrigger)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 5678U, NULL);
    iox_user_trigger_trigger(m_userTrigger[0U]);

    iox_ws_timed_wait(m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    iox_notification_info_t eventInfo = m_eventInfoStorage[0U];

    EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 5678U);
    EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[0U]));
}

TEST_F(iox_ws_test, MultiTriggerCaseTimedWaitReturnsCorrectTrigger)
{
    for (uint64_t i = 0U; i < 8U; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 1337U + i, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_timed_wait(m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    for (uint64_t i = 0U; i < 8U; ++i)
    {
        iox_notification_info_t eventInfo = m_eventInfoStorage[i];
        EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 1337U + i);
        EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[i]));
    }
}

TEST_F(iox_ws_test, MaxTriggerCaseTimedWaitReturnsCorrectTrigger)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 42U * i + 1U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_timed_wait(m_sut, m_timeout, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_notification_info_t eventInfo = m_eventInfoStorage[i];
        EXPECT_EQ(iox_notification_info_get_notification_id(eventInfo), 42U * i + 1U);
        EXPECT_TRUE(iox_notification_info_does_originate_from_user_trigger(eventInfo, m_userTrigger[i]));
    }
}

TEST_F(iox_ws_test, MissedElementsIsZeroWhenNothingWasMissed)
{
    for (uint64_t i = 0U; i < 12U; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_wait(m_sut, m_eventInfoStorage, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET, &m_missedElements);

    EXPECT_EQ(m_missedElements, 0U);
}

TEST_F(iox_ws_test, MissedElementsIsCorrectWhenSomethingWasMissed)
{
    for (uint64_t i = 0U; i < 12U; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_wait(m_sut, m_eventInfoStorage, 8U, &m_missedElements);

    EXPECT_EQ(m_missedElements, 4U);
}

TEST_F(iox_ws_test, MissedElementsIsCorrectWhenAllWereMissed)
{
    for (uint64_t i = 0U; i < MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[i], 0U, NULL);
        iox_user_trigger_trigger(m_userTrigger[i]);
    }

    iox_ws_wait(m_sut, m_eventInfoStorage, 0U, &m_missedElements);

    EXPECT_EQ(m_missedElements, MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET);
}

TIMING_TEST_F(iox_ws_test, WaitIsBlockingTillTriggered, Repeat(5), [&] {
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);

    std::atomic_bool waitWasCalled{false};
    std::thread t([&] {
        iox_ws_wait(m_sut, NULL, 0U, &m_missedElements);
        waitWasCalled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TIMING_TEST_EXPECT_FALSE(waitWasCalled.load());

    iox_user_trigger_trigger(m_userTrigger[0U]);

    t.join();
    TIMING_TEST_EXPECT_TRUE(waitWasCalled.load());
});

TIMING_TEST_F(iox_ws_test, WaitIsNonBlockingAfterMarkForDestruction, Repeat(5), [&] {
    std::atomic_bool waitWasCalled{false};
    std::thread t([&] {
        iox_ws_wait(m_sut, NULL, 0U, &m_missedElements);
        iox_ws_wait(m_sut, NULL, 0U, &m_missedElements);
        iox_ws_wait(m_sut, NULL, 0U, &m_missedElements);
        waitWasCalled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TIMING_TEST_EXPECT_FALSE(waitWasCalled.load());

    iox_ws_mark_for_destruction(m_sut);

    t.join();
    TIMING_TEST_EXPECT_TRUE(waitWasCalled.load());
});


TIMING_TEST_F(iox_ws_test, TimedWaitIsBlockingTillTriggered, Repeat(5), [&] {
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);

    std::atomic_bool waitWasCalled{false};
    std::thread t([&] {
        iox_ws_timed_wait(m_sut, {1000, 1000}, NULL, 0U, &m_missedElements);
        waitWasCalled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TIMING_TEST_EXPECT_FALSE(waitWasCalled.load());

    iox_user_trigger_trigger(m_userTrigger[0U]);

    t.join();
    TIMING_TEST_EXPECT_TRUE(waitWasCalled.load());
});

TIMING_TEST_F(iox_ws_test, TimedWaitIsNonBlockingAfterMarkForDestruction, Repeat(5), [&] {
    std::atomic_bool waitWasCalled{false};
    std::thread t([&] {
        iox_ws_timed_wait(m_sut, {1000, 1000}, NULL, 0U, &m_missedElements);
        iox_ws_timed_wait(m_sut, {1000, 1000}, NULL, 0U, &m_missedElements);
        iox_ws_timed_wait(m_sut, {1000, 1000}, NULL, 0U, &m_missedElements);
        waitWasCalled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TIMING_TEST_EXPECT_FALSE(waitWasCalled.load());

    iox_ws_mark_for_destruction(m_sut);

    t.join();
    TIMING_TEST_EXPECT_TRUE(waitWasCalled.load());
});

TIMING_TEST_F(iox_ws_test, TimedWaitBlocksTillTimeout, Repeat(5), [&] {
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0U], 0U, NULL);

    std::atomic_bool waitWasCalled{false};
    std::thread t([&] {
        constexpr long hundredMsInNanoSeconds = 100000000L;
        iox_ws_timed_wait(m_sut, {0, hundredMsInNanoSeconds}, NULL, 0U, &m_missedElements);
        waitWasCalled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    TIMING_TEST_EXPECT_FALSE(waitWasCalled.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    TIMING_TEST_EXPECT_TRUE(waitWasCalled.load());

    t.join();
});

TEST_F(iox_ws_test, SubscriberEventCallbackIsCalled)
{
    iox_ws_attach_subscriber_event(
        m_sut, &m_subscriberVector[0], iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED, 0, &subscriberCallback);

    m_subscriberVector[0].m_trigger.trigger();
    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(&m_subscriberVector[0]));
}

TEST_F(iox_ws_test, SubscriberEventCallbackWithContextDataIsCalled)
{
    uint64_t someContextData = 0U;
    iox_ws_attach_subscriber_event_with_context_data(m_sut,
                                                     &m_subscriberVector[0],
                                                     iox_SubscriberEvent::SubscriberEvent_DATA_RECEIVED,
                                                     0,
                                                     &subscriberCallbackWithContextData,
                                                     &someContextData);

    m_subscriberVector[0].m_trigger.trigger();
    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(&m_subscriberVector[0]));
    EXPECT_THAT(m_contextData, Eq(&someContextData));
}

TEST_F(iox_ws_test, SubscriberStateCallbackIsCalled)
{
    iox_ws_attach_subscriber_state(
        m_sut, &m_subscriberVector[0], iox_SubscriberState::SubscriberState_HAS_DATA, 0, &subscriberCallback);

    m_subscriberVector[0].m_portData->m_chunkReceiverData.m_queue.push(iox::mepoo::ShmSafeUnmanagedChunk());
    m_subscriberVector[0].m_trigger.trigger();
    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(&m_subscriberVector[0]));
}

TEST_F(iox_ws_test, SubscriberStateCallbackWithContextDataIsCalled)
{
    uint64_t someContextData = 0U;
    iox_ws_attach_subscriber_state_with_context_data(m_sut,
                                                     &m_subscriberVector[0],
                                                     iox_SubscriberState::SubscriberState_HAS_DATA,
                                                     0,
                                                     &subscriberCallbackWithContextData,
                                                     &someContextData);

    m_subscriberVector[0].m_portData->m_chunkReceiverData.m_queue.push(iox::mepoo::ShmSafeUnmanagedChunk());
    m_subscriberVector[0].m_trigger.trigger();
    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(&m_subscriberVector[0]));
    EXPECT_THAT(m_contextData, Eq(&someContextData));
}


TEST_F(iox_ws_test, UserTriggerCallbackIsCalled)
{
    iox_ws_attach_user_trigger_event(m_sut, m_userTrigger[0], 0, &userTriggerCallback);
    iox_user_trigger_trigger(m_userTrigger[0]);

    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(m_userTrigger[0]));
}

TEST_F(iox_ws_test, UserTriggerCallbackWithContextDataIsCalled)
{
    uint64_t someContextData = 0U;
    iox_ws_attach_user_trigger_event_with_context_data(
        m_sut, m_userTrigger[0], 0, &userTriggerCallbackWithContextData, &someContextData);
    iox_user_trigger_trigger(m_userTrigger[0]);

    ASSERT_THAT(iox_ws_wait(m_sut, m_eventInfoStorage, 1U, &m_missedElements), Eq(1U));
    EXPECT_EQ(m_missedElements, 0U);

    iox_notification_info_call(m_eventInfoStorage[0]);

    EXPECT_THAT(m_callbackOrigin, Eq(m_userTrigger[0]));
    EXPECT_THAT(m_contextData, Eq(&someContextData));
}
