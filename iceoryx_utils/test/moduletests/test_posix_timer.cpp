// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_utils/posix_wrapper/timer.hpp"
#include "iceoryx_utils/testing/test.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
using namespace ::testing;

using namespace iox::units;
using namespace iox::units::duration_literals;

using Timer = iox::posix::Timer;
using TimerError = iox::posix::TimerError;

class Timer_test : public Test
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
    static const Duration TIMEOUT;
};

class TimerStopWatch_test : public Test
{
  public:
    static const Duration TIMEOUT;
};
const Duration TimerStopWatch_test::TIMEOUT{10_ms};
const Duration Timer_test::TIMEOUT{TimerStopWatch_test::TIMEOUT};

TEST_F(TimerStopWatch_test, DurationOfZeroCausesError)
{
    Timer sut(0_s);
    EXPECT_THAT(sut.hasError(), Eq(true));
    EXPECT_THAT(sut.getError(), Eq(TimerError::TIMEOUT_IS_ZERO));
}

TEST_F(Timer_test, ZeroTimeoutIsNotAllowed)
{
    Timer sut(0_s, [] {});

    EXPECT_THAT(sut.hasError(), Eq(true));
    EXPECT_THAT(sut.getError(), Eq(iox::posix::TimerError::TIMEOUT_IS_ZERO));
}

TIMING_TEST_F(Timer_test, CallbackNotExecutedWhenNotStarted, Repeat(5), [&] {
    bool callbackExecuted{false};
    Timer sut(TIMEOUT, [&] { callbackExecuted = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));

    TIMING_TEST_EXPECT_ALWAYS_FALSE(callbackExecuted);
});

TIMING_TEST_F(Timer_test, CallbackExecutedOnceAfterStart, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(1_ns, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    TIMING_TEST_EXPECT_TRUE(counter.load() == 1);
});

TIMING_TEST_F(Timer_test, CallbackExecutedPeriodicallyAfterStart, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));
    auto finalCount = counter.load();

    TIMING_TEST_EXPECT_TRUE(6 <= finalCount && finalCount <= 11);
});

TIMING_TEST_F(Timer_test, PeriodicCallbackNotExecutedPrematurely, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    TIMING_TEST_EXPECT_TRUE(counter.load() == 0);
});

TIMING_TEST_F(Timer_test, OneTimeCallbackNotExecutedPrematurely, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    TIMING_TEST_EXPECT_TRUE(counter.load() == 0);
});

TEST_F(Timer_test, StartFailsWhenNoCallbackIsSet)
{
    Timer sut(1_ms);
    auto call = sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT);

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMER_NOT_INITIALIZED));
}

TIMING_TEST_F(Timer_test, StartRunModeOnceIsStoppedAfterStop, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    ASSERT_FALSE(sut.stop().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));

    TIMING_TEST_EXPECT_TRUE(counter.load() == 0);
});

TIMING_TEST_F(Timer_test, StartRunPeriodicOnceIsStoppedAfterStop, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    ASSERT_FALSE(sut.stop().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));

    TIMING_TEST_EXPECT_TRUE(counter.load() == 0);
});

TIMING_TEST_F(Timer_test, StartRunPeriodicOnceIsStoppedInTheMiddleAfterStop, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));
    ASSERT_FALSE(sut.stop().has_error());
    auto previousCount = counter.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));

    TIMING_TEST_EXPECT_TRUE(previousCount == counter.load());
});

TEST_F(Timer_test, StopFailsWhenNoCallbackIsSet)
{
    Timer sut(1_ms);
    auto call = sut.stop();

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMER_NOT_INITIALIZED));
}

TIMING_TEST_F(Timer_test, RestartWithDifferentTiming, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT * 10, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(20 * TIMEOUT.toMilliseconds()));
    ASSERT_FALSE(sut.restart(TIMEOUT, Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    counter = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));
    auto finalCount = counter.load();

    TIMING_TEST_EXPECT_TRUE(6 <= finalCount && finalCount <= 13);
});

TIMING_TEST_F(Timer_test, RestartWithDifferentRunMode, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * TIMEOUT.toMilliseconds() / 3));
    ASSERT_FALSE(sut.restart(TIMEOUT, Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    counter = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    TIMING_TEST_EXPECT_TRUE(counter.load() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    TIMING_TEST_EXPECT_TRUE(counter.load() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    TIMING_TEST_EXPECT_TRUE(counter.load() == 1);
});

TIMING_TEST_F(Timer_test, RestartWithDifferentTimingAndRunMode, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT * 2, [&] { counter++; });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(5 * TIMEOUT.toMilliseconds()));
    counter = 0;
    ASSERT_FALSE(sut.restart(TIMEOUT, Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));

    auto finalCount = counter.load();
    TIMING_TEST_EXPECT_TRUE(6 <= finalCount && finalCount <= 13);
});

