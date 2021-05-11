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

#ifndef IOX_POSH_POPO_NOTIFICATION_ATTORNEY_HPP
#define IOX_POSH_POPO_NOTIFICATION_ATTORNEY_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"

#include <cstdint>

namespace iox
{
namespace popo
{
/// @brief Class which allows accessing private methods to
///         friends of NotificationAttorney. Used for example by the WaitSet.
///         Implements the Client-Attorney Pattern.
class NotificationAttorney
{
    template <uint64_t>
    friend class WaitSet;
    friend class Listener;

  private:
    template <typename T, typename... Targs>
    static void enableEvent(T& eventOrigin, Targs&&... args) noexcept;

    template <typename T, typename... Targs>
    static void disableEvent(T& eventOrigin, Targs&&... args) noexcept;

    template <typename T, typename... Targs>
    static void enableState(T& stateOrigin, Targs&&... args) noexcept;

    template <typename T, typename... Targs>
    static void disableState(T& stateOrigin, Targs&&... args) noexcept;

    template <typename T>
    static cxx::MethodCallback<void, uint64_t> getInvalidateTriggerMethod(T& eventOrigin) noexcept;

    template <typename T, typename... Targs>
    static WaitSetIsConditionSatisfiedCallback getCallbackForIsStateConditionSatisfied(T& eventOrigin,
                                                                                       Targs&&... args) noexcept;
};


} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/notification_attorney.inl"
#endif
