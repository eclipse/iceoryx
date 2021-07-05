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

#ifndef IOX_HOOFS_PERIODIC_TIMER_HPP
#define IOX_HOOFS_PERIODIC_TIMER_HPP

#include "iceoryx_hoofs/error_handling/error_handling.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"
#include "iceoryx_hoofs/posix_wrapper/semaphore.hpp"
#include <thread>

namespace iox
{
namespace posix
{
/// @brief constant holds the acquired value of binary semaphore 0
static constexpr int SEM_ACQUIRED = 0;

/// @brief This enum class offers the timer error codes
enum class TimerErrorCause
{
    INVALID_ARGUMENTS,
    NO_PERMISSION,
    INVALID_POINTER,
    INTERNAL_LOGIC_ERROR,
    TICK_EXCEEDED_TIME_LIMIT,
    NO_ERROR,
    INVALID_STATE
};

/// @brief This enum class offers the timer state events
enum class TimerState : uint8_t
{
    STOP, /// if the Timer is disabled
    TICK, /// if the Timer is executing
    DELAY /// if the Timer is delayed
};

///@brief This enum class offers the various policy to handle the delays
enum class TimerCatchupPolicy : uint8_t
{
    IMMEDIATE_TICK,    /// if there is delay in execution then immediate next activation
    SKIP_TO_NEXT_TICK, /// if there is a delay in execution then the next activation is done in next slot
    HOLD_ON_DELAY      // if there is a delay the activation is not calculated and duration of delay is returned
};

struct WaitResult
{
    TimerState state;
    iox::units::Duration timeDelay;
};

/// @brief This class offers periodic timer functionality. This timer is started immediately upon construction.
/// The periodic timer waits for the duration specified as interval before it comes to execution again. The periodicity
/// is ensured based on the delay calculation.
/// @code
///     iox::posix::PeriodicTimer periodicTimer(1000_ms);
///
///     // to do the execution periodically
///     while(...){
///     ...
///     periodicTimer.wait();
///     }
///     // to stop the timer
///     periodicTimer.stop();
///
/// @endcode
class PeriodicTimer
{
  public:
    /// @brief Constructor
    /// @param[in] interval duration until the timer sleeps and wakes up for execution
    PeriodicTimer(const iox::units::Duration interval) noexcept;

    /// @brief Stops and joins the thread spawned by the constructor.
    virtual ~PeriodicTimer() noexcept = default;

    /// @brief (re-)Starts the timer. This also calculates the time until the timer goes to sleep and next time of
    /// activation. This also acquires the binary semaphore.
    void start() noexcept;

    /// @brief Starts the timer. This also calculates the time until the timer goes for sleep. This also acquires the
    /// binary semaphore.
    /// @param[in] interval The new duration until the timer sleeps and wakes up for execution
    void start(const units::Duration interval) noexcept;

    /// @brief Stops the timer. This also releases the acquired binary semaphore.
    void stop() noexcept;

    /// @brief This function returns the current time.
    /// @return The system clock real time is returned.
    static cxx::expected<units::Duration, TimerErrorCause> now() noexcept;

    /// @brief Briefly waits for the timer interval. This is achieved by trying to do a timed wait for the interval
    /// duration on the already acquired binary semaphore.
    /// @return WaitResult with timer state of TICK if the timer is active and running, STOP if the timer is stopped
    /// already, DELAY if the execution time crossed the next activation time. If the timer state is DELAY the actual
    /// delay duration is set to the WaitResult.
    cxx::expected<iox::posix::WaitResult, TimerErrorCause> wait(TimerCatchupPolicy policy) noexcept;

  private:
    iox::units::Duration m_interval{units::Duration::fromSeconds(0)};
    iox::units::Duration m_timeForNextActivation{units::Duration::fromSeconds(0)};
    posix::Semaphore m_waitSemaphore;
    static cxx::error<TimerErrorCause> createErrorCodeFromErrNo(const int32_t errnum) noexcept;
    iox::posix::WaitResult m_waitResult{iox::posix::TimerState::TICK, units::Duration::fromSeconds(0)};
};

} // namespace posix
} // namespace iox

#endif // IOX_HOOFS_PERIODIC_TIMER_HPP
