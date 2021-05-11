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

#include "iceoryx_posh/internal/popo/building_blocks/condition_listener.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"
#include "test.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>

using namespace ::testing;
using ::testing::Return;
using namespace iox::popo;
using namespace iox::cxx;
using namespace iox::units::duration_literals;

class ConditionVariable_test : public Test
{
  public:
    using NotificationVector_t = ConditionListener::NotificationVector_t;
    using Type_t = iox::cxx::BestFittingType_t<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER>;
    const iox::RuntimeName_t m_runtimeName{"Ferdinand"};
    const iox::units::Duration m_timeToWait = 2_s;
    const iox::units::Duration m_timingTestTime = 100_ms;

    ConditionVariableData m_condVarData{m_runtimeName};
    ConditionListener m_waiter{m_condVarData};
    ConditionNotifier m_signaler{m_condVarData, 0U};
    vector<ConditionNotifier, iox::MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE> m_notifiers;

    void SetUp() override
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE; ++i)
        {
            m_notifiers.emplace_back(m_condVarData, i);
        }
        m_watchdog.watchAndActOnFailure([&] { std::terminate(); });
    }

    Watchdog m_watchdog{m_timeToWait};
    iox::posix::Semaphore m_syncSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();
};

TEST_F(ConditionVariable_test, ConditionListenerIsNeitherCopyNorMovable)
{
    EXPECT_FALSE(std::is_copy_constructible<ConditionListener>::value);
    EXPECT_FALSE(std::is_move_constructible<ConditionListener>::value);
    EXPECT_FALSE(std::is_copy_assignable<ConditionListener>::value);
    EXPECT_FALSE(std::is_move_assignable<ConditionListener>::value);
}

TEST_F(ConditionVariable_test, ConditionNotifierIsNeitherCopyNorMovable)
{
    EXPECT_FALSE(std::is_copy_constructible<ConditionNotifier>::value);
    EXPECT_FALSE(std::is_move_constructible<ConditionNotifier>::value);
    EXPECT_FALSE(std::is_copy_assignable<ConditionNotifier>::value);
    EXPECT_FALSE(std::is_move_assignable<ConditionNotifier>::value);
}

TEST_F(ConditionVariable_test, NotifyOnceResultsInBeingTriggered)
{
    m_signaler.notify();
    EXPECT_TRUE(m_waiter.wasNotified());
}

TEST_F(ConditionVariable_test, NoNotifyResultsInNotBeingTriggered)
{
    EXPECT_FALSE(m_waiter.wasNotified());
}

