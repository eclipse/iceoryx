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

#include "iceoryx_utils/cxx/deadline_timer.hpp"

namespace iox
{
namespace cxx
{
DeadlineTimer::DeadlineTimer(const iox::units::Duration timeToWait) noexcept
    : m_timeToWait(timeToWait)
    , m_endTime(getCurrentMonotonicTime() + timeToWait)
{
}

bool DeadlineTimer::hasExpired() const noexcept
{
    return getCurrentMonotonicTime() >= m_endTime;
}

void DeadlineTimer::reset() noexcept
{
    m_endTime = getCurrentMonotonicTime() + m_timeToWait;
}

void DeadlineTimer::reset(const iox::units::Duration timeToWait) noexcept
{
    m_timeToWait = timeToWait;
    reset();
}

const iox::units::Duration DeadlineTimer::remainingTime() const noexcept
{
    auto currentTime = getCurrentMonotonicTime();
    if (m_endTime > currentTime)
    {
        return m_endTime - currentTime;
    }
    return iox::units::Duration(std::chrono::milliseconds(0));
}

iox::units::Duration DeadlineTimer::getCurrentMonotonicTime() const noexcept
{
    iox::units::Duration currentTime(std::chrono::steady_clock::now().time_since_epoch());
    return currentTime;
}


} // namespace cxx
} // namespace iox
