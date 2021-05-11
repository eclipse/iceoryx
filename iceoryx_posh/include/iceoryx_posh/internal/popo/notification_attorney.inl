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

#ifndef IOX_POSH_POPO_NOTIFICATION_ATTORNEY_INL
#define IOX_POSH_POPO_NOTIFICATION_ATTORNEY_INL

namespace iox
{
namespace popo
{
template <typename T, typename... Targs>
inline void NotificationAttorney::enableEvent(T& eventOrigin, Targs&&... args) noexcept
{
    eventOrigin.enableEvent(std::forward<Targs>(args)...);
}

template <typename T, typename... Targs>
inline void NotificationAttorney::disableEvent(T& eventOrigin, Targs&&... args) noexcept
{
    eventOrigin.disableEvent(std::forward<Targs>(args)...);
}

template <typename T, typename... Targs>
inline void NotificationAttorney::enableState(T& eventOrigin, Targs&&... args) noexcept
{
    eventOrigin.enableState(std::forward<Targs>(args)...);
}

template <typename T, typename... Targs>
inline void NotificationAttorney::disableState(T& eventOrigin, Targs&&... args) noexcept
{
    eventOrigin.disableState(std::forward<Targs>(args)...);
}

template <typename T, typename... Targs>
inline WaitSetIsConditionSatisfiedCallback
NotificationAttorney::getCallbackForIsStateConditionSatisfied(T& eventOrigin, Targs&&... args) noexcept
{
    return eventOrigin.getCallbackForIsStateConditionSatisfied(std::forward<Targs>(args)...);
}

template <typename T>
inline cxx::MethodCallback<void, uint64_t> NotificationAttorney::getInvalidateTriggerMethod(T& eventOrigin) noexcept
{
    return cxx::MethodCallback<void, uint64_t>(
        eventOrigin, static_cast<cxx::MethodCallback<void, uint64_t>::MethodPointer<T>>(&T::invalidateTrigger));
}

} // namespace popo
} // namespace iox

#endif