TEST_F(ConditionVariable_test, WaitResetsAllNotificationsInWait)
{
    m_signaler.notify();
    m_signaler.notify();
    m_signaler.notify();
    m_waiter.wait();

    std::atomic_bool isThreadFinished{false};
    std::thread t([&] {
        m_waiter.wait();
        isThreadFinished = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(isThreadFinished.load());
    m_signaler.notify();
    t.join();
    EXPECT_TRUE(isThreadFinished.load());
}

TEST_F(ConditionVariable_test, WaitAndNotifyResultsInImmediateTriggerMultiThreaded)
{
    std::atomic<int> counter{0};
    std::thread waiter([&] {
        EXPECT_THAT(counter, Eq(0));
        IOX_DISCARD_RESULT(m_syncSemaphore.post());
        m_waiter.wait();
        EXPECT_THAT(counter, Eq(1));
    });
    IOX_DISCARD_RESULT(m_syncSemaphore.wait());
    counter++;
    m_signaler.notify();
    waiter.join();
}

TEST_F(ConditionVariable_test, AllNotificationsAreFalseAfterConstruction)
{
    ConditionVariableData sut;
    for (auto& notification : sut.m_activeNotifications)
    {
        EXPECT_THAT(notification, Eq(false));
    }
}

TEST_F(ConditionVariable_test, CorrectRuntimeNameAfterConstructionWithRuntimeName)
{
    EXPECT_THAT(m_condVarData.m_runtimeName.c_str(), StrEq(m_runtimeName));
}

TEST_F(ConditionVariable_test, AllNotificationsAreFalseAfterConstructionWithRuntimeName)
{
    for (auto& notification : m_condVarData.m_activeNotifications)
    {
        EXPECT_THAT(notification, Eq(false));
    }
}

TEST_F(ConditionVariable_test, NotifyActivatesCorrectIndex)
{
    constexpr Type_t EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U;
    ConditionNotifier sut(m_condVarData, EVENT_INDEX);
    sut.notify();
    for (Type_t i = 0U; i < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; i++)
    {
        if (i == EVENT_INDEX)
        {
            EXPECT_THAT(m_condVarData.m_activeNotifications[i], Eq(true));
        }
        else
        {
            EXPECT_THAT(m_condVarData.m_activeNotifications[i], Eq(false));
        }
    }
}

TEST_F(ConditionVariable_test, TimedWaitWithZeroTimeoutWorks)
{
    ConditionListener sut(m_condVarData);
    EXPECT_TRUE(sut.timedWait(iox::units::Duration::fromSeconds(0)).empty());
}

TEST_F(ConditionVariable_test, TimedWaitWithoutNotificationReturnsEmptyVector)
{
    ConditionListener sut(m_condVarData);
    EXPECT_TRUE(sut.timedWait(iox::units::Duration::fromMilliseconds(100)).empty());
}

TEST_F(ConditionVariable_test, TimedWaitReturnsOneNotifiedIndex)
{
    ConditionListener sut(m_condVarData);
    ConditionNotifier(m_condVarData, 6U).notify();

    auto indices = sut.timedWait(iox::units::Duration::fromMilliseconds(100));

    ASSERT_THAT(indices.size(), Eq(1U));
    EXPECT_THAT(indices[0U], Eq(6U));
}

TEST_F(ConditionVariable_test, TimedWaitReturnsMultipleNotifiedIndices)
{
    ConditionListener sut(m_condVarData);
    ConditionNotifier(m_condVarData, 5U).notify();
    ConditionNotifier(m_condVarData, 15U).notify();

    auto indices = sut.timedWait(iox::units::Duration::fromMilliseconds(100));

    ASSERT_THAT(indices.size(), Eq(2U));
    EXPECT_THAT(indices[0U], Eq(5U));
    EXPECT_THAT(indices[1U], Eq(15U));
}

TEST_F(ConditionVariable_test, TimedWaitReturnsAllNotifiedIndices)
{
    ConditionListener sut(m_condVarData);
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE; ++i)
    {
        ConditionNotifier(m_condVarData, i).notify();
    }

    auto indices = sut.timedWait(iox::units::Duration::fromMilliseconds(100));

    ASSERT_THAT(indices.size(), Eq(iox::MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE));
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE; ++i)
    {
        EXPECT_THAT(indices[i], Eq(i));
    }
}

TIMING_TEST_F(ConditionVariable_test, TimedWaitBlocksUntilTimeout, Repeat(5), [&] {
    ConditionListener listener(m_condVarData);
    NotificationVector_t activeNotifications;
    iox::posix::Semaphore threadSetupSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();
    std::atomic_bool hasWaited{false};

    std::thread waiter([&] {
        activeNotifications = listener.timedWait(m_timingTestTime);
        hasWaited.store(true, std::memory_order_relaxed);
        ASSERT_THAT(activeNotifications.size(), Eq(0U));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * m_timingTestTime.toMilliseconds() / 3));
    EXPECT_THAT(hasWaited, Eq(false));
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * m_timingTestTime.toMilliseconds() / 3));
    EXPECT_THAT(hasWaited, Eq(true));
    waiter.join();
})

TIMING_TEST_F(ConditionVariable_test, TimedWaitBlocksUntilNotification, Repeat(5), [&] {
    ConditionListener listener(m_condVarData);
    NotificationVector_t activeNotifications;
    iox::posix::Semaphore threadSetupSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();
    std::atomic_bool hasWaited{false};

    std::thread waiter([&] {
        activeNotifications = listener.timedWait(m_timingTestTime);
        hasWaited.store(true, std::memory_order_relaxed);
        ASSERT_THAT(activeNotifications.size(), Eq(1U));
        EXPECT_THAT(activeNotifications[0], 13U);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(m_timingTestTime.toMilliseconds() / 4));
    EXPECT_THAT(hasWaited, Eq(false));
    ConditionNotifier(m_condVarData, 13U).notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(m_timingTestTime.toMilliseconds() / 4));
    EXPECT_THAT(hasWaited, Eq(true));
    waiter.join();
})

