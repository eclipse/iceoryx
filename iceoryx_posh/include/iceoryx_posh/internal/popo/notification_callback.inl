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

#ifndef IOX_POSH_POPO_NOTIFICATION_CALLBACK_INL
#define IOX_POSH_POPO_NOTIFICATION_CALLBACK_INL

namespace iox
{
namespace popo
{
namespace internal
{
template <typename T, typename ContextDataType>
inline void TranslateAndCallTypelessCallback<T, ContextDataType>::call(void* const origin,
                                                                       void* const userType,
                                                                       GenericCallbackPtr_t underlyingCallback) noexcept
{
    reinterpret_cast<typename NotificationCallback<T, ContextDataType>::Ptr_t>(underlyingCallback)(
        static_cast<T*>(origin), static_cast<ContextDataType*>(userType));
}

template <typename T>
inline void TranslateAndCallTypelessCallback<T, NoType_t>::call(void* const origin,
                                                                void* const userType IOX_MAYBE_UNUSED,
                                                                GenericCallbackPtr_t underlyingCallback) noexcept
{
    reinterpret_cast<typename NotificationCallback<T, NoType_t>::Ptr_t>(underlyingCallback)(static_cast<T*>(origin));
}
} // namespace internal

template <typename OriginType, typename ContextDataType>
inline NotificationCallback<OriginType, ContextDataType> createNotificationCallback(void (&callback)(OriginType* const))
{
    return NotificationCallback<OriginType, internal::NoType_t>{&callback};
}

template <typename OriginType, typename ContextDataType>
inline NotificationCallback<OriginType, ContextDataType>
createNotificationCallback(void (&callback)(OriginType* const, ContextDataType* const), ContextDataType& userValue)
{
    return NotificationCallback<OriginType, ContextDataType>{&callback, &userValue};
}
} // namespace popo
} // namespace iox

#endif
