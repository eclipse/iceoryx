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

#ifndef IOX_POSH_POPO_EVENT_CALLBACK_HPP
#define IOX_POSH_POPO_EVENT_CALLBACK_HPP

#include "iceoryx_utils/cxx/attributes.hpp"

namespace iox
{
namespace popo
{
template <typename OriginType, typename ContextDataType>
struct EventCallback;

namespace internal
{
struct NoType_t
{
};

using GenericCallbackPtr_t = void (*)();
using GenericCallbackRef_t = void (&)();

using TranslationCallbackRef_t = void (&)(void* const, void* const, GenericCallbackPtr_t const);
using TranslationCallbackPtr_t = void (*)(void* const, void* const, GenericCallbackPtr_t const);

template <typename T, typename ContextDataType>
struct TranslateAndCallTypelessCallback
{
    static void call(void* const origin, void* const userType, GenericCallbackPtr_t underlyingCallback) noexcept;
};

template <typename T>
struct TranslateAndCallTypelessCallback<T, NoType_t>
{
    static void call(void* const origin, void* const userType, GenericCallbackPtr_t underlyingCallback) noexcept;
};
} // namespace internal

///@brief the struct describes a callback with a user defined type which can
///         be attached to a WaitSet or a Listener
template <typename OriginType, typename ContextDataType>
struct NotificationCallback
{
    using Ref_t = void (&)(OriginType* const, ContextDataType* const);
    using Ptr_t = void (*)(OriginType* const, ContextDataType* const);

    Ptr_t m_callback = nullptr;
    ContextDataType* m_contextData = nullptr;
};

///@brief the struct describes a callback which can be attached to a WaitSet or a Listener
template <typename OriginType>
struct NotificationCallback<OriginType, internal::NoType_t>
{
    using Ref_t = void (&)(OriginType* const);
    using Ptr_t = void (*)(OriginType* const);

    Ptr_t m_callback = nullptr;
    internal::NoType_t* m_contextData = nullptr;
};

/// @brief creates an NotificationCallback
/// @param[in] callback reference to a callback with the signature void(OriginType*)
/// @return the callback stored inside of an NotificationCallback
template <typename OriginType, typename ContextDataType = internal::NoType_t>
NotificationCallback<OriginType, ContextDataType> createNotificationCallback(void (&callback)(OriginType* const));

/// @brief creates an NotificationCallback with a user defined value
/// @param[in] callback reference to a callback with the signature void(OriginType*, ContextDataType*)
/// @param[in] userValue reference to a user defined value
/// @return the callback and user value stored inside of an NotificationCallback
template <typename OriginType, typename ContextDataType>
NotificationCallback<OriginType, ContextDataType>
createNotificationCallback(void (&callback)(OriginType* const, ContextDataType* const), ContextDataType& userValue);

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/notification_callback.inl"

#endif