TEST_F(Timer_test, RestartWithEmptyCallbackFails)
{
    Timer sut(1_ms);
    auto call = sut.restart(1_s, Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT);

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMER_NOT_INITIALIZED));
}

TEST_F(Timer_test, RestartWithTimeoutOfZeroFails)
{
    Timer sut(1_ms, [] {});
    auto call = sut.restart(0_s, Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT);

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMEOUT_IS_ZERO));
}

TEST_F(Timer_test, TimeUntilExpirationFailsWithoutCallback)
{
    Timer sut(1_ms);
    auto call = sut.timeUntilExpiration();

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMER_NOT_INITIALIZED));
}

TIMING_TEST_F(Timer_test, TimeUntilExpirationWithCallback, Repeat(5), [&] {
    Timer sut(TIMEOUT, [] {});
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    auto timeUntilExpiration = sut.timeUntilExpiration().value().toMilliseconds();
    TIMING_TEST_EXPECT_TRUE(timeUntilExpiration > 2 * TIMEOUT.toMilliseconds() / 3);

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT.toMilliseconds() / 3));
    timeUntilExpiration = sut.timeUntilExpiration().value().toMilliseconds();
    TIMING_TEST_EXPECT_TRUE(1 <= timeUntilExpiration && timeUntilExpiration <= TIMEOUT.toMilliseconds() / 3);
});

TIMING_TEST_F(Timer_test, TimeUntilExpirationZeroAfterCallbackOnceCalled, Repeat(5), [&] {
    Timer sut(TIMEOUT, [] {});
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));
    auto timeUntilExpiration = sut.timeUntilExpiration().value().toMilliseconds();
    TIMING_TEST_EXPECT_TRUE(timeUntilExpiration == 0);
});

TIMING_TEST_F(Timer_test, StoppingIsNonBlocking, Repeat(5), [&] {
    Timer sut(1_ns, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto startTime = std::chrono::system_clock::now();
    ASSERT_FALSE(sut.stop().has_error());
    auto endTime = std::chrono::system_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    TIMING_TEST_EXPECT_TRUE(elapsedTime < 10);
});

TIMING_TEST_F(Timer_test, MultipleTimersRunningContinuously, Repeat(5), [&] {
    struct TimeValPair
    {
        TimeValPair()
            : timer(TIMEOUT, [&] { this->value++; })
        {
        }
        int value{0};
        Timer timer;
    };

    std::vector<TimeValPair> sutList(4);

    for (auto& sut : sutList)
    {
        ASSERT_FALSE(sut.timer.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));

    for (auto& sut : sutList)
    {
        ASSERT_FALSE(sut.timer.stop().has_error());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));

    for (auto& sut : sutList)
    {
        TIMING_TEST_EXPECT_TRUE(7 <= sut.value && sut.value <= 13);
    }
});

TIMING_TEST_F(Timer_test, MultipleTimersRunningOnce, Repeat(5), [&] {
    struct TimeValPair
    {
        TimeValPair()
            : timer(TIMEOUT, [&] { this->value++; })
        {
        }
        int value{0};
        Timer timer;
    };

    std::vector<TimeValPair> sutList(4);

    for (auto& sut : sutList)
    {
        ASSERT_FALSE(sut.timer.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * TIMEOUT.toMilliseconds()));

    for (auto& sut : sutList)
    {
        TIMING_TEST_EXPECT_TRUE(sut.value == 1);
    }
});

TIMING_TEST_F(Timer_test, DestructorIsBlocking, Repeat(5), [&] {
    std::chrono::time_point<std::chrono::system_clock> startTime;
    {
        Timer sut(1_ns, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });
        ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        startTime = std::chrono::system_clock::now();
    }
    auto endTime = std::chrono::system_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    TIMING_TEST_EXPECT_TRUE(10 <= elapsedTime);
});

TIMING_TEST_F(Timer_test, StartStopAndStartAgainIsNonBlocking, Repeat(5), [&] {
    Timer sut(1_ns, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });
    ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto startTime = std::chrono::system_clock::now();
    ASSERT_FALSE(sut.stop().has_error());
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    auto endTime = std::chrono::system_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    TIMING_TEST_EXPECT_TRUE(elapsedTime <= 1);
});

TEST_F(Timer_test, GetOverrunsFailsWithNoCallback)
{
    Timer sut(1_ms);
    auto call = sut.getOverruns();

    ASSERT_THAT(call.has_error(), Eq(true));
    EXPECT_THAT(call.get_error(), Eq(TimerError::TIMER_NOT_INITIALIZED));
}

TIMING_TEST_F(Timer_test, CatchUpPolicySkipToNextBeatContinuesWhenCallbackIsLongerThenTriggerTime, Repeat(5), [&] {
    std::atomic_bool hasTerminated{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error, const std::function<void()>, const iox::ErrorLevel) { hasTerminated = true; });

    Timer sut(TIMEOUT, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));
    TIMING_TEST_EXPECT_FALSE(hasTerminated);
});

