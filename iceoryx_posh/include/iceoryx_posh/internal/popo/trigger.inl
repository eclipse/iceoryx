// Copyright (c) 2020, 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_POPO_TRIGGER_INL
#define IOX_POSH_POPO_TRIGGER_INL

#include <type_traits>
namespace iox
{
namespace popo
{
template <typename T, typename ContextDataType>
inline Trigger::Trigger(T* const notificationOrigin,
                        const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
                        const cxx::MethodCallback<void, uint64_t>& resetCallback,
                        const uint64_t notificationId,
                        const NotificationCallback<T, ContextDataType>& callback,
                        const uint64_t uniqueId,
                        const TriggerType triggerType,
                        const uint64_t originTriggerType,
                        const uint64_t originTriggerTypeHash) noexcept
    : m_notificationInfo(notificationOrigin, notificationId, callback)
    , m_hasTriggeredCallback(hasTriggeredCallback)
    , m_resetCallback(resetCallback)
    , m_uniqueId(uniqueId)
    , m_triggerType(triggerType)
    , m_originTriggerType(originTriggerType)
    , m_originTriggerTypeHash(originTriggerTypeHash)
{
    if (!resetCallback)
    {
        errorHandler(Error::kPOPO__TRIGGER_INVALID_RESET_CALLBACK, nullptr, ErrorLevel::FATAL);
        invalidate();
    }
}

template <typename T, typename ContextDataType>
inline Trigger::Trigger(StateBasedTrigger_t,
                        T* const stateOrigin,
                        const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
                        const cxx::MethodCallback<void, uint64_t>& resetCallback,
                        const uint64_t notificationId,
                        const NotificationCallback<T, ContextDataType>& callback,
                        const uint64_t uniqueId,
                        const uint64_t stateType,
                        const uint64_t stateTypeHash) noexcept
    : Trigger(stateOrigin,
              hasTriggeredCallback,
              resetCallback,
              notificationId,
              callback,
              uniqueId,
              TriggerType::STATE_BASED,
              stateType,
              stateTypeHash)
{
    if (!hasTriggeredCallback)
    {
        errorHandler(Error::kPOPO__TRIGGER_INVALID_HAS_TRIGGERED_CALLBACK, nullptr, ErrorLevel::FATAL);
        invalidate();
    }
}

template <typename T, typename ContextDataType>
inline Trigger::Trigger(EventBasedTrigger_t,
                        T* const notificationOrigin,
                        const cxx::MethodCallback<void, uint64_t>& resetCallback,
                        const uint64_t notificationId,
                        const NotificationCallback<T, ContextDataType>& callback,
                        const uint64_t uniqueId,
                        const uint64_t notificationType,
                        const uint64_t notificationTypeHash) noexcept
    : Trigger(notificationOrigin,
              cxx::ConstMethodCallback<bool>(),
              resetCallback,
              notificationId,
              callback,
              uniqueId,
              TriggerType::EVENT_BASED,
              notificationType,
              notificationTypeHash)
{
}

template <typename T>
inline void Trigger::updateOrigin(T& newOrigin) noexcept
{
    if (isValid() && &newOrigin != m_notificationInfo.m_notificationOrigin)
    {
        if (m_hasTriggeredCallback
            && m_hasTriggeredCallback.getObjectPointer<T>() == m_notificationInfo.m_notificationOrigin)
        {
            m_hasTriggeredCallback.setCallback(newOrigin, m_hasTriggeredCallback.getMethodPointer<T>());
        }

        if (m_resetCallback && m_resetCallback.getObjectPointer<T>() == m_notificationInfo.m_notificationOrigin)
        {
            m_resetCallback.setCallback(newOrigin, m_resetCallback.getMethodPointer<T>());
        }

        m_notificationInfo.m_notificationOrigin = &newOrigin;
    }
}


} // namespace popo
} // namespace iox

#endif
