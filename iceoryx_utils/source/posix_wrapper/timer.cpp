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

#include "iceoryx_utils/posix_wrapper/timer.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/platform/platform_correction.hpp"
#include <atomic>

namespace iox
{
namespace posix
{
Timer::OsTimerCallbackHandle Timer::OsTimer::s_callbackHandlePool[MAX_NUMBER_OF_CALLBACK_HANDLES];

sigval Timer::OsTimerCallbackHandle::indexAndDescriptorToSigval(uint8_t index, uint32_t descriptor) noexcept
{
    assert(descriptor < MAX_DESCRIPTOR_VALUE);
    uint32_t temp = (descriptor << 8) | static_cast<uint32_t>(index);
    sigval sigvalData;
    sigvalData.sival_int = static_cast<int>(temp);
    return sigvalData;
}

uint8_t Timer::OsTimerCallbackHandle::sigvalToIndex(sigval intVal) noexcept
{
    return static_cast<uint8_t>(0xFF & intVal.sival_int);
}

uint32_t Timer::OsTimerCallbackHandle::sigvalToDescriptor(sigval intVal) noexcept
{
    uint32_t temp = static_cast<uint32_t>(intVal.sival_int);
    return (temp >> 8u) & 0xFFFFFFu;
}

void Timer::OsTimerCallbackHandle::incrementDescriptor() noexcept
{
    auto callbackHandleDescriptor = m_descriptor.load(std::memory_order_relaxed);
    callbackHandleDescriptor++;
    if (callbackHandleDescriptor >= Timer::OsTimerCallbackHandle::MAX_DESCRIPTOR_VALUE)
    {
        callbackHandleDescriptor = 0u;
    }

    m_descriptor.store(callbackHandleDescriptor, std::memory_order_relaxed);
}

void Timer::OsTimer::callbackHelper(sigval data)
{
    uint32_t index = Timer::OsTimerCallbackHandle::sigvalToIndex(data);
    auto descriptor = Timer::OsTimerCallbackHandle::sigvalToDescriptor(data);

    if (index >= Timer::OsTimerCallbackHandle::MAX_DESCRIPTOR_VALUE)
    {
        ///@todo decide if to print a warning
        return;
    }

    auto& handle = OsTimer::s_callbackHandlePool[index];

    // small optimization to not lock the mutex if the callback handle is not valid anymore
    if (descriptor != handle.m_descriptor.load(std::memory_order::memory_order_relaxed))
    {
        return;
    }

    // signal that we intend to call the callback (but we may not start it ourself, it may be done
    // by another invocation of this function)
    auto invocationCounter = handle.m_timerInvocationCounter.fetch_add(1u, std::memory_order_relaxed);

    // avoid the spurious failures of try_lock with this flag end still reduce contention
    // if another thread is in this section we return but we already stated our intention to call the callback
    // via m_timerInvocationCounter and the other thread will do this for us
    // note that thte invocation may be lost if the callback is already running, this is intentional
    // to avoid an unlimited number of callback invocations to pile up

    // note: if we can tolerate those calls to pile up we can get rid of this flag and the while loop as well

    if (!handle.m_callbackIsAboutToBeExecuted.test_and_set(std::memory_order_acq_rel))
    {
        // flag protected region

        // the mutex is needed to protect against concurrent deletion of the timer in the destructor
        std::lock_guard<std::mutex> lockGuard(handle.m_accessMutex);

        // clear the flag regardless of how we leave the function
        cxx::GenericRAII clearCallbackFlag(
            []() {}, [&]() { handle.m_callbackIsAboutToBeExecuted.clear(std::memory_order_release); });

        do
        {
            // prohibits other threads from entering the flag protected region
            handle.m_callbackIsAboutToBeExecuted.test_and_set(std::memory_order_acq_rel);

            if (handle.m_timer == nullptr)
            {
                errorHandler(Error::kPOSIX_TIMER__INCONSISTENT_STATE);
                return;
            }

            if (!handle.m_inUse.load(std::memory_order::memory_order_relaxed))
            {
                return;
            }

            if (descriptor != handle.m_descriptor.load(std::memory_order::memory_order_relaxed))
            {
                return;
            }

            if (!handle.m_isTimerActive.load(std::memory_order::memory_order_relaxed))
            {
                return;
            }

            invocationCounter = handle.m_timerInvocationCounter.exchange(0u, std::memory_order_acq_rel);

            // did someone else execute the callback for us?
            if (invocationCounter != 0u)
            {
                handle.m_timer->executeCallback();
            }

            handle.m_callbackIsAboutToBeExecuted.clear(std::memory_order_release);
            // another thread can try to enter the flag protected region by setting the flag now but will still need to
            // wait for the mutex (they can pile up in theory)
            // the point is: it must be set to false before the counter comparison of the while loop,
            //  otherwise: another thread could increment the counter *after* this
            // comparison, see the flag is still true, leave and rely on this thread to perform the callback, but for
            // this thread the comparison has seen 0 and it will therefore leave the loop

            // by clearing the flag temporarily this cannot happen (but it has other drawbacks)

            // get the latest value (can be outdated after the call, but this is not important here,
            // we just need updates from concurrent threads which may have incremented the counter)
            handle.m_timerInvocationCounter.compare_exchange_strong(
                invocationCounter, invocationCounter, std::memory_order_acq_rel, std::memory_order_acquire);

            // if the counter is positive it means some other thread has incremented it in the meantime
            // otherwise some thread may be about to do so but then will either find the flag to be cleared
            // and be able to enter the region or the flag to be set again, in which case this thread
            // will take care of the call

        } while (handle.m_catchUpPolicy == CatchUpPolicy::IMMEDIATE && invocationCounter > 0u);
    }
    else
    {
        if (handle.m_catchUpPolicy == CatchUpPolicy::TERMINATE)
        {
            errorHandler(Error::kPOSIX_TIMER__CALLBACK_RUNTIME_EXCEEDS_RETRIGGER_TIME);
        }
        return;
    }
}

Timer::OsTimer::OsTimer(const units::Duration timeToWait, const std::function<void()>& callback) noexcept
    : m_timeToWait(timeToWait)
    , m_callback(callback)
{
    // Is the callback valid?
    if (!callback)
    {
        m_isInitialized = false;
        m_errorValue = TimerError::NO_VALID_CALLBACK;
        return;
    }

    // find OsTimerCallbackHandle not in use
    bool callbackHandleFound = false;
    uint32_t callbackHandleDescriptor = 0u;
    for (auto& callbackHandle : OsTimer::s_callbackHandlePool)
    {
        if (!callbackHandle.m_inUse.load(std::memory_order_relaxed))
        {
            std::lock_guard<std::mutex> lock(callbackHandle.m_accessMutex);
            // check in use again, just in case there we lost the race before we got the lock
            if (callbackHandle.m_inUse.load(std::memory_order_relaxed))
            {
                m_callbackHandleIndex++;
                continue;
            }

            callbackHandle.incrementDescriptor();
            callbackHandle.m_isTimerActive.store(true, std::memory_order_relaxed);
            callbackHandle.m_inUse.store(true, std::memory_order_relaxed);
            callbackHandle.m_timer = this;

            // it is sufficient to set the counter in the constructor
            // (setting it in start leads to a subtle race in the loop of callbackHelper
            // were they are checked should the callback call start)
            callbackHandle.m_timerInvocationCounter.store(0u, std::memory_order_relaxed);

            callbackHandleFound = true;
            callbackHandleDescriptor = callbackHandle.m_descriptor.load(std::memory_order_relaxed);
            break;
        }
        m_callbackHandleIndex++;
    }

    if (!callbackHandleFound)
    {
        errorHandler(Error::kPOSIX_TIMER__TIMERPOOL_OVERFLOW);
    }

    // Create the struct in order to configure the timer in the OS
    struct sigevent asyncCallNotification;
    // We want the timer to call a function
    asyncCallNotification.sigev_notify = SIGEV_THREAD;
    // Set the function pointer to our sigevent
    asyncCallNotification.sigev_notify_function = &callbackHelper;
    // Save the pointer to self in order to execute the callback
    asyncCallNotification.sigev_value.sival_ptr = nullptr; // initialize all bits of the sigval union for mem check
    asyncCallNotification.sigev_value.sival_int =
        Timer::OsTimerCallbackHandle::indexAndDescriptorToSigval(m_callbackHandleIndex, callbackHandleDescriptor)
            .sival_int;
    // Do not set any thread attributes
    asyncCallNotification.sigev_notify_attributes = nullptr;

    auto result = cxx::makeSmartC(timer_create,
                                  cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                  {-1},
                                  {},
                                  CLOCK_REALTIME,
                                  &asyncCallNotification,
                                  &m_timerId);

    if (result.hasErrors())
    {
        m_isInitialized = false;
        m_errorValue = createErrorFromErrno(result.getErrNum()).value;
        m_timerId = INVALID_TIMER_ID;
    }
    else
    {
        m_isInitialized = true;
    }
}

Timer::OsTimer::~OsTimer() noexcept
{
    if (m_timerId != INVALID_TIMER_ID)
    {
        stop().or_else([](auto) { std::cerr << "Unable to stop the timer in the destructor." << std::endl; });

        // do not delete the timer while the callback is running, it could access the timer which is about to be deleted
        auto& callbackHandle = OsTimer::s_callbackHandlePool[m_callbackHandleIndex];
        std::lock_guard<std::mutex> lock(callbackHandle.m_accessMutex);

        auto result = cxx::makeSmartC(timer_delete, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_timerId);

        if (result.hasErrors())
        {
            createErrorFromErrno(result.getErrNum());
            std::cerr << "Unable to cleanup posix::Timer \"" << m_timerId << "\" in the destructor" << std::endl;
        }

        m_timerId = INVALID_TIMER_ID;

        callbackHandle.m_inUse.store(false, std::memory_order_seq_cst);
    }
}

void Timer::OsTimer::executeCallback() noexcept
{
    if (m_isInitialized && m_callback)
    {
        m_callback();
    }
    else
    {
        // Thread couldn't reach callback or object is not correctly initalized, maybe the originial object was a
        // temporary?
        errorHandler(Error::kPOSIX_TIMER__FIRED_TIMER_BUT_STATE_IS_INVALID);
    }
}

cxx::expected<TimerError> Timer::OsTimer::start(const RunMode runMode, const CatchUpPolicy catchUpPolicy) noexcept
{
    // Convert units::Duration to itimerspec
    struct itimerspec interval;
    interval.it_value = m_timeToWait.timespec(units::TimeSpecReference::None);

    if (runMode == RunMode::PERIODIC)
    {
        interval.it_interval = m_timeToWait.timespec(units::TimeSpecReference::None);
    }
    else
    {
        interval.it_interval.tv_sec = 0;
        interval.it_interval.tv_nsec = 0;
    }

    auto& handle = OsTimer::s_callbackHandlePool[m_callbackHandleIndex];
    // setting m_isTimerActive after timer_settime could lead to false negatives in a check which decides whether
    // the callback should be executed,
    // setting it beforehand leads to false positives which can be dealt with and avoid this problem
    auto wasActive = handle.m_isTimerActive.exchange(true, std::memory_order_relaxed);
    handle.m_catchUpPolicy = catchUpPolicy;

    // Set the timer
    auto result = cxx::makeSmartC(
        timer_settime, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_timerId, 0, &interval, nullptr);

    if (result.hasErrors())
    {
        // undo optimistically setting m_isTimerActive before
        // not entirely safe against concurrent starts, we cannot detect these in this way
        // needs extensive refactoring to achieve this (e.g. start and stop with mutex protection?)
        handle.m_isTimerActive.exchange(wasActive, std::memory_order_relaxed);
        return createErrorFromErrno(result.getErrNum());
    }


    return cxx::success<void>();
}

cxx::expected<TimerError> Timer::OsTimer::stop() noexcept
{
    auto& handle = OsTimer::s_callbackHandlePool[m_callbackHandleIndex];
    // Signal callbackHelper() that no callbacks shall be executed anymore
    auto wasActive = handle.m_isTimerActive.exchange(false, std::memory_order_relaxed);

    if (!wasActive)
    {
        // Timer was not started yet
        return cxx::success<void>();
    }

    struct itimerspec interval;
    units::Duration zero = 0_s;
    interval.it_value = zero.timespec(units::TimeSpecReference::None);
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_nsec = 0;


    // Disarm the timer
    auto result = cxx::makeSmartC(
        timer_settime, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_timerId, 0, &interval, nullptr);

    if (result.hasErrors())
    {
        return createErrorFromErrno(result.getErrNum());
    }

    return cxx::success<void>();
}

cxx::expected<TimerError> Timer::OsTimer::restart(const units::Duration timeToWait,
                                                  const RunMode runMode,
                                                  const CatchUpPolicy catchUpPolicy) noexcept
{
    // See if there is currently an active timer in the operating system and update m_isActive accordingly
    auto gettimeResult = timeUntilExpiration();

    if (gettimeResult.has_error())
    {
        return gettimeResult;
    }

    // Set new timeToWait value
    m_timeToWait = timeToWait;

    // Disarm running timer
    if (OsTimer::s_callbackHandlePool[m_callbackHandleIndex].m_isTimerActive.load(std::memory_order_relaxed))
    {
        auto stopResult = stop();

        if (stopResult.has_error())
        {
            return stopResult;
        }
    }

    // Activate the timer with the new timeToWait value
    auto startResult = start(runMode, catchUpPolicy);

    if (startResult.has_error())
    {
        return startResult;
    }
    return cxx::success<void>();
}

cxx::expected<units::Duration, TimerError> Timer::OsTimer::timeUntilExpiration() noexcept
{
    struct itimerspec currentInterval;

    auto result =
        cxx::makeSmartC(timer_gettime, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_timerId, &currentInterval);

    if (result.hasErrors())
    {
        return createErrorFromErrno(result.getErrNum());
    }

    if (currentInterval.it_value.tv_sec == 0 && currentInterval.it_value.tv_nsec == 0)
    {
        // Timer is disarmed
        OsTimer::s_callbackHandlePool[m_callbackHandleIndex].m_isTimerActive.store(false, std::memory_order_relaxed);
    }
    return cxx::success<units::Duration>(currentInterval.it_value);
}

cxx::expected<uint64_t, TimerError> Timer::OsTimer::getOverruns() noexcept
{
    auto result = cxx::makeSmartC(timer_getoverrun, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_timerId);

    if (result.hasErrors())
    {
        return createErrorFromErrno(result.getErrNum());
    }
    return cxx::success<uint64_t>(static_cast<uint64_t>(result.getReturnValue()));
}

bool Timer::OsTimer::hasError() const noexcept
{
    return !m_isInitialized;
}

TimerError Timer::OsTimer::getError() const noexcept
{
    return m_errorValue;
}

cxx::expected<units::Duration, TimerError> Timer::now() noexcept
{
    struct timespec value;
    auto result =
        cxx::makeSmartC(clock_gettime, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, CLOCK_REALTIME, &value);

    if (result.hasErrors())
    {
        return createErrorFromErrno(result.getErrNum());
    }

    return cxx::success<units::Duration>(value);
}

Timer::Timer(const units::Duration timeToWait) noexcept
    : m_timeToWait(timeToWait)
    , m_creationTime(now().value())
{
    if (m_timeToWait.toNanoseconds() == 0U)
    {
        m_errorValue = TimerError::TIMEOUT_IS_ZERO;
    }
}

Timer::Timer(const units::Duration timeToWait, const std::function<void()>& callback) noexcept
    : m_timeToWait(timeToWait)
    , m_creationTime(now().value())
{
    if (m_timeToWait.toNanoseconds() == 0U)
    {
        m_errorValue = TimerError::TIMEOUT_IS_ZERO;
        return;
    }

    m_osTimer.emplace(timeToWait, callback);
    if (m_osTimer->hasError())
    {
        m_errorValue = m_osTimer->getError();
        m_osTimer.reset();
    }
}

cxx::expected<TimerError> Timer::start(const RunMode runMode, const CatchUpPolicy catchUpPolicy) noexcept
{
    if (!m_osTimer.has_value())
    {
        return cxx::error<TimerError>(TimerError::TIMER_NOT_INITIALIZED);
    }

    return m_osTimer->start(runMode, catchUpPolicy);
}

cxx::expected<TimerError> Timer::stop() noexcept
{
    if (!m_osTimer.has_value())
    {
        return cxx::error<TimerError>(TimerError::TIMER_NOT_INITIALIZED);
    }

    return m_osTimer->stop();
}

cxx::expected<TimerError>
Timer::restart(const units::Duration timeToWait, const RunMode runMode, const CatchUpPolicy catchUpPolicy) noexcept
{
    if (timeToWait.toNanoseconds() == 0U)
    {
        return cxx::error<TimerError>(TimerError::TIMEOUT_IS_ZERO);
    }

    if (!m_osTimer.has_value())
    {
        return cxx::error<TimerError>(TimerError::TIMER_NOT_INITIALIZED);
    }

    return m_osTimer->restart(timeToWait, runMode, catchUpPolicy);
}

cxx::expected<units::Duration, TimerError> Timer::timeUntilExpiration() noexcept
{
    if (!m_osTimer.has_value())
    {
        return cxx::error<TimerError>(TimerError::TIMER_NOT_INITIALIZED);
    }

    return m_osTimer->timeUntilExpiration();
}

cxx::expected<uint64_t, TimerError> Timer::getOverruns() noexcept
{
    if (!m_osTimer.has_value())
    {
        return cxx::error<TimerError>(TimerError::TIMER_NOT_INITIALIZED);
    }

    return m_osTimer->getOverruns();
}

bool Timer::hasError() const noexcept
{
    return m_errorValue != TimerError::NO_ERROR;
}

TimerError Timer::getError() const noexcept
{
    return m_errorValue;
}

cxx::error<TimerError> Timer::createErrorFromErrno(const int32_t errnum) noexcept
{
    TimerError timerError = TimerError::INTERNAL_LOGIC_ERROR;
    switch (errnum)
    {
    case EAGAIN:
    {
        std::cerr << "Kernel failed to allocate timer structures" << std::endl;
        timerError = TimerError::KERNEL_ALLOC_FAILED;
        break;
    }
    case EINVAL:
    {
        std::cerr << "Provided invalid arguments for posix::Timer" << std::endl;
        timerError = TimerError::INVALID_ARGUMENTS;
        break;
    }
    case ENOMEM:
    {
        std::cerr << "Could not allocate memory for posix::Timer" << std::endl;
        timerError = TimerError::ALLOC_MEM_FAILED;
        break;
    }
    case EPERM:
    {
        std::cerr << "No permissions to set the clock" << std::endl;
        timerError = TimerError::NO_PERMISSION;
        break;
    }
    case EFAULT:
    {
        std::cerr << "An invalid pointer was provided" << std::endl;
        timerError = TimerError::INVALID_POINTER;
        break;
    }
    default:
    {
        std::cerr << "Internal logic error in posix::Timer occurred" << std::endl;
        break;
    }
    }
    return cxx::error<TimerError>(timerError);
}

} // namespace posix
} // namespace iox
