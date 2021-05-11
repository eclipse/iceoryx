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

#ifndef IOX_POSH_POPO_WAIT_SET_INL
#define IOX_POSH_POPO_WAIT_SET_INL

namespace iox
{
namespace popo
{
template <uint64_t Capacity>
inline WaitSet<Capacity>::WaitSet() noexcept
    : WaitSet(*runtime::PoshRuntime::getInstance().getMiddlewareConditionVariable())
{
}

template <uint64_t Capacity>
inline WaitSet<Capacity>::WaitSet(ConditionVariableData& condVarData) noexcept
    : m_conditionVariableDataPtr(&condVarData)
    , m_conditionListener(condVarData)
{
    for (uint64_t i = 0U; i < Capacity; ++i)
    {
        m_indexRepository.push(i);
    }
}

template <uint64_t Capacity>
inline WaitSet<Capacity>::~WaitSet() noexcept
{
    removeAllTriggers();
    m_conditionVariableDataPtr->m_toBeDestroyed.store(true, std::memory_order_relaxed);
}

template <uint64_t Capacity>
inline void WaitSet<Capacity>::markForDestruction() noexcept
{
    m_conditionListener.destroy();
}

template <uint64_t Capacity>
template <typename T, typename ContextDataType>
inline cxx::expected<uint64_t, WaitSetError>
WaitSet<Capacity>::attachImpl(T& eventOrigin,
                              const WaitSetIsConditionSatisfiedCallback& hasTriggeredCallback,
                              const uint64_t eventId,
                              const NotificationCallback<T, ContextDataType>& eventCallback,
                              const uint64_t originType,
                              const uint64_t originTypeHash) noexcept
{
    for (auto& currentTrigger : m_triggerArray)
    {
        if (currentTrigger && currentTrigger->isLogicalEqualTo(&eventOrigin, originType, originTypeHash))
        {
            return cxx::error<WaitSetError>(WaitSetError::ALREADY_ATTACHED);
        }
    }

    cxx::MethodCallback<void, uint64_t> invalidationCallback =
        NotificationAttorney::getInvalidateTriggerMethod(eventOrigin);
    auto index = m_indexRepository.pop();
    if (!index)
    {
        return cxx::error<WaitSetError>(WaitSetError::WAIT_SET_FULL);
    }


    if (hasTriggeredCallback)
    {
        m_triggerArray[*index].emplace(StateBasedTrigger,
                                       &eventOrigin,
                                       hasTriggeredCallback,
                                       invalidationCallback,
                                       eventId,
                                       eventCallback,
                                       *index,
                                       originType,
                                       originTypeHash);
    }
    else
    {
        m_triggerArray[*index].emplace(EventBasedTrigger,
                                       &eventOrigin,
                                       invalidationCallback,
                                       eventId,
                                       eventCallback,
                                       *index,
                                       originType,
                                       originTypeHash);
    }

    return cxx::success<uint64_t>(*index);
}

template <uint64_t Capacity>
template <typename T, typename EventType, typename ContextDataType, typename>
inline cxx::expected<WaitSetError>
WaitSet<Capacity>::attachEvent(T& eventOrigin,
                               const EventType eventType,
                               const uint64_t eventId,
                               const NotificationCallback<T, ContextDataType>& eventCallback) noexcept
{
    static_assert(IS_EVENT_ENUM<EventType>, "Only enums with an underlying EventEnumIdentifier are allowed.");

    return attachImpl(eventOrigin,
                      WaitSetIsConditionSatisfiedCallback(),
                      eventId,
                      eventCallback,
                      static_cast<uint64_t>(eventType),
                      typeid(EventType).hash_code())
        .and_then([&](auto& uniqueId) {
            NotificationAttorney::enableEvent(
                eventOrigin,
                TriggerHandle(*m_conditionVariableDataPtr, {*this, &WaitSet::removeTrigger}, uniqueId),
                eventType);
        });
}

template <uint64_t Capacity>
template <typename T, typename EventType, typename ContextDataType, typename>
inline cxx::expected<WaitSetError> WaitSet<Capacity>::attachEvent(
    T& eventOrigin, const EventType eventType, const NotificationCallback<T, ContextDataType>& eventCallback) noexcept
{
    return attachEvent(eventOrigin, eventType, NotificationInfo::INVALID_ID, eventCallback);
}

template <uint64_t Capacity>
template <typename T, typename ContextDataType>
inline cxx::expected<WaitSetError> WaitSet<Capacity>::attachEvent(
    T& eventOrigin, const uint64_t eventId, const NotificationCallback<T, ContextDataType>& eventCallback) noexcept
{
    return attachImpl(eventOrigin,
                      WaitSetIsConditionSatisfiedCallback(),
                      eventId,
                      eventCallback,
                      static_cast<uint64_t>(NoEventEnumUsed::PLACEHOLDER),
                      typeid(NoEventEnumUsed).hash_code())
        .and_then([&](auto& uniqueId) {
            NotificationAttorney::enableEvent(
                eventOrigin, TriggerHandle(*m_conditionVariableDataPtr, {*this, &WaitSet::removeTrigger}, uniqueId));
        });
}

template <uint64_t Capacity>
template <typename T, typename ContextDataType>
inline cxx::expected<WaitSetError>
WaitSet<Capacity>::attachEvent(T& eventOrigin, const NotificationCallback<T, ContextDataType>& eventCallback) noexcept
{
    return attachEvent(eventOrigin, NotificationInfo::INVALID_ID, eventCallback);
}

template <uint64_t Capacity>
template <typename T, typename StateType, typename ContextDataType, typename>
inline cxx::expected<WaitSetError>
WaitSet<Capacity>::attachState(T& stateOrigin,
                               const StateType stateType,
                               const uint64_t id,
                               const NotificationCallback<T, ContextDataType>& stateCallback) noexcept
{
    static_assert(IS_STATE_ENUM<StateType>, "Only enums with an underlying StateEnumIdentifier are allowed.");
    auto hasTriggeredCallback = NotificationAttorney::getCallbackForIsStateConditionSatisfied(stateOrigin, stateType);

    return attachImpl(stateOrigin,
                      hasTriggeredCallback,
                      id,
                      stateCallback,
                      static_cast<uint64_t>(stateType),
                      typeid(StateType).hash_code())
        .and_then([&](auto& uniqueId) {
            NotificationAttorney::enableState(
                stateOrigin,
                TriggerHandle(*m_conditionVariableDataPtr, {*this, &WaitSet::removeTrigger}, uniqueId),
                stateType);
        });
}

template <uint64_t Capacity>
template <typename T, typename StateType, typename ContextDataType, typename>
inline cxx::expected<WaitSetError> WaitSet<Capacity>::attachState(
    T& stateOrigin, const StateType stateType, const NotificationCallback<T, ContextDataType>& stateCallback) noexcept
{
    return attachState(stateOrigin, stateType, NotificationInfo::INVALID_ID, stateCallback);
}

template <uint64_t Capacity>
template <typename T, typename ContextDataType>
inline cxx::expected<WaitSetError> WaitSet<Capacity>::attachState(
    T& stateOrigin, const uint64_t id, const NotificationCallback<T, ContextDataType>& stateCallback) noexcept
{
    auto hasTriggeredCallback = NotificationAttorney::getCallbackForIsStateConditionSatisfied(stateOrigin);
    return attachImpl(stateOrigin,
                      hasTriggeredCallback,
                      id,
                      stateCallback,
                      static_cast<uint64_t>(NoStateEnumUsed::PLACEHOLDER),
                      typeid(NoStateEnumUsed).hash_code())
        .and_then([&](auto& uniqueId) {
            NotificationAttorney::enableState(
                stateOrigin, TriggerHandle(*m_conditionVariableDataPtr, {*this, &WaitSet::removeTrigger}, uniqueId));
        });
}

template <uint64_t Capacity>
template <typename T, typename ContextDataType>
inline cxx::expected<WaitSetError>
WaitSet<Capacity>::attachState(T& stateOrigin, const NotificationCallback<T, ContextDataType>& stateCallback) noexcept
{
    return attachState(stateOrigin, NotificationInfo::INVALID_ID, stateCallback);
}

template <uint64_t Capacity>
template <typename T, typename... Targs>
inline void WaitSet<Capacity>::detachEvent(T& eventOrigin, const Targs&... args) noexcept
{
    NotificationAttorney::disableEvent(eventOrigin, args...);
}

template <uint64_t Capacity>
template <typename T, typename... Targs>
inline void WaitSet<Capacity>::detachState(T& stateOrigin, const Targs&... args) noexcept
{
    NotificationAttorney::disableState(stateOrigin, args...);
}

template <uint64_t Capacity>
inline void WaitSet<Capacity>::removeTrigger(const uint64_t uniqueTriggerId) noexcept
{
    for (auto& trigger : m_triggerArray)
    {
        if (trigger.has_value() && trigger->getUniqueId() == uniqueTriggerId)
        {
            trigger->invalidate();
            trigger.reset();
            cxx::Ensures(m_indexRepository.push(uniqueTriggerId));
            return;
        }
    }
}

template <uint64_t Capacity>
inline void WaitSet<Capacity>::removeAllTriggers() noexcept
{
    for (auto& trigger : m_triggerArray)
    {
        trigger.reset();
    }
}

template <uint64_t Capacity>
inline typename WaitSet<Capacity>::NotificationInfoVector
WaitSet<Capacity>::timedWait(const units::Duration timeout) noexcept
{
    return waitAndReturnTriggeredTriggers([this, timeout] { return this->m_conditionListener.timedWait(timeout); });
}

template <uint64_t Capacity>
inline typename WaitSet<Capacity>::NotificationInfoVector WaitSet<Capacity>::wait() noexcept
{
    return waitAndReturnTriggeredTriggers([this] { return this->m_conditionListener.wait(); });
}

template <uint64_t Capacity>
inline typename WaitSet<Capacity>::NotificationInfoVector
WaitSet<Capacity>::createVectorWithTriggeredTriggers() noexcept
{
    NotificationInfoVector triggers;
    if (!m_activeNotifications.empty())
    {
        for (uint64_t i = m_activeNotifications.size() - 1U;; --i)
        {
            auto index = m_activeNotifications[i];
            auto& trigger = m_triggerArray[index];
            bool doRemoveNotificationId = !static_cast<bool>(trigger);

            if (!doRemoveNotificationId && trigger->isStateConditionSatisfied())
            {
                cxx::Expects(triggers.push_back(&m_triggerArray[index]->getNotificationInfo()));
                doRemoveNotificationId = (trigger->getTriggerType() == TriggerType::EVENT_BASED);
            }

            if (doRemoveNotificationId)
            {
                m_activeNotifications.erase(m_activeNotifications.begin() + i);
            }

            if (i == 0U)
            {
                break;
            }
        }
    }

    return triggers;
}

template <uint64_t Capacity>
inline void WaitSet<Capacity>::acquireNotifications(const WaitFunction& wait) noexcept
{
    auto notificationVector = wait();
    if (m_activeNotifications.empty())
    {
        m_activeNotifications = notificationVector;
    }
    else if (!notificationVector.empty())
    {
        m_activeNotifications = algorithm::uniqueMergeSortedContainers(notificationVector, m_activeNotifications);
    }
}

template <uint64_t Capacity>
inline typename WaitSet<Capacity>::NotificationInfoVector
WaitSet<Capacity>::waitAndReturnTriggeredTriggers(const WaitFunction& wait) noexcept
{
    if (m_conditionListener.wasNotified())
    {
        this->acquireNotifications(wait);
    }

    NotificationInfoVector triggers = createVectorWithTriggeredTriggers();

    if (!triggers.empty())
    {
        return triggers;
    }

    acquireNotifications(wait);
    return createVectorWithTriggeredTriggers();
}

template <uint64_t Capacity>
inline uint64_t WaitSet<Capacity>::size() const noexcept
{
    return Capacity - m_indexRepository.size();
}

template <uint64_t Capacity>
inline constexpr uint64_t WaitSet<Capacity>::capacity() noexcept
{
    return Capacity;
}


} // namespace popo
} // namespace iox

#endif
