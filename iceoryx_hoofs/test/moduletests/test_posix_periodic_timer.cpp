// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_hoofs/error_handling/error_handling.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"
#include "iceoryx_hoofs/posix_wrapper/periodic_timer.hpp"
#include "iceoryx_hoofs/testing/timing_test.hpp"
#include "test.hpp"

#include <chrono>
#include <thread>

namespace
{
using namespace ::testing;
using namespace iox::units;
using namespace iox::units::duration_literals;

using Timer = iox::posix::PeriodicTimer;

class PeriodicTimer_test : public Test
{
  public:
    virtual void SetUp()
    {
        numberOfCalls = 0;
    }

    virtual void TearDown()
    {
    }

    Duration second{1_s};

    std::atomic<int> numberOfCalls{0};
    static const Duration INTERVAL;
};

const Duration PeriodicTimer_test::INTERVAL{50_ms};

TEST_F(PeriodicTimer_test, TimerAutoStartTest)
{
    Timer sut(0_s);

    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);

    ASSERT_FALSE(timerState.has_error());
}

TEST_F(PeriodicTimer_test, ZeroINTERVALTest)
{
    Timer sut(0_s);

    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);

    ASSERT_FALSE(timerState.has_error());
    EXPECT_EQ(timerState.value().state, iox::posix::TimerState::TICK);
}

TIMING_TEST_F(PeriodicTimer_test, DurationINTERVALTest, Repeat(5), [&] {
    Timer sut(INTERVAL);

    auto timeBeforeWait = sut.now();
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    auto timeAfterWait = sut.now();

    ASSERT_FALSE(timeBeforeWait.has_error());
    ASSERT_FALSE(timerState.has_error());
    ASSERT_FALSE(timeAfterWait.has_error());

    auto duration = timeAfterWait.value() - timeBeforeWait.value();

    TIMING_TEST_EXPECT_TRUE(duration.toMilliseconds()  ==  INTERVAL.toMilliseconds());           
});

TEST_F(PeriodicTimer_test, TimerStopTest)
{
    Timer sut(0_s);

    sut.stop();
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);

    ASSERT_FALSE(timerState.has_error());
    EXPECT_EQ(timerState.value().state, iox::posix::TimerState::STOP);
}

TEST_F(PeriodicTimer_test, TimerStopAfterWaitTest)
{
    Timer sut(INTERVAL);

    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    sut.stop();
    timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);

    ASSERT_FALSE(timerState.has_error());
    EXPECT_EQ(timerState.value().state, iox::posix::TimerState::STOP);
}

