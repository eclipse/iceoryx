// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/internal/units/duration.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::units;

constexpr uint64_t SECONDS_PER_MINUTE = Duration::SECS_PER_MINUTE;
constexpr uint64_t SECONDS_PER_HOUR = Duration::SECS_PER_HOUR;
constexpr uint64_t HOURS_PER_DAY = Duration::HOURS_PER_DAY;

constexpr uint64_t MILLISECS_PER_SECOND = Duration::MILLISECS_PER_SEC;
constexpr uint64_t MICROSECS_PER_SECONDS = Duration::MICROSECS_PER_SEC;

constexpr uint64_t NANOSECS_PER_MICROSECOND = Duration::NANOSECS_PER_MICROSEC;
constexpr uint64_t NANOSECS_PER_MILLISECOND = Duration::NANOSECS_PER_MILLISEC;
constexpr uint64_t NANOSECS_PER_SECOND = Duration::NANOSECS_PER_SEC;

struct DurationAccessor : public Duration
{
    using Duration::createDuration;
    using Duration::max;

    using Duration::Nanoseconds_t;
    using Duration::Seconds_t;
};

constexpr Duration createDuration(DurationAccessor::Seconds_t seconds, DurationAccessor::Nanoseconds_t nanoseconds)
{
    return DurationAccessor::createDuration(seconds, nanoseconds);
}

TEST(Duration_test, ConversionConstants)
{
    static_assert(Duration::SECS_PER_MINUTE == 60U, "Mismatch for conversion constants!");
    static_assert(Duration::SECS_PER_HOUR == 3600U, "Mismatch for conversion constants!");
    static_assert(Duration::HOURS_PER_DAY == 24U, "Mismatch for conversion constants!");

    static_assert(Duration::MILLISECS_PER_SEC == 1000U, "Mismatch for conversion constants!");
    static_assert(Duration::MICROSECS_PER_SEC == 1000000U, "Mismatch for conversion constants!");

    static_assert(Duration::NANOSECS_PER_MICROSEC == 1000U, "Mismatch for conversion constants!");
    static_assert(Duration::NANOSECS_PER_MILLISEC == 1000000U, "Mismatch for conversion constants!");
    static_assert(Duration::NANOSECS_PER_SEC == 1000000000U, "Mismatch for conversion constants!");
}

// BEGIN CONSTRUCTOR TESTS

TEST(Duration_test, ConstructDurationWithZeroTime)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{0U};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{0U};

    auto sut = createDuration(SECONDS, NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithResultOfLessNanosecondsThanOneSecond)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{7337U};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{NANOSECONDS};

    auto sut = createDuration(SECONDS, NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithNanosecondsLessThanOneSecond)
{
    constexpr uint64_t SECONDS{37U};
    constexpr uint64_t NANOSECONDS{73U};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{SECONDS * NANOSECS_PER_SECOND + NANOSECONDS};

    auto sut = createDuration(SECONDS, NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithNanosecondsEqualToOneSecond)
{
    constexpr uint64_t SECONDS{13U};
    constexpr uint64_t NANOSECONDS{NANOSECS_PER_SECOND};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{(SECONDS + 1U) * NANOSECS_PER_SECOND};

    auto sut = createDuration(SECONDS, NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithNanosecondsMoreThanOneSecond)
{
    constexpr uint64_t SECONDS{37U};
    constexpr uint64_t NANOSECONDS{42U};
    constexpr uint64_t MORE_THAN_ONE_SECOND_NANOSECONDS{NANOSECS_PER_SECOND + NANOSECONDS};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{(SECONDS + 1U) * NANOSECS_PER_SECOND + NANOSECONDS};

    auto sut = createDuration(SECONDS, MORE_THAN_ONE_SECOND_NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithNanosecondsMaxValue)
{
    constexpr uint64_t SECONDS{37U};
    constexpr uint64_t MAX_NANOSECONDS_FOR_CTOR{std::numeric_limits<DurationAccessor::Nanoseconds_t>::max()};
    constexpr uint64_t EXPECTED_SECONDS = SECONDS + MAX_NANOSECONDS_FOR_CTOR / NANOSECS_PER_SECOND;
    constexpr uint64_t REMAINING_NANOSECONDS = MAX_NANOSECONDS_FOR_CTOR % NANOSECS_PER_SECOND;
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{(EXPECTED_SECONDS)*NANOSECS_PER_SECOND + REMAINING_NANOSECONDS};

    auto sut = createDuration(SECONDS, MAX_NANOSECONDS_FOR_CTOR);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructDurationWithSecondsAndNanosecondsMaxValues)
{
    constexpr uint64_t MAX_SECONDS_FOR_CTOR{std::numeric_limits<DurationAccessor::Seconds_t>::max()};
    constexpr uint64_t MAX_NANOSECONDS_FOR_CTOR{std::numeric_limits<DurationAccessor::Nanoseconds_t>::max()};

    auto sut = createDuration(MAX_SECONDS_FOR_CTOR, MAX_NANOSECONDS_FOR_CTOR);

    EXPECT_THAT(sut, Eq(DurationAccessor::max()));
}

TEST(Duration_test, ConstructDurationWithOneNanosecondResultsNotInZeroNanoseconds)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{1U};
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{NANOSECONDS};

    auto sut = createDuration(SECONDS, NANOSECONDS);

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, ConstructFromTimespecWithZeroValue)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{0U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct timespec ts;
    ts.tv_sec = SECONDS;
    ts.tv_nsec = NANOSECONDS;

    Duration sut{ts};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimespecWithValueLessThanOneSecond)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{456U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct timespec value;
    value.tv_sec = SECONDS;
    value.tv_nsec = NANOSECONDS;

    Duration sut{value};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimespecWithValueMoreThanOneSecond)
{
    constexpr uint64_t SECONDS{73U};
    constexpr uint64_t NANOSECONDS{456U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct timespec value;
    value.tv_sec = SECONDS;
    value.tv_nsec = NANOSECONDS;

    Duration sut{value};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimespecWithMaxValue)
{
    constexpr uint64_t SECONDS{std::numeric_limits<DurationAccessor::Seconds_t>::max()};
    constexpr uint64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    struct timespec ts;
    ts.tv_sec = SECONDS;
    ts.tv_nsec = NANOSECONDS;

    Duration sut{ts};
    EXPECT_THAT(sut, Eq(DurationAccessor::max()));
}

TEST(Duration_test, ConstructFromITimerspecWithZeroValue)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{0U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct itimerspec its;
    its.it_interval.tv_sec = SECONDS;
    its.it_interval.tv_nsec = NANOSECONDS;

    Duration sut{its};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromITimerspecWithValueLessThanOneSecond)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t NANOSECONDS{642U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct itimerspec its;
    its.it_interval.tv_sec = SECONDS;
    its.it_interval.tv_nsec = NANOSECONDS;

    Duration sut{its};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromITimerspecWithValueMoreThanOneSecond)
{
    constexpr uint64_t SECONDS{13U};
    constexpr uint64_t NANOSECONDS{42U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, NANOSECONDS);

    struct itimerspec its;
    its.it_interval.tv_sec = SECONDS;
    its.it_interval.tv_nsec = NANOSECONDS;

    Duration sut{its};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromITimerspecWithMaxValue)
{
    constexpr uint64_t SECONDS{std::numeric_limits<DurationAccessor::Seconds_t>::max()};
    constexpr uint64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    struct itimerspec its;
    its.it_interval.tv_sec = SECONDS;
    its.it_interval.tv_nsec = NANOSECONDS;

    Duration sut{its};
    EXPECT_THAT(sut, Eq(DurationAccessor::max()));
}

TEST(Duration_test, ConstructFromTimevalWithZeroValue)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t MICROSECONDS{0U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, MICROSECONDS * NANOSECS_PER_MICROSECOND);

    struct timeval tv;
    tv.tv_sec = SECONDS;
    tv.tv_usec = MICROSECONDS;

    Duration sut{tv};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimevalWithValueLessThanOneSecond)
{
    constexpr uint64_t SECONDS{0U};
    constexpr uint64_t MICROSECONDS{13U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, MICROSECONDS * NANOSECS_PER_MICROSECOND);

    struct timeval tv;
    tv.tv_sec = SECONDS;
    tv.tv_usec = MICROSECONDS;

    Duration sut{tv};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimevalWithValueMoreThanOneSecond)
{
    constexpr uint64_t SECONDS{1337U};
    constexpr uint64_t MICROSECONDS{42U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, MICROSECONDS * NANOSECS_PER_MICROSECOND);

    struct timeval tv;
    tv.tv_sec = SECONDS;
    tv.tv_usec = MICROSECONDS;

    Duration sut{tv};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromTimevalWithMaxValue)
{
    constexpr uint64_t SECONDS{std::numeric_limits<uint64_t>::max()};
    constexpr uint64_t MICROSECONDS{Duration::MICROSECS_PER_SEC - 1U};
    constexpr Duration EXPECTED_DURATION = createDuration(SECONDS, MICROSECONDS * Duration::NANOSECS_PER_MICROSEC);

    struct timeval tv;
    tv.tv_sec = SECONDS;
    tv.tv_usec = MICROSECONDS;

    Duration sut{tv};
    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, ConstructFromChronoMillisecondsZero)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{0U};
    Duration sut{std::chrono::milliseconds(EXPECTED_MILLISECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, ConstructFromChronoMillisecondsLessThanOneSecond)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{44U};
    Duration sut{std::chrono::milliseconds(EXPECTED_MILLISECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_MILLISECONDS * NANOSECS_PER_MILLISECOND));
}

TEST(Duration_test, ConstructFromChronoMillisecondsMoreThanOneSecond)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{1001};
    Duration sut{std::chrono::milliseconds(EXPECTED_MILLISECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_MILLISECONDS * NANOSECS_PER_MILLISECOND));
}

TEST(Duration_test, ConstructFromChronoMillisecondsMax)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{std::numeric_limits<int64_t>::max()};
    Duration sut{std::chrono::milliseconds(EXPECTED_MILLISECONDS)};
    EXPECT_THAT(sut.toMilliseconds(), Eq(EXPECTED_MILLISECONDS));
}

