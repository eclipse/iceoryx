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

#ifndef IOX_POSH_POPO_LISTENER_INL
#define IOX_POSH_POPO_LISTENER_INL
namespace iox
{
namespace popo
{
template <typename T, typename UserType>
inline cxx::expected<ListenerError>
Listener::attachEvent(T& eventOrigin, const NotificationCallback<T, UserType>& eventCallback) noexcept
{
    return addEvent(&eventOrigin,
                    eventCallback.m_contextData,
                    static_cast<uint64_t>(NoEnumUsed::PLACEHOLDER),
                    typeid(NoEnumUsed).hash_code(),
                    reinterpret_cast<internal::GenericCallbackRef_t>(*eventCallback.m_callback),
                    internal::TranslateAndCallTypelessCallback<T, UserType>::call,
                    NotificationAttorney::getInvalidateTriggerMethod(eventOrigin))
        .and_then([&](auto& eventId) {
            NotificationAttorney::enableEvent(
                eventOrigin, TriggerHandle(*m_conditionVariableData, {*this, &Listener::removeTrigger}, eventId));
        });
}

template <typename T, typename EventType, typename UserType, typename>
inline cxx::expected<ListenerError> Listener::attachEvent(
    T& eventOrigin, const EventType eventType, const NotificationCallback<T, UserType>& eventCallback) noexcept
{
    return addEvent(&eventOrigin,
                    eventCallback.m_contextData,
                    static_cast<uint64_t>(eventType),
                    typeid(EventType).hash_code(),
                    reinterpret_cast<internal::GenericCallbackRef_t>(*eventCallback.m_callback),
                    internal::TranslateAndCallTypelessCallback<T, UserType>::call,
                    NotificationAttorney::getInvalidateTriggerMethod(eventOrigin))
        .and_then([&](auto& eventId) {
            NotificationAttorney::enableEvent(
                eventOrigin,
                TriggerHandle(*m_conditionVariableData, {*this, &Listener::removeTrigger}, eventId),
                eventType);
        });
}

template <typename T, typename EventType, typename>
inline void Listener::detachEvent(T& eventOrigin, const EventType eventType) noexcept
{
    static_assert(IS_EVENT_ENUM<EventType>,
                  "Only enums with an underlying EventEnumIdentifier can be attached/detached to the Listener");
    NotificationAttorney::disableEvent(eventOrigin, eventType);
}

template <typename T>
inline void Listener::detachEvent(T& eventOrigin) noexcept
{
    NotificationAttorney::disableEvent(eventOrigin);
}

inline constexpr uint64_t Listener::capacity() noexcept
{
    return MAX_NUMBER_OF_EVENTS_PER_LISTENER;
}


} // namespace popo
} // namespace iox
#endif
