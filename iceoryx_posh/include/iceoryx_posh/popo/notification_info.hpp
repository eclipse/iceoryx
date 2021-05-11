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

#ifndef IOX_POSH_POPO_NOTIFICATION_INFO_HPP
#define IOX_POSH_POPO_NOTIFICATION_INFO_HPP

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/popo/notification_callback.hpp"
#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

#include <cstdint>
#include <limits>

namespace iox
{
namespace popo
{
/// @brief NotificationInfo holds the state of a trigger like the pointer to the triggerOrigin,
///        the notification id and the callback.
class NotificationInfo
{
  public:
    static constexpr uint64_t INVALID_ID = std::numeric_limits<uint64_t>::max();

    /// @brief constructs an empty NotificationInfo
    NotificationInfo() = default;
    virtual ~NotificationInfo() = default;

    /// @brief constructs a NotificationInfo object
    /// @param[in] notificationOrigin the origin of the event
    /// @param[in] notificationId id of the event
    /// @param[in] callback the callback of the event
    template <typename T, typename ContextDataType>
    NotificationInfo(T* const notificationOrigin,
                     const uint64_t notificationId,
                     const NotificationCallback<T, ContextDataType>& callback) noexcept;

    /// @brief returns the notification id
    /// @return the empty NotificationInfo always returns INVALID_ID, otherwise the actual notificationId is returned
    /// which can also be INVALID_ID
    uint64_t getNotificationId() const noexcept;

    /// @brief confirms the notificationOrigin
    /// @param[in] notificationOrigin the possible notificationOrigin
    /// @return true if the address is equal to the notificationOrigin, otherwise false. The empty NotificationInfo
    /// returns always false.
    template <typename T>
    bool doesOriginateFrom(T* const notificationOrigin) const noexcept;

    /// @brief returns the pointer to the notificationOrigin.
    /// @return If T equals the Triggerable type it returns the notificationOrigin.
    /// Otherwise it calls the errorHandler with a moderate error of
    /// kPOPO__EVENT_INFO_TYPE_INCONSISTENCY_IN_GET_ORIGIN and returns nullptr.
    template <typename T>
    T* getOrigin() const noexcept;

    /// @brief If a callback is set it executes the callback.
    /// @return true if the callback was called, otherwise false
    bool operator()() const noexcept;

    friend class Trigger;

  protected:
    void* m_notificationOrigin = nullptr;
    void* m_userValue = nullptr;
    uint64_t m_notificationOriginTypeHash = 0U;
    uint64_t m_notificationId = INVALID_ID;

    internal::GenericCallbackPtr_t m_callbackPtr = nullptr;
    internal::TranslationCallbackPtr_t m_callback = nullptr;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/notification_info.inl"

#endif