TEST(Duration_test, ConstructFromNegativeChronoMillisecondsIsZero)
{
    Duration sut(std::chrono::milliseconds(-1));
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, ConstructFromChronoNanosecondsZero)
{
    constexpr uint64_t EXPECTED_NANOSECONDS{0U};
    Duration sut{std::chrono::nanoseconds(EXPECTED_NANOSECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_NANOSECONDS));
}

TEST(Duration_test, ConstructFromChronoNanosecondsLessThanOneSecond)
{
    constexpr uint64_t EXPECTED_NANOSECONDS{424242U};
    Duration sut{std::chrono::nanoseconds(EXPECTED_NANOSECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_NANOSECONDS));
}

TEST(Duration_test, ConstructFromChronoNanosecondsMoreThanOneSecond)
{
    constexpr uint64_t EXPECTED_NANOSECONDS{NANOSECS_PER_SECOND + 42U};
    Duration sut{std::chrono::nanoseconds(EXPECTED_NANOSECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_NANOSECONDS));
}

TEST(Duration_test, ConstructFromChronoNanosecondsMax)
{
    constexpr uint64_t EXPECTED_NANOSECONDS{std::numeric_limits<int64_t>::max()};
    Duration sut{std::chrono::nanoseconds(EXPECTED_NANOSECONDS)};
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_NANOSECONDS));
}

TEST(Duration_test, ConstructFromNegativeChronoNanosecondsIsZero)
{
    Duration sut(std::chrono::nanoseconds(-1));
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

// END CONSTRUCTOR TESTS

// BEGIN ASSIGNMENT TESTS

TEST(Duration_test, AssignFromChronoMillisecondsZero)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{0U};
    Duration sut = 0_ns;
    sut = std::chrono::milliseconds(EXPECTED_MILLISECONDS);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, AssignFromChronoMillisecondsLessThanOneSecond)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{73U};
    Duration sut = 0_ns;
    sut = std::chrono::milliseconds(EXPECTED_MILLISECONDS);
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_MILLISECONDS * NANOSECS_PER_MILLISECOND));
}

TEST(Duration_test, AssignFromChronoMillisecondsMoreThanOneSecond)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{1073U};
    Duration sut = 0_ns;
    sut = std::chrono::milliseconds(EXPECTED_MILLISECONDS);
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_MILLISECONDS * NANOSECS_PER_MILLISECOND));
}

TEST(Duration_test, AssignFromChronoMillisecondsMax)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{std::numeric_limits<int64_t>::max()};
    Duration sut = 0_ns;
    sut = std::chrono::milliseconds(EXPECTED_MILLISECONDS);
    EXPECT_THAT(sut.toMilliseconds(), Eq(EXPECTED_MILLISECONDS));
}