TIMING_TEST_F(PeriodicTimer_test, ResetWithNewDurationINTERVALTest, Repeat(5), [&] {
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    iox::units::Duration NEW_DURATION{100_ms};
    Timer sut(INTERVAL);
    
    sut.start(NEW_DURATION);
    auto timeBeforeWait = sut.now();
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    auto timeAfterWait = sut.now();

    ASSERT_FALSE(timeBeforeWait.has_error());
    ASSERT_FALSE(timerState.has_error());
    ASSERT_FALSE(timeAfterWait.has_error());

    auto duration = timeAfterWait.value() - timeBeforeWait.value();

    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_TRUE(duration.toMilliseconds() - NEW_DURATION.toMilliseconds() <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, currentTimeTest, Repeat(5), [&] {
    Timer sut(INTERVAL);
    struct timespec ts;

    TIMING_TEST_EXPECT_TRUE(clock_gettime(CLOCK_REALTIME, &ts) == 0);
    
    auto timeNow = sut.now();
    auto currentSystemTime = iox::units::Duration(ts);

    TIMING_TEST_EXPECT_FALSE(timeNow.has_error());
    TIMING_TEST_EXPECT_TRUE(timeNow.value().toMilliseconds() == currentSystemTime.toMilliseconds());
});

TIMING_TEST_F(PeriodicTimer_test, periodicityWithoutExecutionTimeTest, Repeat(5), [&] {
    Timer sut(INTERVAL);
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    auto timeUntilNextActivation = sut.now();

    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    auto currentTime = sut.now();

    TIMING_TEST_EXPECT_FALSE(timeUntilNextActivation.has_error());
    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_FALSE(currentTime.has_error());
    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);
    TIMING_TEST_EXPECT_TRUE(currentTime.value().toMilliseconds() - (timeUntilNextActivation.value() + INTERVAL).toMilliseconds()  <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, periodicityExecutionTimeLessThanActivationTimeTest, Repeat(5), [&] {
    constexpr int EXECUTIONTIME = 30;
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    Timer sut(INTERVAL);
    
    auto timeBeforeActivation = sut.now();
    std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTIONTIME));
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    auto currentTime = sut.now();

    TIMING_TEST_EXPECT_FALSE(timeBeforeActivation.has_error());
    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_FALSE(currentTime.has_error());
    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);

    auto expectedTimeOfActivation = timeBeforeActivation.value() + INTERVAL;
    TIMING_TEST_EXPECT_TRUE(currentTime.value().toMilliseconds() - expectedTimeOfActivation.toMilliseconds()  <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, immediateCatchupPolicyTest, Repeat(5), [&] {
    constexpr int EXECUTIONTIME = 70;
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    Timer sut(INTERVAL);

    std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTIONTIME));
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);

    auto currentTimeAfterExecution = sut.now();
    timerState = sut.wait(iox::posix::TimerCatchupPolicy::IMMEDIATE_TICK);
    auto currentTimeAfterWait = sut.now();

    TIMING_TEST_EXPECT_FALSE(currentTimeAfterExecution.has_error());
    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_FALSE(currentTimeAfterWait.has_error());

    auto remainingTimeForNextActivation = currentTimeAfterWait.value() - currentTimeAfterExecution.value();

    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);
    TIMING_TEST_EXPECT_TRUE(remainingTimeForNextActivation.toMilliseconds() <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, skipToNextTickCatchupPolicyWithLessDelayTest, Repeat(5), [&] {
    constexpr int EXECUTIONTIME = 70;
    constexpr int TIME_SLOT_CONSUMED = 2;
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    Timer sut(INTERVAL);
    

    auto timeBeforeActivation = sut.now();
    std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTIONTIME));
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::SKIP_TO_NEXT_TICK);
    auto timeAfterExecution = sut.now();

    TIMING_TEST_EXPECT_FALSE(timeBeforeActivation.has_error());
    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_FALSE(timeAfterExecution.has_error());

    auto timeBetweenActivation = timeAfterExecution.value() - timeBeforeActivation.value();
    auto diffInActivationTime = timeBetweenActivation.toMilliseconds() - (INTERVAL.toMilliseconds() * TIME_SLOT_CONSUMED);

    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);
    TIMING_TEST_EXPECT_TRUE(diffInActivationTime <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, skipToNextTickCatchupPolicyWithLargeDelayTest, Repeat(5), [&] {
    constexpr int EXECUTIONTIME = 150;
    constexpr int TIME_SLOT_CONSUMED = 3;
    constexpr int RANGE_APPROX = 2; // 2ms approximation. This may be lost in execution.
    Timer sut(INTERVAL);
    

    auto timeBeforeActivation = sut.now();
    std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTIONTIME));
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::SKIP_TO_NEXT_TICK);
    auto timeAfterExecution = sut.now();

    TIMING_TEST_EXPECT_FALSE(timeBeforeActivation.has_error());
    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_FALSE(timeAfterExecution.has_error());

    auto timeBetweenActivation = timeAfterExecution.value() - timeBeforeActivation.value();
    auto diffInActivationTime = timeBetweenActivation.toMilliseconds() - (INTERVAL.toMilliseconds() * TIME_SLOT_CONSUMED);
    
    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::TICK);
    TIMING_TEST_EXPECT_TRUE(diffInActivationTime <= RANGE_APPROX);
});

TIMING_TEST_F(PeriodicTimer_test, errorCatchupPolicyTest, Repeat(5), [&] {
    constexpr int EXECUTIONTIME = 70;
    Timer sut(INTERVAL);

    std::this_thread::sleep_for(std::chrono::milliseconds(EXECUTIONTIME));
    auto timerState = sut.wait(iox::posix::TimerCatchupPolicy::HOLD_ON_DELAY);
    auto delayExpected = EXECUTIONTIME - INTERVAL.toMilliseconds();

    TIMING_TEST_EXPECT_FALSE(timerState.has_error());
    TIMING_TEST_EXPECT_TRUE(timerState.value().state == iox::posix::TimerState::DELAY);
    TIMING_TEST_EXPECT_TRUE(delayExpected == timerState.value().timeDelay.toMilliseconds());
});

} //namespace