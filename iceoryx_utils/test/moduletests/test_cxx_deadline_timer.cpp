// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/cxx/deadline_timer.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "test.hpp"

#include <chrono>
#include <thread>

namespace
{
using namespace ::testing;

using namespace iox::units;
using namespace iox::units::duration_literals;

using Timer = iox::cxx::DeadlineTimer;

class DeadlineTimer_test : public Test
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
    static const int SLEEPTIME;
};

const Duration DeadlineTimer_test::TIMEOUT{10_ms};
const int DeadlineTimer_test::SLEEPTIME = DeadlineTimer_test::TIMEOUT.toMilliseconds();

TIMING_TEST_F(DeadlineTimer_test, ZeroTimeoutTest, Repeat(5), [&] {
    Timer sut(0_s);

    TIMING_TEST_EXPECT_TRUE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, DurationOfNonZeroIsExpiresAfterTimeout, Repeat(5), [&] {
    Timer sut(TIMEOUT);

    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME / 3));
    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME / 3));
    TIMING_TEST_EXPECT_TRUE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, ResetWithDurationIsExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));
    TIMING_TEST_EXPECT_TRUE(sut.hasExpired());
    sut.reset();
    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, ResetWhenNotExpiredIsStillNotExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME / 3));
    sut.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME / 3));
    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, ResetAfterBeingExpiredIsNotExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_ASSERT_TRUE(sut.hasExpired());
    sut.reset();
    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, ResetWithCustomizedTimeAfterBeingExpiredIsNotExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_ASSERT_TRUE(sut.hasExpired());

    sut.reset(20_s);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_EXPECT_FALSE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, ResetWithCustomizedTimeAfterBeingExpiredIsExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_ASSERT_TRUE(sut.hasExpired());

    sut.reset(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_ASSERT_TRUE(sut.hasExpired());
});

TIMING_TEST_F(DeadlineTimer_test, RemainingTimeCheckIfExpired, Repeat(5), [&] {
    Timer sut(TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEPTIME));

    TIMING_TEST_ASSERT_TRUE(sut.hasExpired());

    int remainingTime = sut.remainingTime().toMilliseconds();
    const int EXPECTED_REMAINING_TIME = 0; // the timer is expired the remaining wait time is Zero
    TIMING_TEST_EXPECT_TRUE(remainingTime == EXPECTED_REMAINING_TIME);
});

TIMING_TEST_F(DeadlineTimer_test, RemainingTimeCheckIfNotExpired, Repeat(5), [&] {
    Timer sut(2 * TIMEOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEPTIME));

    TIMING_TEST_ASSERT_FALSE(sut.hasExpired());

    int remainingTime = sut.remainingTime().toMilliseconds();
    const int PASSED_TIMER_TIME = SLEEPTIME; // Already 10ms passed in sleeping out of 20ms
    const int RANGE_APPROX = 2;              // 2ms arppoximation. This may be lost after arming the timer in execution.
    const int EXPECTED_REMAINING_TIME = PASSED_TIMER_TIME - RANGE_APPROX;

    TIMING_TEST_EXPECT_TRUE(remainingTime >= EXPECTED_REMAINING_TIME && remainingTime <= PASSED_TIMER_TIME);
});
} // namespace