TEST(Duration_test, AssignFromNegativeChronoMillisecondsIsZero)
{
    Duration sut = 22_ns;
    sut = std::chrono::milliseconds(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

// END ASSIGNMENT TESTS

// BEGIN CREATION FROM LITERAL TESTS

TEST(Duration_test, CreateDurationFromDaysLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{2U * HOURS_PER_DAY * SECONDS_PER_HOUR * NANOSECS_PER_SECOND};
    auto sut = 2_d;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromHoursLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{3U * SECONDS_PER_HOUR * NANOSECS_PER_SECOND};
    auto sut = 3_h;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMinutesLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{4U * SECONDS_PER_MINUTE * NANOSECS_PER_SECOND};
    auto sut = 4_m;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromSecondsLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{5U * NANOSECS_PER_SECOND};
    auto sut = 5_s;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMillisecondsLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{6U * NANOSECS_PER_MILLISECOND};
    auto sut = 6_ms;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMicrosecondsLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{7U * NANOSECS_PER_MICROSECOND};
    auto sut = 7_us;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromNanosecondsLiteral)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{8U};
    auto sut = 8_ns;

    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

// END CREATION FROM LITERAL TESTS

// BEGIN CREATION FROM STATIC FUNCTION TESTS

TEST(Duration_test, CreateDurationFromDaysFunctionWithZeroDays)
{
    auto sut1 = Duration::fromDays(0);
    auto sut2 = Duration::fromDays(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromDaysFunctionWithMultipleDays)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{2U * 24U * SECONDS_PER_HOUR * NANOSECS_PER_SECOND};
    auto sut1 = Duration::fromDays(2);
    auto sut2 = Duration::fromDays(2U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromDaysFunctionWithDaysResultsNotYetInSaturation)
{
    constexpr uint64_t SECONDS_PER_DAY{HOURS_PER_DAY * SECONDS_PER_HOUR};
    constexpr uint64_t MAX_DAYS_BEFORE_OVERFLOW{std::numeric_limits<DurationAccessor::Seconds_t>::max()
                                                / SECONDS_PER_DAY};
    constexpr Duration EXPECTED_DURATION = createDuration(MAX_DAYS_BEFORE_OVERFLOW * SECONDS_PER_DAY, 0U);
    static_assert(EXPECTED_DURATION < DurationAccessor::max(),
                  "EXPECTED_DURATION too large to exclude saturation! Please decrease!");

    auto sut1 = Duration::fromDays(static_cast<int64_t>(MAX_DAYS_BEFORE_OVERFLOW));
    auto sut2 = Duration::fromDays(MAX_DAYS_BEFORE_OVERFLOW);

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, CreateDurationFromDaysFunctionWithMaxDaysResultsInSaturation)
{
    auto sut1 = Duration::fromDays(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromDays(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(DurationAccessor::max()));
    EXPECT_THAT(sut2, Eq(DurationAccessor::max()));
}

TEST(Duration_test, CreateDurationFromDaysFunctionWithNegativeValuesIsZero)
{
    auto sut = Duration::fromDays(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromHoursFunctionWithZeroHours)
{
    auto sut1 = Duration::fromHours(0);
    auto sut2 = Duration::fromHours(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromHoursFunctionWithMultipleHours)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{3U * SECONDS_PER_HOUR * NANOSECS_PER_SECOND};
    auto sut1 = Duration::fromHours(3);
    auto sut2 = Duration::fromHours(3U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromHoursFunctionWithHoursResultsNotYetInSaturation)
{
    constexpr uint64_t MAX_HOURS_BEFORE_OVERFLOW{std::numeric_limits<DurationAccessor::Seconds_t>::max()
                                                 / SECONDS_PER_HOUR};
    constexpr Duration EXPECTED_DURATION = createDuration(MAX_HOURS_BEFORE_OVERFLOW * SECONDS_PER_HOUR, 0U);
    static_assert(EXPECTED_DURATION < DurationAccessor::max(),
                  "EXPECTED_DURATION too large to exclude saturation! Please decrease!");

    auto sut1 = Duration::fromHours(static_cast<int64_t>(MAX_HOURS_BEFORE_OVERFLOW));
    auto sut2 = Duration::fromHours(MAX_HOURS_BEFORE_OVERFLOW);

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, CreateDurationFromHoursFunctionWithMaxHoursResultsInSaturation)
{
    auto sut1 = Duration::fromHours(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromHours(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(DurationAccessor::max()));
    EXPECT_THAT(sut2, Eq(DurationAccessor::max()));
}

TEST(Duration_test, CreateDurationFromHoursFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromHours(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMinutesFunctionWithZeroMinuts)
{
    auto sut1 = Duration::fromMinutes(0);
    auto sut2 = Duration::fromMinutes(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMinutesFunctionWithMultipleMinutes)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{4U * SECONDS_PER_MINUTE * NANOSECS_PER_SECOND};
    auto sut1 = Duration::fromMinutes(4);
    auto sut2 = Duration::fromMinutes(4U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMinutesFunctionWithMinutesResultsNotYetInSaturation)
{
    constexpr uint64_t MAX_MINUTES_BEFORE_OVERFLOW{std::numeric_limits<DurationAccessor::Seconds_t>::max()
                                                   / SECONDS_PER_MINUTE};
    constexpr Duration EXPECTED_DURATION = createDuration(MAX_MINUTES_BEFORE_OVERFLOW * SECONDS_PER_MINUTE, 0U);
    static_assert(EXPECTED_DURATION < DurationAccessor::max(),
                  "EXPECTED_DURATION too large to exclude saturation! Please decrease!");

    auto sut1 = Duration::fromMinutes(static_cast<int64_t>(MAX_MINUTES_BEFORE_OVERFLOW));
    auto sut2 = Duration::fromMinutes(MAX_MINUTES_BEFORE_OVERFLOW);

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, CreateDurationFromMinutesFunctionWithMaxMinutesResultsInSaturation)
{
    auto sut1 = Duration::fromMinutes(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromMinutes(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(DurationAccessor::max()));
    EXPECT_THAT(sut2, Eq(DurationAccessor::max()));
}

TEST(Duration_test, CreateDurationFromMinutesFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromMinutes(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromSecondsFunctionWithZeroSeconds)
{
    auto sut1 = Duration::fromSeconds(0);
    auto sut2 = Duration::fromSeconds(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromSecondsFunction)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{5U * NANOSECS_PER_SECOND};
    auto sut1 = Duration::fromSeconds(5);
    auto sut2 = Duration::fromSeconds(5U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromSecondsFunctionWithMaxSeconds)
{
    constexpr uint64_t MAX_SECONDS_FROM_SIGNED{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_SIGNED = createDuration(MAX_SECONDS_FROM_SIGNED, 0U);
    constexpr uint64_t MAX_SECONDS_FROM_USIGNED{std::numeric_limits<uint64_t>::max()};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_UNSIGNED = createDuration(MAX_SECONDS_FROM_USIGNED, 0U);

    auto sut1 = Duration::fromSeconds(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromSeconds(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION_FROM_MAX_SIGNED));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION_FROM_MAX_UNSIGNED));
}

TEST(Duration_test, CreateDurationFromSecondsFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromSeconds(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMillisecondsFunctionWithZeroMilliseconds)
{
    auto sut1 = Duration::fromMilliseconds(0);
    auto sut2 = Duration::fromMilliseconds(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMillisecondsFunctionWithMultipleMilliseconds)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{6U * NANOSECS_PER_MILLISECOND};
    auto sut1 = Duration::fromMilliseconds(6);
    auto sut2 = Duration::fromMilliseconds(6U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMillisecondsFunctionWithMaxMilliseconds)
{
    constexpr uint64_t MAX_MILLISECONDS_FROM_SIGNED{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_SIGNED =
        createDuration(MAX_MILLISECONDS_FROM_SIGNED / MILLISECS_PER_SECOND,
                       (MAX_MILLISECONDS_FROM_SIGNED % MILLISECS_PER_SECOND) * NANOSECS_PER_MILLISECOND);
    constexpr uint64_t MAX_MILLISECONDS_FROM_USIGNED{std::numeric_limits<uint64_t>::max()};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_UNSIGNED =
        createDuration(MAX_MILLISECONDS_FROM_USIGNED / MILLISECS_PER_SECOND,
                       (MAX_MILLISECONDS_FROM_USIGNED % MILLISECS_PER_SECOND) * NANOSECS_PER_MILLISECOND);

    auto sut1 = Duration::fromMilliseconds(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromMilliseconds(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION_FROM_MAX_SIGNED));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION_FROM_MAX_UNSIGNED));
}

TEST(Duration_test, CreateDurationFromMillisecondsFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromMilliseconds(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMicrosecondsFunctionWithZeroMicroseconds)
{
    auto sut1 = Duration::fromMicroseconds(0);
    auto sut2 = Duration::fromMicroseconds(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromMicrosecondsFunctionWithMultipleMicroseconds)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{7U * NANOSECS_PER_MICROSECOND};
    auto sut1 = Duration::fromMicroseconds(7);
    auto sut2 = Duration::fromMicroseconds(7U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromMicrosecondsFunctionWithMaxMicroseconds)
{
    constexpr uint64_t MAX_MICROSECONDS_FROM_SIGNED{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_SIGNED =
        createDuration(MAX_MICROSECONDS_FROM_SIGNED / MICROSECS_PER_SECONDS,
                       (MAX_MICROSECONDS_FROM_SIGNED % MICROSECS_PER_SECONDS) * NANOSECS_PER_MICROSECOND);
    constexpr uint64_t MAX_MICROSECONDS_FROM_USIGNED{std::numeric_limits<uint64_t>::max()};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_UNSIGNED =
        createDuration(MAX_MICROSECONDS_FROM_USIGNED / MICROSECS_PER_SECONDS,
                       (MAX_MICROSECONDS_FROM_USIGNED % MICROSECS_PER_SECONDS) * NANOSECS_PER_MICROSECOND);

    auto sut1 = Duration::fromMicroseconds(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromMicroseconds(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION_FROM_MAX_SIGNED));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION_FROM_MAX_UNSIGNED));
}

TEST(Duration_test, CreateDurationFromMicrosecondsFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromMicroseconds(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromNanosecondsFunctionWithZeroNanoseconds)
{
    auto sut1 = Duration::fromNanoseconds(0);
    auto sut2 = Duration::fromNanoseconds(0U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(0U));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(0U));
}

TEST(Duration_test, CreateDurationFromNanosecondsFunctionWithMultipleNanoseconds)
{
    constexpr uint64_t EXPECTED_DURATION_IN_NANOSECONDS{8U};
    auto sut1 = Duration::fromNanoseconds(8);
    auto sut2 = Duration::fromNanoseconds(8U);

    EXPECT_THAT(sut1.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
    EXPECT_THAT(sut2.toNanoseconds(), Eq(EXPECTED_DURATION_IN_NANOSECONDS));
}

TEST(Duration_test, CreateDurationFromNanosecondsFunction)
{
    constexpr uint64_t MAX_NANOSECONDS_FROM_SIGNED{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_SIGNED = createDuration(
        MAX_NANOSECONDS_FROM_SIGNED / NANOSECS_PER_SECOND, MAX_NANOSECONDS_FROM_SIGNED % NANOSECS_PER_SECOND);
    constexpr uint64_t MAX_NANOSECONDS_FROM_USIGNED{std::numeric_limits<uint64_t>::max()};
    constexpr Duration EXPECTED_DURATION_FROM_MAX_UNSIGNED = createDuration(
        MAX_NANOSECONDS_FROM_USIGNED / NANOSECS_PER_SECOND, MAX_NANOSECONDS_FROM_USIGNED % NANOSECS_PER_SECOND);

    auto sut1 = Duration::fromNanoseconds(std::numeric_limits<int64_t>::max());
    auto sut2 = Duration::fromNanoseconds(std::numeric_limits<uint64_t>::max());

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION_FROM_MAX_SIGNED));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION_FROM_MAX_UNSIGNED));
}

TEST(Duration_test, CreateDurationFromNanosecondsFunctionWithNegativeValueIsZero)
{
    auto sut = Duration::fromNanoseconds(-1);
    EXPECT_THAT(sut.toNanoseconds(), Eq(0U));
}

// END CREATION FROM STATIC FUNCTION TESTS

// BEGIN CONVERSION FUNCTION TESTS

TEST(Duration_test, ConvertDaysFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toDays(), Eq(0U));
}

TEST(Duration_test, ConvertDaysFromDurationLessThanOneDay)
{
    auto sut = 3473_s;
    EXPECT_THAT(sut.toDays(), Eq(0U));
}

TEST(Duration_test, ConvertDaysFromDurationMoreThanOneDay)
{
    auto sut = 7_d + 3066_s;
    EXPECT_THAT(sut.toDays(), Eq(7U));
}

TEST(Duration_test, ConvertDaysFromMaxDuration)
{
    constexpr uint64_t SECONDS_PER_DAY{60U * 60U * 24U};
    constexpr uint64_t EXPECTED_DAYS{std::numeric_limits<DurationAccessor::Seconds_t>::max() / SECONDS_PER_DAY};
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toDays(), Eq(EXPECTED_DAYS));
}

TEST(Duration_test, ConvertHoursFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toHours(), Eq(0U));
}

TEST(Duration_test, ConvertHoursFromDurationLessThanOneHour)
{
    auto sut = 37_m;
    EXPECT_THAT(sut.toHours(), Eq(0U));
}

TEST(Duration_test, ConvertHoursFromDurationMoreThanOneHour)
{
    auto sut = 73_h + 42_m;
    EXPECT_THAT(sut.toHours(), Eq(73U));
}

TEST(Duration_test, ConvertHoursFromMaxDuration)
{
    constexpr uint64_t SECONDS_PER_HOUR{60U * 60U};
    constexpr uint64_t EXPECTED_HOURS{std::numeric_limits<DurationAccessor::Seconds_t>::max() / SECONDS_PER_HOUR};
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toHours(), Eq(EXPECTED_HOURS));
}

TEST(Duration_test, ConvertMinutesFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toMinutes(), Eq(0U));
}

TEST(Duration_test, ConvertMinutesFromDurationLessThanOneMinute)
{
    auto sut = 34_s;
    EXPECT_THAT(sut.toMinutes(), Eq(0U));
}

TEST(Duration_test, ConvertMinutesFromDurationMoreThanOneMinute)
{
    auto sut = 13_m + 42_s;
    EXPECT_THAT(sut.toMinutes(), Eq(13U));
}

TEST(Duration_test, ConvertMinutesFromMaxDuration)
{
    constexpr uint64_t SECONDS_PER_MINUTE{60U};
    constexpr uint64_t EXPECTED_MINUTES{std::numeric_limits<DurationAccessor::Seconds_t>::max() / SECONDS_PER_MINUTE};
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toMinutes(), Eq(EXPECTED_MINUTES));
}

TEST(Duration_test, ConvertSecondsFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toSeconds(), Eq(0U));
}

TEST(Duration_test, ConvertSecondsFromDurationLessThanOneSecond)
{
    auto sut = 737_ms;
    EXPECT_THAT(sut.toSeconds(), Eq(0U));
}

TEST(Duration_test, ConvertSecondsFromDurationMoreThanOneSecond)
{
    auto sut = 7_s + 833_ms;
    EXPECT_THAT(sut.toSeconds(), Eq(7U));
}

TEST(Duration_test, ConvertSecondsFromMaxSecondsMinusOne)
{
    constexpr uint64_t EXPECTED_SECONDS{std::numeric_limits<DurationAccessor::Seconds_t>::max() - 1U};
    auto sut = DurationAccessor::max() - 1_s;
    EXPECT_THAT(sut.toSeconds(), Eq(EXPECTED_SECONDS));
}

TEST(Duration_test, ConvertSecondsFromMaxDuration)
{
    constexpr uint64_t EXPECTED_SECONDS{std::numeric_limits<DurationAccessor::Seconds_t>::max()};
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toSeconds(), Eq(EXPECTED_SECONDS));
}

TEST(Duration_test, ConvertMillisecondsFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toMilliseconds(), Eq(0U));
}

TEST(Duration_test, ConvertMillisecondsFromDurationLessThanOneMillisecond)
{
    auto sut = 637_us;
    EXPECT_THAT(sut.toMilliseconds(), Eq(0U));
}

TEST(Duration_test, ConvertMilliecondsFromDurationMoreThanOneMillisecond)
{
    auto sut = 55_ms + 633_us;
    EXPECT_THAT(sut.toMilliseconds(), Eq(55U));
}

TEST(Duration_test, ConvertMillisecondsFromDurationResultsNotYetInSaturation)
{
    constexpr uint64_t EXPECTED_MILLISECONDS{std::numeric_limits<uint64_t>::max() - 1U};
    auto sut = Duration::fromMilliseconds(EXPECTED_MILLISECONDS);
    EXPECT_THAT(sut.toMilliseconds(), Eq(EXPECTED_MILLISECONDS));
}

TEST(Duration_test, ConvertMillisecondsFromMaxDurationResultsInSaturation)
{
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toMilliseconds(), Eq(std::numeric_limits<uint64_t>::max()));
}

TEST(Duration_test, ConvertMicrosecondsFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toMicroseconds(), Eq(0U));
}

