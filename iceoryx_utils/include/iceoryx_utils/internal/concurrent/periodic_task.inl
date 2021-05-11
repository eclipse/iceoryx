// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_UTILS_CONCURRENT_PERIODIC_TASK_INL
#define IOX_UTILS_CONCURRENT_PERIODIC_TASK_INL

namespace iox
{
namespace concurrent
{
template <typename T>
template <typename... Args>
inline PeriodicTask<T>::PeriodicTask(const PeriodicTaskManualStart_t,
                                     const posix::ThreadName_t taskName,
                                     Args&&... args) noexcept
    : m_callable(std::forward<Args>(args)...)
    , m_taskName(taskName)
{
}

template <typename T>
template <typename... Args>
inline PeriodicTask<T>::PeriodicTask(const PeriodicTaskAutoStart_t,
                                     const units::Duration interval,
                                     const posix::ThreadName_t taskName,
                                     Args&&... args) noexcept
    : PeriodicTask(PeriodicTaskManualStart, taskName, std::forward<Args>(args)...)
{
    start(interval);
}

template <typename T>
inline PeriodicTask<T>::~PeriodicTask() noexcept
{
    stop();
}

template <typename T>
inline void PeriodicTask<T>::start(const units::Duration interval) noexcept
{
    stop();
    m_interval = interval;
    m_taskExecutor = std::thread(&PeriodicTask::run, this);
    posix::setThreadName(m_taskExecutor.native_handle(), m_taskName);
}

template <typename T>
inline void PeriodicTask<T>::stop() noexcept
{
    if (m_taskExecutor.joinable())
    {
        cxx::Expects(!m_stop.post().has_error());
        m_taskExecutor.join();
    }
}

template <typename T>
inline bool PeriodicTask<T>::isActive() const noexcept
{
    return m_taskExecutor.joinable();
}

template <typename T>
inline void PeriodicTask<T>::run() noexcept
{
    posix::SemaphoreWaitState waitState = posix::SemaphoreWaitState::NO_TIMEOUT;
    do
    {
        IOX_DISCARD_RESULT(m_callable());

        /// @todo use a refactored posix::Timer::wait method returning TIMER_TICK and TIMER_STOPPED once available
        auto waitResult = m_stop.timedWait(m_interval, true);
        cxx::Expects(!waitResult.has_error());

        waitState = waitResult.value();
    } while (waitState == posix::SemaphoreWaitState::TIMEOUT);
}

} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_PERIODIC_TASK_INL