TEST_F(ConditionVariable_test, WaitIsNonBlockingAfterDestroyAndReturnsEmptyVector)
{
    ConditionListener sut(m_condVarData);

    sut.destroy();
    const auto& activeNotifications = sut.wait();

    EXPECT_THAT(activeNotifications.size(), Eq(0U));
}

TEST_F(ConditionVariable_test, WaitIsNonBlockingAfterDestroyAndNotifyAndReturnsEmptyVector)
{
    ConditionListener sut(m_condVarData);
    sut.destroy();

    ConditionNotifier notifier(m_condVarData, 0U);
    notifier.notify();

    const auto& activeNotifications = sut.wait();
    EXPECT_THAT(activeNotifications.size(), Eq(0U));
}

TEST_F(ConditionVariable_test, DestroyWakesUpWaitWhichReturnsEmptyVector)
{
    ConditionListener sut(m_condVarData);

    NotificationVector_t activeNotifications;

    std::thread waiter([&] {
        activeNotifications = sut.wait();
        EXPECT_THAT(activeNotifications.size(), Eq(0U));
    });

    sut.destroy();
    waiter.join();
}

TEST_F(ConditionVariable_test, GetCorrectNotificationVectorAfterNotifyAndWait)
{
    constexpr Type_t EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U;
    ConditionNotifier notifier(m_condVarData, EVENT_INDEX);
    ConditionListener listener(m_condVarData);

    notifier.notify();
    const auto& activeNotifications = listener.wait();

    ASSERT_THAT(activeNotifications.size(), Eq(1U));
    EXPECT_THAT(activeNotifications[0], Eq(EVENT_INDEX));
}

TEST_F(ConditionVariable_test, GetCorrectNotificationVectorAfterMultipleNotifyAndWait)
{
    constexpr Type_t FIRST_EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U;
    constexpr Type_t SECOND_EVENT_INDEX = 0U;
    ConditionNotifier notifier1(m_condVarData, FIRST_EVENT_INDEX);
    ConditionNotifier notifier2(m_condVarData, SECOND_EVENT_INDEX);
    ConditionListener listener(m_condVarData);

    notifier1.notify();
    notifier2.notify();
    const auto& activeNotifications = listener.wait();

    ASSERT_THAT(activeNotifications.size(), Eq(2U));
    EXPECT_THAT(activeNotifications[0], Eq(SECOND_EVENT_INDEX));
    EXPECT_THAT(activeNotifications[1], Eq(FIRST_EVENT_INDEX));
}

TEST_F(ConditionVariable_test, WaitAndNotifyResultsInCorrectNotificationVector)
{
    constexpr Type_t EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 5U;
    ConditionNotifier notifier(m_condVarData, EVENT_INDEX);
    ConditionListener listener(m_condVarData);
    NotificationVector_t activeNotifications;

    std::thread waiter([&] {
        activeNotifications = listener.wait();
        ASSERT_THAT(activeNotifications.size(), Eq(1U));
        EXPECT_THAT(activeNotifications[0], Eq(EVENT_INDEX));
    });

    notifier.notify();
    waiter.join();
}

TIMING_TEST_F(ConditionVariable_test, WaitBlocks, Repeat(5), [&] {
    constexpr Type_t EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 5U;
    ConditionNotifier notifier(m_condVarData, EVENT_INDEX);
    ConditionListener listener(m_condVarData);
    NotificationVector_t activeNotifications;
    iox::posix::Semaphore threadSetupSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();
    std::atomic_bool hasWaited{false};

    std::thread waiter([&] {
        IOX_DISCARD_RESULT(threadSetupSemaphore.post());
        activeNotifications = listener.wait();
        hasWaited.store(true, std::memory_order_relaxed);
        ASSERT_THAT(activeNotifications.size(), Eq(1U));
        EXPECT_THAT(activeNotifications[0], Eq(EVENT_INDEX));
    });

    IOX_DISCARD_RESULT(threadSetupSemaphore.wait());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THAT(hasWaited, Eq(false));
    notifier.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THAT(hasWaited, Eq(true));
    waiter.join();
})