TEST(Duration_test, ConvertMicrosecondsFromDurationLessThanOneMicrosecond)
{
    auto sut = 733_ns;
    EXPECT_THAT(sut.toMicroseconds(), Eq(0U));
}

TEST(Duration_test, ConvertMicrosecondsFromDurationMoreThanOneMicrosecond)
{
    auto sut = 555_us + 733_ns;
    EXPECT_THAT(sut.toMicroseconds(), Eq(555U));
}

TEST(Duration_test, ConvertMicrosecondsFromDurationResultsNotYetInSaturation)
{
    constexpr uint64_t EXPECTED_MICROSECONDS{std::numeric_limits<uint64_t>::max() - 1U};
    auto sut = Duration::fromMicroseconds(EXPECTED_MICROSECONDS);
    EXPECT_THAT(sut.toMicroseconds(), Eq(EXPECTED_MICROSECONDS));
}

TEST(Duration_test, ConvertMicroecondsFromMaxDurationResultsInSaturation)
{
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toMicroseconds(), Eq(std::numeric_limits<uint64_t>::max()));
}

TEST(Duration_test, ConvertNanosecondsFromZeroDuration)
{
    auto sut = 0_s;
    EXPECT_THAT(sut.toMicroseconds(), Eq(0U));
}

TEST(Duration_test, ConvertNanosecondsFromDurationOfOneNanosecond)
{
    auto sut = 1_ns;
    EXPECT_THAT(sut.toNanoseconds(), Eq(1U));
}

