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

#include "iceoryx_posh/popo/notification_info.hpp"

namespace iox
{
namespace popo
{
constexpr uint64_t NotificationInfo::INVALID_ID;

uint64_t NotificationInfo::getNotificationId() const noexcept
{
    return m_notificationId;
}

bool NotificationInfo::operator()() const noexcept
{
    if (m_notificationOrigin != nullptr && m_callbackPtr != nullptr)
    {
        m_callback(m_notificationOrigin, m_userValue, m_callbackPtr);
        return true;
    }
    return false;
}

} // namespace popo
} // namespace iox
