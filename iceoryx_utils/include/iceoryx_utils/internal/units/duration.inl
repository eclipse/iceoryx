// Copyright (c) 2019, 2021 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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
#ifndef IOX_UTILS_UNITS_DURATION_INL
#define IOX_UTILS_UNITS_DURATION_INL

#include "iceoryx_utils/internal/units/duration.hpp"

namespace iox
{
namespace units
{
inline constexpr Duration::Duration(const Seconds_t seconds, const Nanoseconds_t nanoseconds) noexcept
    : m_seconds(seconds)
    , m_nanoseconds(nanoseconds)
{
    if (nanoseconds >= NANOSECS_PER_SEC)
    {
        auto additionalSeconds = nanoseconds / NANOSECS_PER_SEC;
        if (std::numeric_limits<Seconds_t>::max() - additionalSeconds < m_seconds)
        {
            /// @todo #607 issue warning or fail

            m_seconds = std::numeric_limits<Seconds_t>::max();
            m_nanoseconds = NANOSECS_PER_SEC - 1U;
        }
        else
        {
            m_seconds += additionalSeconds;
            m_nanoseconds = m_nanoseconds % NANOSECS_PER_SEC;
        }
    }
}

inline constexpr Duration Duration::createDuration(const Seconds_t seconds, const Nanoseconds_t nanoseconds) noexcept
{
    return Duration(seconds, nanoseconds);
}

inline constexpr Duration Duration::max() noexcept
{
    return Duration{std::numeric_limits<Seconds_t>::max(), NANOSECS_PER_SEC - 1U};
}

inline constexpr Duration Duration::zero() noexcept
{
    return Duration{0U, 0U};
}

template <typename T, typename String>
inline constexpr unsigned long long int Duration::positiveValueOrClampToZero(const T value, const String) noexcept
{
    static_assert(std::numeric_limits<T>::is_integer, "only integer types are supported");

    if (value < 0)
    {
        /// @todo #607 issue warning or fail

        return 0U;
    }

    return static_cast<unsigned long long int>(value);
}

template <typename T>
constexpr Duration Duration::fromNanoseconds(const T value) noexcept
{
    return operator"" _ns(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromMicroseconds(const T value) noexcept
{
    return operator"" _us(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromMilliseconds(const T value) noexcept
{
    return operator"" _ms(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromSeconds(const T value) noexcept
{
    return operator"" _s(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromMinutes(const T value) noexcept
{
    return operator"" _m(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromHours(const T value) noexcept
{
    return operator"" _h(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}
template <typename T>
constexpr Duration Duration::fromDays(const T value) noexcept
{
    return operator"" _d(positiveValueOrClampToZero(value, __PRETTY_FUNCTION__));
}

inline constexpr Duration::Duration(const struct timeval& value) noexcept
    : Duration(static_cast<Seconds_t>(value.tv_sec), static_cast<Nanoseconds_t>(value.tv_usec) * NANOSECS_PER_MICROSEC)
{
}

inline constexpr Duration::Duration(const struct timespec& value) noexcept
    : Duration(static_cast<Seconds_t>(value.tv_sec), static_cast<Nanoseconds_t>(value.tv_nsec))
{
}

inline constexpr Duration::Duration(const struct itimerspec& value) noexcept
    : Duration(value.it_interval)
{
}

inline constexpr Duration::Duration(const std::chrono::milliseconds& value) noexcept
    : Duration(Duration::fromMilliseconds(value.count()))
{
}

inline constexpr Duration::Duration(const std::chrono::nanoseconds& value) noexcept
    : Duration(Duration::fromNanoseconds(value.count()))
{
}

inline Duration& Duration::operator=(const std::chrono::milliseconds& rhs) noexcept
{
    *this = Duration(rhs);
    return *this;
}

inline constexpr uint64_t Duration::toNanoseconds() const noexcept
{
    constexpr Seconds_t MAX_SECONDS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / NANOSECS_PER_SEC};
    constexpr Nanoseconds_t MAX_NANOSECONDS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() % NANOSECS_PER_SEC};
    constexpr Duration MAX_DURATION_BEFORE_OVERFLOW =
        createDuration(MAX_SECONDS_BEFORE_OVERFLOW, MAX_NANOSECONDS_BEFORE_OVERFLOW);

    if (*this > MAX_DURATION_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return std::numeric_limits<uint64_t>::max();
    }

    return m_seconds * NANOSECS_PER_SEC + m_nanoseconds;
}

inline constexpr uint64_t Duration::toMicroseconds() const noexcept
{
    constexpr Seconds_t MAX_SECONDS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / MICROSECS_PER_SEC};
    constexpr Nanoseconds_t MAX_NANOSECONDS_BEFORE_OVERFLOW{(std::numeric_limits<uint64_t>::max() % MICROSECS_PER_SEC)
                                                            * NANOSECS_PER_MICROSEC};
    constexpr Duration MAX_DURATION_BEFORE_OVERFLOW =
        createDuration(MAX_SECONDS_BEFORE_OVERFLOW, MAX_NANOSECONDS_BEFORE_OVERFLOW);

    if (*this > MAX_DURATION_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return std::numeric_limits<uint64_t>::max();
    }

    return m_seconds * MICROSECS_PER_SEC + m_nanoseconds / NANOSECS_PER_MICROSEC;
}

inline constexpr uint64_t Duration::toMilliseconds() const noexcept
{
    constexpr Seconds_t MAX_SECONDS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / MILLISECS_PER_SEC};
    constexpr Nanoseconds_t MAX_NANOSECONDS_BEFORE_OVERFLOW{(std::numeric_limits<uint64_t>::max() % MILLISECS_PER_SEC)
                                                            * NANOSECS_PER_MILLISEC};
    constexpr Duration MAX_DURATION_BEFORE_OVERFLOW =
        createDuration(MAX_SECONDS_BEFORE_OVERFLOW, MAX_NANOSECONDS_BEFORE_OVERFLOW);

    if (*this > MAX_DURATION_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return std::numeric_limits<uint64_t>::max();
    }

    return m_seconds * MILLISECS_PER_SEC + m_nanoseconds / NANOSECS_PER_MILLISEC;
}

inline constexpr uint64_t Duration::toSeconds() const noexcept
{
    return m_seconds;
}

inline constexpr uint64_t Duration::toMinutes() const noexcept
{
    return m_seconds / SECS_PER_MINUTE;
}

inline constexpr uint64_t Duration::toHours() const noexcept
{
    return m_seconds / SECS_PER_HOUR;
}

inline constexpr uint64_t Duration::toDays() const noexcept
{
    return m_seconds / (HOURS_PER_DAY * SECS_PER_HOUR);
}

inline constexpr Duration::operator timeval() const noexcept
{
    using SEC_TYPE = decltype(timeval::tv_sec);
    using USEC_TYPE = decltype(timeval::tv_usec);
    static_assert(sizeof(Seconds_t) >= sizeof(SEC_TYPE), "casting might alter result");
    if (m_seconds > static_cast<Seconds_t>(std::numeric_limits<SEC_TYPE>::max()))
    {
        /// @todo #607 issue warning or fail

        return {std::numeric_limits<SEC_TYPE>::max(), MICROSECS_PER_SEC - 1U};
    }
    return {static_cast<SEC_TYPE>(m_seconds), static_cast<USEC_TYPE>(m_nanoseconds / NANOSECS_PER_MICROSEC)};
}

inline constexpr bool Duration::operator==(const Duration& rhs) const noexcept
{
    return (m_seconds == rhs.m_seconds) && (m_nanoseconds == rhs.m_nanoseconds);
}

inline constexpr bool Duration::operator!=(const Duration& rhs) const noexcept
{
    return !(*this == rhs);
}

inline constexpr bool Duration::operator<(const Duration& rhs) const noexcept
{
    return (m_seconds < rhs.m_seconds) || ((m_seconds == rhs.m_seconds) && (m_nanoseconds < rhs.m_nanoseconds));
}

inline constexpr bool Duration::operator<=(const Duration& rhs) const noexcept
{
    return !(*this > rhs);
}

inline constexpr bool Duration::operator>(const Duration& rhs) const noexcept
{
    return (m_seconds > rhs.m_seconds) || ((m_seconds == rhs.m_seconds) && (m_nanoseconds > rhs.m_nanoseconds));
}

inline constexpr bool Duration::operator>=(const Duration& rhs) const noexcept
{
    return !(*this < rhs);
}

inline constexpr Duration Duration::operator+(const Duration& rhs) const noexcept
{
    auto seconds = m_seconds + rhs.m_seconds;
    auto nanoseconds = m_nanoseconds + rhs.m_nanoseconds;
    if (nanoseconds >= NANOSECS_PER_SEC)
    {
        ++seconds;
        nanoseconds -= NANOSECS_PER_SEC;
    }

    auto sum = Duration{seconds, nanoseconds};
    if (sum < *this)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    return sum;
}

inline constexpr Duration Duration::operator-(const Duration& rhs) const noexcept
{
    if (*this < rhs)
    {
        /// @todo #607 issue warning or fail

        return Duration::zero();
    }
    auto seconds = m_seconds - rhs.m_seconds;
    Nanoseconds_t nanoseconds{0U};
    if (m_nanoseconds >= rhs.m_nanoseconds)
    {
        nanoseconds = m_nanoseconds - rhs.m_nanoseconds;
    }
    else
    {
        nanoseconds = NANOSECS_PER_SEC - rhs.m_nanoseconds + m_nanoseconds;
        --seconds;
    }
    return Duration{seconds, nanoseconds};
}

template <typename T>
inline constexpr Duration
Duration::multiplyWith(const std::enable_if_t<!std::is_floating_point<T>::value, T>& rhs) const noexcept
{
    // operator*(...) takes care of negative values and 0 for rhs

    static_assert(sizeof(T) <= sizeof(Seconds_t),
                  "only integer types with less or equal to size of uint64_t are allowed for multiplication");
    const auto multiplicator = static_cast<Seconds_t>(rhs);

    const auto maxBeforeOverflow = std::numeric_limits<Seconds_t>::max() / multiplicator;

    // check if the result of the m_seconds multiplication would already overflow
    if (m_seconds > maxBeforeOverflow)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    auto durationFromSeconds = Duration(m_seconds * multiplicator, 0U);

    // the m_nanoseconds multiplication cannot exceed the limits of a Duration, since m_nanoseconds is always less than
    // a second and m_seconds can hold 64 bits and the multiplicator is at max 64 bits

    // check if the result of the m_nanoseconds multiplication can easily be converted into a Duration
    if (m_nanoseconds <= maxBeforeOverflow)
    {
        return durationFromSeconds + Duration::fromNanoseconds(m_nanoseconds * multiplicator);
    }

    // when we reach this, the multiplicator must be larger than 2^32, since smaller values multiplied with the
    // m_nanoseconds(uint32_t) would fit into 64 bits;
    // to accurately determine the result, the calculation is split into a multiplication with the lower 32 bits of the
    // multiplicator and another one with the upper 32 bits;

    // this is the easy part with the lower 32 bits
    uint64_t multiplicatorLow = static_cast<uint32_t>(multiplicator);
    auto durationFromNanosecondsLow = Duration::fromNanoseconds(m_nanoseconds * multiplicatorLow);

    // this is the complicated part with the upper 32 bits;
    // the m_nanoseconds are multiplied with the upper 32 bits of the multiplicator shifted by 32 bit to the right, thus
    // having again a multiplication of two 32 bit values whose result fits into a 64 bit variable;
    // one bit of the result represents 2^32 nanoseconds;
    // just shifting left by 32 bits would result in an overflow, therefore blocks of full seconds must be extracted of
    // the result;
    // this cannot be done by dividing through NANOSECS_PER_SEC, since that one is base 1_000_000_000 and the result is
    // base 2^32, therefore the least common multiple can be used to get blocks of full seconds represented with the LSB
    // representing 2^32 nanoseconds;
    // this can then safely be converted to seconds as well as nanoseconds without loosing precision

    // least common multiple of 2^32 and NANOSECONDS_PER_SECOND;
    // for the following calculation it is not important to be the least common multiple, any common multiple will do
    constexpr uint64_t LEAST_COMMON_MULTIPLE{8388608000000000};
    static_assert(LEAST_COMMON_MULTIPLE % (1ULL << 32) == 0, "invalid multiple");
    static_assert(LEAST_COMMON_MULTIPLE % NANOSECS_PER_SEC == 0, "invalid multiple");

    constexpr uint64_t ONE_FULL_BLOCK_OF_SECONDS_ONLY{LEAST_COMMON_MULTIPLE >> 32};
    constexpr uint64_t SECONDS_PER_FULL_BLOCK{LEAST_COMMON_MULTIPLE / NANOSECS_PER_SEC};

    uint64_t multiplicatorHigh = static_cast<uint32_t>(multiplicator >> 32U);
    auto nanosecondsFromHigh = m_nanoseconds * multiplicatorHigh;
    auto fullBlocksOfSecondsOnly = nanosecondsFromHigh / ONE_FULL_BLOCK_OF_SECONDS_ONLY;
    auto remainingBlockWithFullAndFractionalSeconds = nanosecondsFromHigh % ONE_FULL_BLOCK_OF_SECONDS_ONLY;

    auto durationFromNanosecondsHigh = Duration{fullBlocksOfSecondsOnly * SECONDS_PER_FULL_BLOCK, 0U}
                                       + Duration::fromNanoseconds(remainingBlockWithFullAndFractionalSeconds << 32);

    return durationFromSeconds + durationFromNanosecondsLow + durationFromNanosecondsHigh;
}


template <typename From, typename To>
inline constexpr bool Duration::wouldCastFromFloatingPointProbablyOverflow(const From floatingPoint) const noexcept
{
    static_assert(std::is_floating_point<From>::value, "only floating point is allowed");

    // depending on the internal representation this could be either the last value to not cause an overflow
    // or the first one which causes an overflow;
    // to be safe, this is handled like causing an overflow which would result in undefined behavior when casting to
    // Seconds_t
    constexpr To SECONDS_BEFORE_LIKELY_OVERFLOW = static_cast<To>(std::numeric_limits<Seconds_t>::max());
    return floatingPoint >= SECONDS_BEFORE_LIKELY_OVERFLOW;
}

template <typename T>
inline constexpr Duration Duration::fromFloatingPointSeconds(const T floatingPointSeconds) const noexcept
{
    static_assert(std::is_floating_point<T>::value, "only floating point is allowed");

    if (std::isinf(floatingPointSeconds))
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }

    double secondsFull{0.0};
    double secondsFraction = modf(floatingPointSeconds, &secondsFull);

    if (wouldCastFromFloatingPointProbablyOverflow<T, Seconds_t>(secondsFull))
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }

    return Duration{static_cast<Seconds_t>(secondsFull),
                    static_cast<Nanoseconds_t>(secondsFraction * NANOSECS_PER_SEC)};
}

template <typename T>
inline constexpr Duration Duration::multiplyWith(const std::enable_if_t<std::is_floating_point<T>::value, T>& rhs) const
    noexcept
{
    // operator*(...) takes care of negative values for rhs

    if (std::isnan(rhs))
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }

    auto durationFromSeconds = fromFloatingPointSeconds(m_seconds * rhs);

    auto resultNanoseconds = m_nanoseconds * rhs;

    if (!wouldCastFromFloatingPointProbablyOverflow<T, uint64_t>(resultNanoseconds))
    {
        return durationFromSeconds + Duration::fromNanoseconds(static_cast<uint64_t>(resultNanoseconds));
    }

    // the multiplication result of nanoseconds would exceed the value an uint64_t can represent
    // -> convert result to seconds and and calculate duration
    auto durationFromNanoseconds = fromFloatingPointSeconds(resultNanoseconds /= NANOSECS_PER_SEC);

    return durationFromSeconds + durationFromNanoseconds;
}

template <typename T>
inline constexpr Duration Duration::operator*(const T& rhs) const noexcept
{
    static_assert(std::is_arithmetic<T>::value, "non arithmetic types are not supported for multiplication");

    if (rhs <= static_cast<T>(0) || *this == Duration::zero())
    {
        if (rhs < static_cast<T>(0))
        {
            /// @todo #607 issue warning or fail
        }
        return Duration::zero();
    }

    return multiplyWith<T>(rhs);
}

inline namespace duration_literals
{
inline constexpr Duration operator"" _ns(unsigned long long int value) noexcept // PRQA S 48
{
    auto seconds = static_cast<Duration::Seconds_t>(value / Duration::NANOSECS_PER_SEC);
    auto nanoseconds = static_cast<Duration::Nanoseconds_t>(value % Duration::NANOSECS_PER_SEC);
    return Duration{seconds, nanoseconds};
}

inline constexpr Duration operator"" _us(unsigned long long int value) noexcept // PRQA S 48
{
    auto seconds = static_cast<Duration::Seconds_t>(value / Duration::MICROSECS_PER_SEC);
    auto nanoseconds =
        static_cast<Duration::Nanoseconds_t>((value % Duration::MICROSECS_PER_SEC) * Duration::NANOSECS_PER_MICROSEC);
    return Duration{seconds, nanoseconds};
}

inline constexpr Duration operator"" _ms(unsigned long long int value) noexcept // PRQA S 48
{
    auto seconds = static_cast<Duration::Seconds_t>(value / Duration::MILLISECS_PER_SEC);
    auto nanoseconds =
        static_cast<Duration::Nanoseconds_t>((value % Duration::MILLISECS_PER_SEC) * Duration::NANOSECS_PER_MILLISEC);
    return Duration{seconds, nanoseconds};
}

inline constexpr Duration operator"" _s(unsigned long long int value) noexcept // PRQA S 48
{
    constexpr Duration::Seconds_t MAX_SECONDS_BEFORE_OVERFLOW{std::numeric_limits<Duration::Seconds_t>::max()};
    if (value > MAX_SECONDS_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    return Duration{static_cast<Duration::Seconds_t>(value), 0U};
}

inline constexpr Duration operator"" _m(unsigned long long int value) noexcept // PRQA S 48
{
    constexpr uint64_t MAX_MINUTES_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / Duration::SECS_PER_MINUTE};
    if (value > MAX_MINUTES_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    return Duration{static_cast<Duration::Seconds_t>(value * Duration::SECS_PER_MINUTE), 0U};
}

inline constexpr Duration operator"" _h(unsigned long long int value) noexcept // PRQA S 48
{
    constexpr uint64_t MAX_HOURS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / Duration::SECS_PER_HOUR};
    if (value > MAX_HOURS_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    return Duration{static_cast<Duration::Seconds_t>(value * Duration::SECS_PER_HOUR), 0U};
}

inline constexpr Duration operator"" _d(unsigned long long int value) noexcept // PRQA S 48
{
    constexpr uint64_t SECS_PER_DAY{Duration::HOURS_PER_DAY * Duration::SECS_PER_HOUR};
    constexpr uint64_t MAX_DAYS_BEFORE_OVERFLOW{std::numeric_limits<uint64_t>::max() / SECS_PER_DAY};
    if (value > MAX_DAYS_BEFORE_OVERFLOW)
    {
        /// @todo #607 issue warning or fail

        return Duration::max();
    }
    return Duration{static_cast<Duration::Seconds_t>(value * SECS_PER_DAY), 0U};
}

} // namespace duration_literals

template <typename T>
inline constexpr Duration operator*(const T& lhs, const Duration& rhs) noexcept
{
    return rhs * lhs;
}

} // namespace units
} // namespace iox

#endif // IOX_UTILS_UNITS_DURATION_INL