TEST(Duration_test, ConvertNanosecondsFromDurationMultipleNanoseconds)
{
    auto sut = 42_ns;
    EXPECT_THAT(sut.toNanoseconds(), Eq(42U));
}

TEST(Duration_test, ConvertNanosecondsFromDurationResultsNotYetInSaturation)
{
    constexpr uint64_t EXPECTED_NANOSECONDS{std::numeric_limits<uint64_t>::max() - 1U};
    auto sut = Duration::fromNanoseconds(EXPECTED_NANOSECONDS);
    EXPECT_THAT(sut.toNanoseconds(), Eq(EXPECTED_NANOSECONDS));
}

TEST(Duration_test, ConvertNanoecondsFromMaxDurationResultsInSaturation)
{
    auto sut = DurationAccessor::max();
    EXPECT_THAT(sut.toNanoseconds(), Eq(std::numeric_limits<uint64_t>::max()));
}

TEST(Duration_test, ConvertTimespecWithNoneReferenceFromZeroDuration)
{
    constexpr int64_t SECONDS{0};
    constexpr int64_t NANOSECONDS{0};

    auto duration = createDuration(SECONDS, NANOSECONDS);

    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::None);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithNoneReferenceFromDurationLessThanOneSecond)
{
    constexpr int64_t SECONDS{0};
    constexpr int64_t NANOSECONDS{55};

    auto duration = createDuration(SECONDS, NANOSECONDS);

    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::None);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithNoneReferenceFromDurationMoreThanOneSecond)
{
    constexpr int64_t SECONDS{44};
    constexpr int64_t NANOSECONDS{55};

    auto duration = createDuration(SECONDS, NANOSECONDS);

    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::None);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithNoneReferenceFromDurationResultsNotYetInSaturation)
{
    constexpr int64_t SECONDS{std::numeric_limits<int64_t>::max()};
    constexpr int64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    auto duration = createDuration(SECONDS, NANOSECONDS);

    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::None);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithNoneReferenceFromMaxDurationResultsInSaturation)
{
    constexpr int64_t SECONDS{std::numeric_limits<int64_t>::max()};
    constexpr int64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    struct timespec sut = DurationAccessor::max().timespec(iox::units::TimeSpecReference::None);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithMonotonicReference)
{
    constexpr int64_t SECONDS{4};
    constexpr int64_t NANOSECONDS{66};

    auto timeSinceUnixEpoch = std::chrono::system_clock::now().time_since_epoch();
    auto timeSinceMonotonicEpoch = std::chrono::steady_clock::now().time_since_epoch();

    auto duration = createDuration(SECONDS, NANOSECONDS);
    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::Monotonic);

    auto secondsSinceUnixEpoch = std::chrono::duration_cast<std::chrono::seconds>(timeSinceUnixEpoch).count();
    auto secondsSinceMonotonicEpoch = std::chrono::duration_cast<std::chrono::seconds>(timeSinceMonotonicEpoch).count();
    EXPECT_THAT(sut.tv_sec, Lt(secondsSinceUnixEpoch));
    EXPECT_THAT(sut.tv_sec, Gt(secondsSinceMonotonicEpoch));
}

TEST(Duration_test, ConvertTimespecWithMonotonicReferenceFromMaxDurationResultsInSaturation)
{
    constexpr int64_t SECONDS{std::numeric_limits<int64_t>::max()};
    constexpr int64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    struct timespec sut = DurationAccessor::max().timespec(iox::units::TimeSpecReference::Monotonic);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

TEST(Duration_test, ConvertTimespecWithEpochReference)
{
    constexpr int64_t SECONDS{5};
    constexpr int64_t NANOSECONDS{77};

    auto timeSinceUnixEpoch = std::chrono::system_clock::now().time_since_epoch();

    auto duration = createDuration(SECONDS, NANOSECONDS);
    struct timespec sut = duration.timespec(iox::units::TimeSpecReference::Epoch);

    auto secondsSinceUnixEpoch = std::chrono::duration_cast<std::chrono::seconds>(timeSinceUnixEpoch).count();
    EXPECT_THAT(10 * SECONDS, Lt(secondsSinceUnixEpoch));
    EXPECT_THAT(sut.tv_sec, Gt(secondsSinceUnixEpoch));
}

TEST(Duration_test, ConvertTimespecWithEpochReferenceFromMaxDurationResultsInSaturation)
{
    constexpr int64_t SECONDS{std::numeric_limits<int64_t>::max()};
    constexpr int64_t NANOSECONDS{NANOSECS_PER_SECOND - 1U};

    struct timespec sut = DurationAccessor::max().timespec(iox::units::TimeSpecReference::Epoch);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_nsec, Eq(NANOSECONDS));
}

// END CONVERSION FUNCTION TESTS

// BEGIN CONVERSION OPERATOR TESTS

TEST(Duration_test, OperatorTimevalFromZeroDuration)
{
    auto duration = createDuration(0U, 0U);

    struct timeval sut = timeval(duration);

    EXPECT_THAT(sut.tv_sec, Eq(0U));
    EXPECT_THAT(sut.tv_usec, Eq(0U));
}