TIMING_TEST_F(ConditionVariable_test, SecondWaitBlocksUntilNewNotification, Repeat(5), [&] {
    constexpr Type_t FIRST_EVENT_INDEX = iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 2U;
    constexpr Type_t SECOND_EVENT_INDEX = 0U;
    ConditionNotifier notifier1(m_condVarData, FIRST_EVENT_INDEX);
    ConditionNotifier notifier2(m_condVarData, SECOND_EVENT_INDEX);
    ConditionListener listener(m_condVarData);
    iox::posix::Semaphore threadSetupSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();
    std::atomic_bool hasWaited{false};

    Watchdog watchdogFirstWait(m_timeToWait);
    watchdogFirstWait.watchAndActOnFailure([&] { listener.destroy(); });

    notifier1.notify();
    notifier2.notify();
    NotificationVector_t activeNotifications = listener.wait();

    ASSERT_THAT(activeNotifications.size(), Eq(2U));
    EXPECT_THAT(activeNotifications[0], Eq(SECOND_EVENT_INDEX));
    EXPECT_THAT(activeNotifications[1], Eq(FIRST_EVENT_INDEX));

    Watchdog watchdogSecondWait(m_timeToWait);
    watchdogSecondWait.watchAndActOnFailure([&] { listener.destroy(); });

    std::thread waiter([&] {
        IOX_DISCARD_RESULT(threadSetupSemaphore.post());
        activeNotifications = listener.wait();
        hasWaited.store(true, std::memory_order_relaxed);
        ASSERT_THAT(activeNotifications.size(), Eq(1U));
        EXPECT_THAT(activeNotifications[0], Eq(FIRST_EVENT_INDEX));
        for (const auto& notification : m_condVarData.m_activeNotifications)
        {
            EXPECT_THAT(notification, Eq(false));
        }
    });

    IOX_DISCARD_RESULT(threadSetupSemaphore.wait());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THAT(hasWaited, Eq(false));
    notifier1.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THAT(hasWaited, Eq(true));
    waiter.join();
})

void waitReturnsSortedListWhenTriggeredInOrder(ConditionVariable_test& test,
                                               const function_ref<ConditionListener::NotificationVector_t()>& wait)
{
    for (uint64_t i = 0U; i < test.m_notifiers.size(); ++i)
    {
        test.m_notifiers[i].notify();
    }

    auto notifications = wait();
    ASSERT_THAT(notifications.size(), Eq(test.m_notifiers.size()));
    for (uint64_t i = 0U; i < test.m_notifiers.size(); ++i)
    {
        EXPECT_THAT(notifications[i], Eq(i));
    }
}

TEST_F(ConditionVariable_test, WaitReturnsSortedListWhenTriggeredInOrder)
{
    waitReturnsSortedListWhenTriggeredInOrder(*this, [this] { return m_waiter.wait(); });
}

TEST_F(ConditionVariable_test, TimedWaitReturnsSortedListWhenTriggeredInOrder)
{
    waitReturnsSortedListWhenTriggeredInOrder(
        *this, [this] { return m_waiter.timedWait(iox::units::Duration::fromSeconds(1)); });
}

void waitReturnsSortedListWhenTriggeredInReverseOrder(
    ConditionVariable_test& test, const function_ref<ConditionListener::NotificationVector_t()>& wait)
{
    for (uint64_t i = 0U; i < test.m_notifiers.size(); ++i)
    {
        test.m_notifiers[test.m_notifiers.size() - i - 1U].notify();
    }

    auto notifications = wait();
    ASSERT_THAT(notifications.size(), Eq(test.m_notifiers.size()));
    for (uint64_t i = 0U; i < test.m_notifiers.size(); ++i)
    {
        EXPECT_THAT(notifications[i], Eq(i));
    }
}

TEST_F(ConditionVariable_test, WaitReturnsSortedListWhenTriggeredInReverseOrder)
{
    waitReturnsSortedListWhenTriggeredInReverseOrder(*this, [this] { return m_waiter.wait(); });
}

TEST_F(ConditionVariable_test, TimedWaitReturnsSortedListWhenTriggeredInReverseOrder)
{
    waitReturnsSortedListWhenTriggeredInReverseOrder(
        *this, [this] { return m_waiter.timedWait(iox::units::Duration::fromSeconds(1)); });
}
