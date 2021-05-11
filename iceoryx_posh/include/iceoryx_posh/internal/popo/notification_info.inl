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

#ifndef IOX_POSH_POPO_NOTIFICATION_INFO_INL
#define IOX_POSH_POPO_NOTIFICATION_INFO_INL

namespace iox
{
namespace popo
{
template <typename T, typename ContextDataType>
inline NotificationInfo::NotificationInfo(T* const notificationOrigin,
                                          const uint64_t notificationId,
                                          const NotificationCallback<T, ContextDataType>& callback) noexcept
    : m_notificationOrigin(notificationOrigin)
    , m_userValue(callback.m_contextData)
    , m_notificationOriginTypeHash(typeid(T).hash_code())
    , m_notificationId(notificationId)
    , m_callbackPtr(reinterpret_cast<internal::GenericCallbackPtr_t>(callback.m_callback))
    , m_callback(internal::TranslateAndCallTypelessCallback<T, ContextDataType>::call)
{
}

template <typename T>
inline bool NotificationInfo::doesOriginateFrom(T* const notificationOrigin) const noexcept
{
    if (m_notificationOrigin == nullptr)
    {
        return false;
    }
    return m_notificationOrigin == notificationOrigin;
}

template <typename T>
inline T* NotificationInfo::getOrigin() const noexcept
{
    if (m_notificationOriginTypeHash != typeid(T).hash_code())
    {
        errorHandler(
            Error::kPOPO__NOTIFICATION_INFO_TYPE_INCONSISTENCY_IN_GET_ORIGIN, nullptr, iox::ErrorLevel::MODERATE);
        return nullptr;
    }

    return static_cast<T*>(m_notificationOrigin);
}

} // namespace popo
} // namespace iox

#endif