TEST(Duration_test, OperatorTimevalFromDurationWithLessThanOneSecond)
{
    constexpr int64_t SECONDS{0};
    constexpr int64_t MICROSECONDS{222};
    constexpr int64_t ROUND_OFF_NANOSECONDS{666};

    auto duration = createDuration(SECONDS, MICROSECONDS * NANOSECS_PER_MICROSECOND + ROUND_OFF_NANOSECONDS);

    struct timeval sut = timeval(duration);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_usec, Eq(MICROSECONDS));
}

TEST(Duration_test, OperatorTimevalFromDurationWithMoreThanOneSecond)
{
    constexpr int64_t SECONDS{111};
    constexpr int64_t MICROSECONDS{222};
    constexpr int64_t ROUND_OFF_NANOSECONDS{666};

    auto duration = createDuration(SECONDS, MICROSECONDS * NANOSECS_PER_MICROSECOND + ROUND_OFF_NANOSECONDS);

    struct timeval sut = timeval(duration);

    EXPECT_THAT(sut.tv_sec, Eq(SECONDS));
    EXPECT_THAT(sut.tv_usec, Eq(MICROSECONDS));
}

TEST(Duration_test, OperatorTimevalFromDurationResultsNotYetInSaturation)
{
    using SEC_TYPE = decltype(timeval::tv_sec);
    auto duration = Duration::fromSeconds(std::numeric_limits<SEC_TYPE>::max());

    struct timeval sut = timeval(duration);

    EXPECT_THAT(sut.tv_sec, Eq(std::numeric_limits<SEC_TYPE>::max()));
    EXPECT_THAT(sut.tv_usec, Eq(0));
}

TEST(Duration_test, OperatorTimevalFromMaxDurationResultsInSaturation)
{
    using SEC_TYPE = decltype(timeval::tv_sec);
    using USEC_TYPE = decltype(timeval::tv_usec);

    struct timeval sut = timeval(DurationAccessor::max());

    EXPECT_THAT(sut.tv_sec, Eq(std::numeric_limits<SEC_TYPE>::max()));
    EXPECT_THAT(sut.tv_usec, Eq(static_cast<USEC_TYPE>(MICROSECS_PER_SECONDS - 1U)));
}

// END CONVERSION OPERATOR TESTS

// BEGIN COMPARISON TESTS

TEST(Duration_test, CompareTwoEqualDurationsForEquality)
{
    auto time1 = 200_us;
    auto time2 = 200000_ns;
    EXPECT_TRUE(time1 == time2);
}

TEST(Duration_test, CompareTwoNonEqualDurationsForEquality)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_s + 1_ns;
    auto time3 = 1_ns;
    EXPECT_FALSE(time1 == time2);
    EXPECT_FALSE(time2 == time1);
    EXPECT_FALSE(time2 == time3);
    EXPECT_FALSE(time3 == time2);
}

TEST(Duration_test, CompareTwoNonEqualDurationsForInequality)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_ns;
    EXPECT_TRUE(time1 != time2);
    EXPECT_TRUE(time2 != time1);
}

TEST(Duration_test, CompareTwoEqualDurationsForInequality)
{
    auto time1 = 200_us;
    auto time2 = 200000_ns;
    EXPECT_FALSE(time1 != time2);
}

TEST(Duration_test, CompareTwoEqualDurationsAreNotLessThan)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_s + 200_us;
    EXPECT_FALSE(time1 < time2);
}

TEST(Duration_test, CompareTwoEqualDurationsAreNotGreaterThan)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_s + 200_us;
    EXPECT_FALSE(time1 > time2);
}

TEST(Duration_test, CompareTwoEqualDurationsAreLessThanOrEqualTo)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_s + 200_us;
    EXPECT_TRUE(time1 <= time2);
}

TEST(Duration_test, CompareTwoEqualDurationsAreGreaterThanOrEqualTo)
{
    auto time1 = 1_s + 200_us;
    auto time2 = 1_s + 200_us;
    EXPECT_TRUE(time1 >= time2);
}

TEST(Duration_test, CompareDurationIsLessThanOther)
{
    auto time1 = 100_us;
    auto time2 = 400_us;
    auto time3 = 1_s + 200_us;
    auto time4 = 1_s + 300_us;
    EXPECT_TRUE(time1 < time2);
    EXPECT_TRUE(time1 < time3);
    EXPECT_TRUE(time2 < time3);
    EXPECT_TRUE(time3 < time4);
}

TEST(Duration_test, CompareDurationIsNotLessThanOther)
{
    auto time1 = 100_us;
    auto time2 = 400_us;
    auto time3 = 1_s + 200_us;
    auto time4 = 1_s + 300_us;
    EXPECT_FALSE(time2 < time1);
    EXPECT_FALSE(time3 < time1);
    EXPECT_FALSE(time3 < time2);
    EXPECT_FALSE(time4 < time3);
}

TEST(Duration_test, CompareDurationIsLessThanOrEqualToOther)
{
    auto time1 = 100_us;
    auto time2 = 400_us;
    auto time3 = 1_s + 200_us;
    auto time4 = 1_s + 300_us;
    EXPECT_TRUE(time1 <= time2);
    EXPECT_TRUE(time1 <= time3);
    EXPECT_TRUE(time2 <= time3);
    EXPECT_TRUE(time3 <= time4);
}

TEST(Duration_test, CompareDurationIsNotLessThanOrEqualToOther)
{
    auto time1 = 100_us;
    auto time2 = 400_us;
    auto time3 = 1_s + 200_us;
    auto time4 = 1_s + 300_us;
    EXPECT_FALSE(time2 <= time1);
    EXPECT_FALSE(time3 <= time1);
    EXPECT_FALSE(time3 <= time2);
    EXPECT_FALSE(time4 <= time3);
}

TEST(Duration_test, CompareDurationIsGreaterThanOther)
{
    auto time1 = 1_s + 300_us;
    auto time2 = 1_s + 200_us;
    auto time3 = 400_us;
    auto time4 = 100_us;
    EXPECT_TRUE(time1 > time2);
    EXPECT_TRUE(time1 > time3);
    EXPECT_TRUE(time2 > time3);
    EXPECT_TRUE(time3 > time4);
}

TEST(Duration_test, CompareDurationIsNotGreaterThanOther)
{
    auto time1 = 1_s + 300_us;
    auto time2 = 1_s + 200_us;
    auto time3 = 400_us;
    auto time4 = 100_us;
    EXPECT_FALSE(time2 > time1);
    EXPECT_FALSE(time3 > time1);
    EXPECT_FALSE(time3 > time2);
    EXPECT_FALSE(time4 > time3);
}

TEST(Duration_test, CompareDurationIsGreaterThanOrEqualToOther)
{
    auto time1 = 1_s + 300_us;
    auto time2 = 1_s + 200_us;
    auto time3 = 400_us;
    auto time4 = 100_us;
    EXPECT_TRUE(time1 >= time2);
    EXPECT_TRUE(time1 >= time3);
    EXPECT_TRUE(time2 >= time3);
    EXPECT_TRUE(time3 >= time4);
}