TIMING_TEST_F(Timer_test, CatchUpPolicyImmediateContinuesWhenCallbackIsLongerThenTriggerTime, Repeat(5), [&] {
    std::atomic_bool hasTerminated{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error, const std::function<void()>, const iox::ErrorLevel) { hasTerminated = true; });

    Timer sut(TIMEOUT, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::IMMEDIATE).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));
    TIMING_TEST_EXPECT_FALSE(hasTerminated);
});

TIMING_TEST_F(Timer_test, CatchUpPolicyTerminateTerminatesWhenCallbackIsLongerThenTriggerTime, Repeat(5), [&] {
    std::atomic_bool hasTerminated{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error, const std::function<void()>, const iox::ErrorLevel) { hasTerminated = true; });

    Timer sut(TIMEOUT, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::TERMINATE).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));
    TIMING_TEST_EXPECT_TRUE(hasTerminated);
});

TIMING_TEST_F(Timer_test, CatchUpPolicyChangeToTerminateChangesBehaviorToTerminate, Repeat(5), [&] {
    std::atomic_bool hasTerminated{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error, const std::function<void()>, const iox::ErrorLevel) { hasTerminated = true; });

    Timer sut(TIMEOUT, [] { std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10)); });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));
    ASSERT_FALSE(sut.restart(TIMEOUT, Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::TERMINATE).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 10));

    TIMING_TEST_EXPECT_TRUE(hasTerminated);
});

TIMING_TEST_F(Timer_test, CatchUpPolicySkipToNextBeatSkipsCallbackWhenStillRunning, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] {
        ++counter;
        // wait slightly longer then the timeout so that the effect is better measurable
        std::this_thread::sleep_for(std::chrono::microseconds(TIMEOUT.toMilliseconds() * 1100));
    });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 100));
    // every second callback is skipped since the runtime is slightly longer therefore
    // the counter must be in that range
    TIMING_TEST_EXPECT_TRUE(40 <= counter && counter <= 70);
});

TIMING_TEST_F(Timer_test, CatchUpPolicyImmediateCallsCallbackImmediatelyAfterFinishing, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] {
        ++counter;
        // wait slightly longer then the timeout so that the effect is better measurable
        std::this_thread::sleep_for(std::chrono::microseconds(TIMEOUT.toMilliseconds() * 1100));
    });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::IMMEDIATE).has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 100));

    // the asap timer should in theory call the callback 90 times since it is calling it right
    // after the last one finished and one callback takes 1.1 ms and we run for 100ms.
    TIMING_TEST_EXPECT_TRUE(70 < counter && counter <= 100);
});

TIMING_TEST_F(Timer_test, CatchUpPolicySkipToNextBeatCallsLessCallbacksThanASAPTimer, Repeat(5), [&] {
    std::atomic_int counter{0};
    Timer sut(TIMEOUT, [&] {
        ++counter;
        // wait slightly longer then the timeout so that the effect is better measurable
        std::this_thread::sleep_for(std::chrono::microseconds(TIMEOUT.toMilliseconds() * 1100));
    });

    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::SKIP_TO_NEXT_BEAT).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 100));
    int softTimerCounter = counter.load();
    ASSERT_FALSE(sut.stop().has_error());

    counter.store(0);
    ASSERT_FALSE(sut.start(Timer::RunMode::PERIODIC, Timer::CatchUpPolicy::IMMEDIATE).has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT.toMilliseconds() * 100));
    int asapTimerCounter = counter.load();
    ASSERT_FALSE(sut.stop().has_error());

    TIMING_TEST_EXPECT_TRUE(softTimerCounter < asapTimerCounter);
});

/// Unit tests which segfaults (issue #243). If the segfault is fixed this unit test has
/// to be adjusted but for the moment it seems that it causes the segfault
/// reliable.
TEST_F(Timer_test, DISABLED_SelfTriggeringTimerWorksAndDoesNotCauseSegFault)
{
    Duration selfTriggerTimeout = 1_ns;
    int repetitions = 100;
    std::atomic_int counter{0};
    {
        Timer sut{selfTriggerTimeout, [&] {
                      /// this timing is set to provoke the segfault. if the timing is
                      /// decreased the segfault is more unlikely to occure but with a
                      /// value of 100 ms it always happens. see issue #243
                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                      if (counter < repetitions)
                      {
                          EXPECT_FALSE(
                              sut.restart(selfTriggerTimeout, Timer::RunMode::ONCE, Timer::CatchUpPolicy::IMMEDIATE)
                                  .has_error());
                      }
                      ++counter;
                  }};
        ASSERT_FALSE(sut.start(Timer::RunMode::ONCE, Timer::CatchUpPolicy::IMMEDIATE).has_error());

        /// this time seems to be sufficient to cause the segfault
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
} // namespace
