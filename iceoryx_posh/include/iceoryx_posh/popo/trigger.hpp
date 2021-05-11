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

#ifndef IOX_POSH_POPO_TRIGGER_HPP
#define IOX_POSH_POPO_TRIGGER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/notification_callback.hpp"
#include "iceoryx_posh/popo/notification_info.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"

#include <type_traits>
#include <typeinfo>

namespace iox
{
namespace popo
{
struct StateBasedTrigger_t
{
};
constexpr StateBasedTrigger_t StateBasedTrigger{};

struct EventBasedTrigger_t
{
};
constexpr EventBasedTrigger_t EventBasedTrigger{};

enum class TriggerType
{
    STATE_BASED,
    EVENT_BASED,
    INVALID
};

/// @brief The Trigger class is usually managed by a factory class like a
///      WaitSet and acquired by classes which would like to signal a
///      notification. Multiple Trigger can share a common ConditionVariableData pointer
///      so that multiple Trigger can signal a single instance.
///
class Trigger
{
  public:
    static constexpr uint64_t INVALID_TRIGGER_ID = std::numeric_limits<uint64_t>::max();

    Trigger() noexcept = delete;
    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;

    /// @brief Creates a state based Trigger
    /// @param[in] StateBasedTrigger_t signals that we are creating a state based trigger
    /// @param[in] stateOrigin pointer to the class where the signal originates from, if it's set to nullptr the Trigger
    /// is in a defined but invalid state
    /// @param[in] hasTriggeredCallback callback to a method which informs the trigger if it was triggered or not. If an
    /// empty callback is set the trigger is in a defined but invalid state.
    /// @param[in] resetCallback callback which is called when the trigger goes out of scope.
    /// @param[in] notificationId id of the corresponding event/state
    /// @param[in] callback function pointer of type void(*)(T * const) to a callback which can be called by the
    /// trigger.
    /// @param[in] uniqueId a context wide unique id to identify the trigger
    /// @param[in] stateType the uint64_t value of the  state origins state enum
    /// @param[in] stateTypeHash the uint64_t type hash of the state enum
    template <typename T, typename UserType>
    Trigger(StateBasedTrigger_t,
            T* const stateOrigin,
            const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
            const cxx::MethodCallback<void, uint64_t>& resetCallback,
            const uint64_t notificationId,
            const NotificationCallback<T, UserType>& callback,
            const uint64_t uniqueId,
            const uint64_t stateType,
            const uint64_t stateTypeHash) noexcept;

    /// @brief Creates an event based Trigger
    /// @param[in] EventBasedTrigger_t signals that we are creating an event based trigger
    /// @param[in] notificationOrigin pointer to the class where the signal originates from, if it's set to nullptr the
    /// Trigger is in a defined but invalid state
    /// @param[in] resetCallback callback which is called when the trigger goes out of scope.
    /// @param[in] notificationId id of the corresponding event
    /// @param[in] callback function pointer of type void(*)(T * const) to a callback which can be called by the
    /// trigger.
    /// @param[in] uniqueId a context wide unique id to identify the trigger
    /// @param[in] notificationType the uint64_t value of the events origins event enum
    /// @param[in] notificationTypeHash the uint64_t type hash of the event enum
    template <typename T, typename UserType>
    Trigger(EventBasedTrigger_t,
            T* const notificationOrigin,
            const cxx::MethodCallback<void, uint64_t>& resetCallback,
            const uint64_t notificationId,
            const NotificationCallback<T, UserType>& callback,
            const uint64_t uniqueId,
            const uint64_t notificationType,
            const uint64_t notificationTypeHash) noexcept;

    Trigger(Trigger&& rhs) noexcept;
    Trigger& operator=(Trigger&& rhs) noexcept;

    /// @brief calls reset on destruction
    ~Trigger();

    /// @brief returns true if the Trigger is valid otherwise false
    ///        A trigger is valid when:
    ///         - origin != nullptr
    ///         - hasTriggeredCallback is set
    explicit operator bool() const noexcept;

    /// @brief returns true if the trigger is valid otherwise false
    bool isValid() const noexcept;

    /// @brief returns the result of the provided hasTriggeredCallback
    /// @note  an event based trigger returns always true when it's valid
    bool isStateConditionSatisfied() const noexcept;

    /// @brief resets and invalidates the Trigger
    void reset() noexcept;

    /// @brief invalidates the Trigger without calling the reset callback
    void invalidate() noexcept;

    /// @brief returns the internal unique id of the trigger
    uint64_t getUniqueId() const noexcept;

    /// @brief returns true if the Triggers are logical equal otherwise false. Two Triggers are logical equal when
    ///       - both Trigger are valid
    ///       - origin == rhs.origin
    ///       - originTriggerType == rhs.originTriggerType
    ///       - originTriggerTypeHash == rhs.originTriggerTypeHash
    bool isLogicalEqualTo(const void* const notificationOrigin,
                          const uint64_t originTriggerType,
                          const uint64_t originTriggerTypeHash) const noexcept;

    /// @brief sets a new origin of the trigger
    /// @param[in] newOrigin reference to the new origin
    template <typename T>
    void updateOrigin(T& newOrigin) noexcept;

    /// @brief returns the NotificationInfo
    const NotificationInfo& getNotificationInfo() const noexcept;

    /// @brief returns the type of trigger
    TriggerType getTriggerType() const noexcept;

  private:
    template <typename T, typename ContextDataType>
    Trigger(T* const notificationOrigin,
            const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
            const cxx::MethodCallback<void, uint64_t>& resetCallback,
            const uint64_t notificationId,
            const NotificationCallback<T, ContextDataType>& callback,
            const uint64_t uniqueId,
            const TriggerType triggerType,
            const uint64_t originTriggerType,
            const uint64_t originTriggerTypeHash) noexcept;

  private:
    NotificationInfo m_notificationInfo;

    cxx::ConstMethodCallback<bool> m_hasTriggeredCallback;
    cxx::MethodCallback<void, uint64_t> m_resetCallback;
    uint64_t m_uniqueId = INVALID_TRIGGER_ID;

    TriggerType m_triggerType = TriggerType::STATE_BASED;
    uint64_t m_originTriggerType = INVALID_TRIGGER_ID;
    uint64_t m_originTriggerTypeHash = INVALID_TRIGGER_ID;
};


} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/trigger.inl"

#endif