TEST(Duration_test, CompareDurationIsNotGreaterThanOrEqualToOther)
{
    auto time1 = 1_s + 300_us;
    auto time2 = 1_s + 200_us;
    auto time3 = 400_us;
    auto time4 = 100_us;
    EXPECT_FALSE(time2 >= time1);
    EXPECT_FALSE(time3 >= time1);
    EXPECT_FALSE(time3 >= time2);
    EXPECT_FALSE(time4 >= time3);
}

// END COMPARISON TESTS

// BEGIN ARITHMETIC TESTS

TEST(Duration_test, AddDurationDoesNotChangeOriginalObject)
{
    constexpr Duration EXPECTED_DURATION{13_s + 42_ns};

    auto sut1 = EXPECTED_DURATION;
    auto result1 IOX_MAYBE_UNUSED = sut1 + 15_s;

    auto sut2 = EXPECTED_DURATION;
    auto result2 IOX_MAYBE_UNUSED = 15_s + sut1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithTwoZeroDurationsResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration1 = 0_s;
    auto duration2 = 0_s;

    auto sut = duration1 + duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithOneZeroDurationsResultsInNoneZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{10_ns};
    auto duration1 = 0_s;
    auto duration2 = 10_ns;

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithSumOfDurationsLessThanOneSecondsResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(0U, 100 * NANOSECS_PER_MICROSECOND + 10U);
    auto duration1 = 100_us;
    auto duration2 = 10_ns;

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithSumOfDurationsMoreThanOneSecondsResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(1U, 700 * NANOSECS_PER_MILLISECOND);
    auto duration1 = 800_ms;
    auto duration2 = 900_ms;

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithOneDurationMoreThanOneSecondsResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(2U, 700 * NANOSECS_PER_MILLISECOND);
    auto duration1 = createDuration(1U, 800 * NANOSECS_PER_MILLISECOND);
    auto duration2 = 900_ms;

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationWithDurationsMoreThanOneSecondsResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(3U, 700 * NANOSECS_PER_MILLISECOND);
    auto duration1 = createDuration(1U, 800 * NANOSECS_PER_MILLISECOND);
    auto duration2 = createDuration(1U, 900 * NANOSECS_PER_MILLISECOND);

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationResultsNotYetInSaturation)
{
    constexpr Duration EXPECTED_DURATION =
        createDuration(std::numeric_limits<DurationAccessor::Seconds_t>::max(), NANOSECS_PER_SECOND - 2U);
    auto duration1 =
        createDuration(std::numeric_limits<DurationAccessor::Seconds_t>::max() - 1U, NANOSECS_PER_SECOND - 1U);
    auto duration2 = createDuration(0U, NANOSECS_PER_SECOND - 1U);

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, AddDurationResultsInSaturationFromNanoseconds)
{
    auto duration1 = createDuration(std::numeric_limits<DurationAccessor::Seconds_t>::max(), NANOSECS_PER_SECOND - 2U);
    auto duration2 = createDuration(0U, 2U);

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(DurationAccessor::max()));
    EXPECT_THAT(sut2, Eq(DurationAccessor::max()));
}

TEST(Duration_test, AddDurationResultsInSaturationFromSeconds)
{
    auto duration1 =
        createDuration(std::numeric_limits<DurationAccessor::Seconds_t>::max() - 1U, NANOSECS_PER_SECOND - 1U);
    auto duration2 = createDuration(2U, 0U);

    auto sut1 = duration1 + duration2;
    auto sut2 = duration2 + duration1;

    EXPECT_THAT(sut1, Eq(DurationAccessor::max()));
    EXPECT_THAT(sut2, Eq(DurationAccessor::max()));
}

TEST(Duration_test, SubtractDurationDoesNotChangeOriginalObject)
{
    constexpr Duration EXPECTED_DURATION{13_s + 42_ns};

    auto sut1 = EXPECTED_DURATION;
    auto result1 IOX_MAYBE_UNUSED = sut1 - 5_s;

    auto sut2 = EXPECTED_DURATION;
    auto result2 IOX_MAYBE_UNUSED = 35_s + sut1;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithTwoZeroDurationsResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration1 = 0_s;
    auto duration2 = 0_s;

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithDurationsWithSameValueResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration1 = createDuration(10U, 123U);
    auto duration2 = createDuration(10U, 123U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationFromZeroDurationsResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration0 = 0_s;
    auto duration1 = 10_ns;
    auto duration2 = 10_s;

    auto sut1 = duration0 - duration1;
    auto sut2 = duration0 - duration2;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithLargerDurationsResultsInZeroDurationFromNanoseconds)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration1 = 10_ns;
    auto duration2 = 110_ns;

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithLargerDurationsResultsInZeroDurationFromSeconds)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration1 = createDuration(10U, 123U);
    auto duration2 = createDuration(100U, 123U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithZeroDurationsResultsInOriginaDuration)
{
    constexpr Duration EXPECTED_DURATION = createDuration(10U, 42U);
    auto duration1 = EXPECTED_DURATION;
    auto duration2 = 0_s;

    auto sut = duration1 + duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationMoreThanOneSecondWithLessThanOneSecondResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(1U, 36U);
    auto duration1 = createDuration(1U, 73U);
    auto duration2 = createDuration(0U, 37U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationMoreThanOneSecondWithLessThanOneSecondResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(0U, NANOSECS_PER_SECOND - 36U);
    auto duration1 = createDuration(1U, 37U);
    auto duration2 = createDuration(0U, 73U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationMoreThanOneSecondWithMoreThanOneSecondResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION = createDuration(0U, 36U);
    auto duration1 = createDuration(1U, 73U);
    auto duration2 = createDuration(1U, 37U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, SubtractDurationWithSecondsAndNanosecondsCausingReductionOfSeconds)
{
    constexpr Duration EXPECTED_DURATION = createDuration(0U, NANOSECS_PER_SECOND - 36U);
    auto duration1 = createDuration(2U, 37U);
    auto duration2 = createDuration(1U, 73U);

    auto sut = duration1 - duration2;

    EXPECT_THAT(sut, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, MultiplyDurationDoesNotChangeOriginalObject)
{
    constexpr Duration EXPECTED_DURATION{13_s + 42_ns};

    auto sut1 = EXPECTED_DURATION;
    auto result1 IOX_MAYBE_UNUSED = sut1 * 0;

    auto sut2 = EXPECTED_DURATION;
    auto result2 IOX_MAYBE_UNUSED = sut2 * 0;

    EXPECT_THAT(sut1, Eq(EXPECTED_DURATION));
    EXPECT_THAT(sut2, Eq(EXPECTED_DURATION));
}

template <typename T>
void multiply(Duration duration, T multiplicator, const Duration expectedDuration)
{
    auto sut1 = duration * multiplicator;
    auto sut2 = multiplicator * duration;

    EXPECT_THAT(sut1, Eq(expectedDuration));
    EXPECT_THAT(sut2, Eq(expectedDuration));
}

TEST(Duration_test, MultiplyZeroDurationWithZeroSignedMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 0_s;

    multiply(duration, 0, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyZeroDurationWithZeroUnsignedMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 0_s;

    multiply(duration, 0U, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyZeroDurationWithZeroFloatMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 0_s;

    multiply(duration, 0.0, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationWithZeroSignedMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 1_s + 12_ns;

    multiply(duration, 0, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationWithZeroUnsignedMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 1_s + 12_ns;

    multiply(duration, 0U, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationWithZeroFloatMultiplicatorResultsInZeroDuration)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 1_s + 12_ns;

    multiply(duration, 0.0, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithSignedResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{36_ns};
    auto duration = 12_ns;

    multiply(duration, 3, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithUnsignedResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{36_ns};
    auto duration = 12_ns;

    multiply(duration, 3U, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithFloatResultsInLessThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{42_ns};
    auto duration = 12_ns;

    multiply(duration, 3.5, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithSignedResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{1_s + 800_ms};
    auto duration = 600_ms;

    multiply(duration, 3, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithUnsignedResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{1_s + 800_ms};
    auto duration = 600_ms;

    multiply(duration, 3U, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondWithFloatResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{2_s + 100_ms};
    auto duration = 600_ms;

    multiply(duration, 3.5, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationMoreThanOneSecondWithSignedResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{13_s + 800_ms};
    auto duration = 4_s + 600_ms;

    multiply(duration, 3, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationMoreThanOneSecondWithUnsignedResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{13_s + 800_ms};
    auto duration = 4_s + 600_ms;

    multiply(duration, 3U, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationMoreThanOneSecondWithFloatResultsInMoreThanOneSecond)
{
    constexpr Duration EXPECTED_DURATION{16_s + 100_ms};
    auto duration = 4_s + 600_ms;

    multiply(duration, 3.5, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationWithFractionalFloat)
{
    constexpr Duration EXPECTED_DURATION{2_s + 800_ms};
    auto duration = 5_s + 600_ms;

    multiply(duration, 0.5, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationWithNegativMultiplicatorResultsInZero)
{
    constexpr Duration EXPECTED_DURATION{0_s};
    auto duration = 4_s + 600_ms;

    multiply(duration, -1, EXPECTED_DURATION);
    multiply(duration, -1.0, EXPECTED_DURATION);
}

TEST(Duration_test, MultiplyDurationLessThanOneSecondResultsInMoreNanosecondsThan64BitCanRepresent)
{
    constexpr uint64_t MULTIPLICATOR{(1ULL << 32U) * 42U + 73U};
    constexpr Duration DURATION = 473_ms + 578_us + 511_ns;
    auto EXPECTED_RESULT = createDuration(85428177141U, 573034055U);

    auto result = MULTIPLICATOR * DURATION;
    EXPECT_THAT(result, Eq(EXPECTED_RESULT));
    EXPECT_THAT(result.toNanoseconds(), Eq(std::numeric_limits<uint64_t>::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(EXPECTED_RESULT));
}

TEST(Duration_test, MultiplyDurationResultsNotYetInSaturation)
{
    constexpr uint64_t MULTIPLICATOR{1343535617188545796U};
    constexpr Duration DURATION = 13_s + 730_ms + 37_ns;
    constexpr Duration EXPECTED_DURATION =
        createDuration(std::numeric_limits<DurationAccessor::Seconds_t>::max(), 56194452U);
    static_assert(EXPECTED_DURATION < DurationAccessor::max(),
                  "EXPECTED_DURATION too large to exclude saturation! Please decrease!");

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(EXPECTED_DURATION));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, MultiplyDurationResultsInSaturationDueToSeconds)
{
    constexpr uint64_t MULTIPLICATOR{1343535617188545797U};
    constexpr Duration DURATION = 14_s;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyDurationResultsInSaturationDueToNanoseconds)
{
    constexpr uint64_t MULTIPLICATOR{1343535617188545797U};
    constexpr Duration DURATION = 13_s + 730_ms + 37_ns;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyZeroDurationWithNaNDoubleResultsInZeroDuration)
{
    EXPECT_THAT(0_s * NAN, Eq(0_s));
}

TEST(Duration_test, MultiplyMaxDurationWithNaNDoubleResultsInMaxDuration)
{
    EXPECT_THAT(DurationAccessor::max() * NAN, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyZeroDurationWithPosInfDoubleResultsInZeroDuration)
{
    EXPECT_THAT(0_s * INFINITY, Eq(0_ns));
}

TEST(Duration_test, MultiplyMaxDurationWithPosInfDoubleResultsInMaxDuration)
{
    EXPECT_THAT(DurationAccessor::max() * INFINITY, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyZeroDurationWithNegInfDoubleResultsInZeroDuration)
{
    EXPECT_THAT(0_s * (INFINITY * -1.0), Eq(0_ns));
}

TEST(Duration_test, MultiplyMaxDurationWithNegInfDoubleResultsInZeroDuration)
{
    EXPECT_THAT(DurationAccessor::max() * (INFINITY * -1.0), Eq(0_ns));
}

TEST(Duration_test, MultiplyDurationWithMinimalFloatResultsInZero)
{
    constexpr float MULTIPLICATOR{std::numeric_limits<float>::min()};
    constexpr Duration DURATION = 13_s + 730_ms + 37_ns;
    auto EXPECTED_DURATION = createDuration(0U, 0U);

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(EXPECTED_DURATION));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, MultiplyDurationWithMinimalDoubleResultsInZero)
{
    constexpr double MULTIPLICATOR{std::numeric_limits<double>::min()};
    constexpr Duration DURATION = 13_s + 730_ms + 37_ns;
    auto EXPECTED_DURATION = createDuration(0U, 0U);

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(EXPECTED_DURATION));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(EXPECTED_DURATION));
}

TEST(Duration_test, MultiplyMaxDurationWithFloatOneResultsInMaxDuration)
{
    EXPECT_THAT(DurationAccessor::max() * 1.0, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyMaxDurationWithDoubleOneResultsInMaxDuration)
{
    EXPECT_THAT(DurationAccessor::max() * 1.0, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyDurationWithFloatResultsInSaturationDueToSeconds)
{
    constexpr float MULTIPLICATOR{1343535617188545797.0};
    constexpr Duration DURATION = 14_s;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyDurationWithDoubleResultsInSaturationDueToSeconds)
{
    constexpr double MULTIPLICATOR{1343535617188545797.0};
    constexpr Duration DURATION = 14_s;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyDurationWithFloatResultsInSaturationDueToNanoseconds)
{
    constexpr float MULTIPLICATOR{1343535617188545797.0};
    constexpr Duration DURATION = 13_s + 930_ms + 37_ns;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, MultiplyDurationWithDoubleResultsInSaturationDueToNanoseconds)
{
    constexpr double MULTIPLICATOR{1343535617188545797.0};
    constexpr Duration DURATION = 13_s + 930_ms + 37_ns;

    EXPECT_THAT(MULTIPLICATOR * DURATION, Eq(DurationAccessor::max()));
    EXPECT_THAT(DURATION * MULTIPLICATOR, Eq(DurationAccessor::max()));
}

TEST(Duration_test, StreamingOperator)
{
    std::stringstream capture;
    auto clogBuffer = std::clog.rdbuf();
    std::clog.rdbuf(capture.rdbuf());

    capture.str("");
    std::clog << 0_s;
    EXPECT_STREQ(capture.str().c_str(), "0s 0ns");

    capture.str("");
    std::clog << 42_ns;
    EXPECT_STREQ(capture.str().c_str(), "0s 42ns");

    capture.str("");
    std::clog << (13_s + 73_ms + 37_us + 42_ns);
    EXPECT_STREQ(capture.str().c_str(), "13s 73037042ns");

    std::clog.rdbuf(clogBuffer);
}

// END ARITHMETIC TESTS
} // namespace
