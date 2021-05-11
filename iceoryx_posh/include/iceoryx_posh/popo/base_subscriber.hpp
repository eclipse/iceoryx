// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_POSH_POPO_BASE_SUBSCRIBER_HPP
#define IOX_POSH_POPO_BASE_SUBSCRIBER_HPP

#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/popo/enum_trigger_type.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/unique_ptr.hpp"

namespace iox
{
namespace popo
{
using uid_t = UniquePortId;

enum class SubscriberEvent : EventEnumIdentifier
{
    DATA_RECEIVED
};

enum class SubscriberState : StateEnumIdentifier
{
    HAS_DATA
};

/// @brief base class for all types of subscriber
/// @param[in] port_t type of the underlying port, required for testing
template <typename port_t = iox::SubscriberPortUserType>
class BaseSubscriber
{
  public:
    virtual ~BaseSubscriber();

    ///
    /// @brief uid Get the unique ID of the subscriber.
    /// @return The subscriber's unique ID.
    ///
    uid_t getUid() const noexcept;

    ///
    /// @brief getServiceDescription Get the service description of the subscriber.
    /// @return The service description.
    ///
    capro::ServiceDescription getServiceDescription() const noexcept;

    ///
    /// @brief subscribe Initiate subscription.
    /// @return
    ///
    void subscribe() noexcept;

    ///
    /// @brief getSubscriptionState Get current subscription state.
    /// @return The current subscription state.
    ///
    SubscribeState getSubscriptionState() const noexcept;

    ///
    /// @brief unsubscribe Unsubscribes if currently subscribed, otherwise do nothing.
    ///
    void unsubscribe() noexcept;

    ///
    /// @brief Check if data is available.
    /// @return True if data is available.
    ///
    bool hasData() const noexcept;

    ///
    /// @brief Check if data has been missed since the last call of this method.
    /// @return True if data has been missed.
    /// @details Data may be missed due to overflowing receive queue.
    ///
    bool hasMissedData() noexcept;

    /// @brief Releases any unread queued data.
    void releaseQueuedData() noexcept;

    friend class NotificationAttorney;

  protected:
    /// @brief Only usable by the WaitSet, not for public use. Invalidates the internal triggerHandle.
    /// @param[in] uniqueTriggerId the id of the corresponding trigger
    /// @brief Only usable by the WaitSet, not for public use
    using SelfType = BaseSubscriber<port_t>;
    using PortType = port_t;

    BaseSubscriber() noexcept; // Required for testing.
    BaseSubscriber(const capro::ServiceDescription& service, const SubscriberOptions& subscriberOptions) noexcept;

    BaseSubscriber(const BaseSubscriber& other) = delete;
    BaseSubscriber& operator=(const BaseSubscriber&) = delete;
    BaseSubscriber(BaseSubscriber&& rhs) = delete;
    BaseSubscriber& operator=(BaseSubscriber&& rhs) = delete;

    /// @brief small helper method to unwrap the `expected<optional<ChunkHeader*>>` from the `tryGetChunk` method of the
    /// port
    cxx::expected<const mepoo::ChunkHeader*, ChunkReceiveResult> takeChunk() noexcept;

    void invalidateTrigger(const uint64_t trigger) noexcept;

    /// @brief Only usable by the WaitSet, not for public use. Attaches the triggerHandle to the internal trigger.
    /// @param[in] triggerHandle rvalue reference to the triggerHandle. This class takes the ownership of that handle.
    /// @param[in] subscriberState the state which should be attached
    void enableState(iox::popo::TriggerHandle&& triggerHandle, const SubscriberState subscriberState) noexcept;

    /// @brief Only usable by the WaitSet, not for public use. Returns method pointer to the event corresponding
    /// hasTriggered method callback
    /// @param[in] subscriberState the state to which the hasTriggeredCallback is required
    WaitSetIsConditionSatisfiedCallback
    getCallbackForIsStateConditionSatisfied(const SubscriberState subscriberState) const noexcept;

    /// @brief Only usable by the WaitSet, not for public use. Resets the internal triggerHandle
    /// @param[in] subscriberState the state which should be detached
    void disableState(const SubscriberState subscriberState) noexcept;

    /// @brief Only usable by the WaitSet, not for public use. Attaches the triggerHandle to the internal trigger.
    /// @param[in] triggerHandle rvalue reference to the triggerHandle. This class takes the ownership of that handle.
    /// @param[in] subscriberEvent the event which should be attached
    void enableEvent(iox::popo::TriggerHandle&& triggerHandle, const SubscriberEvent subscriberState) noexcept;

    /// @brief Only usable by the WaitSet, not for public use. Resets the internal triggerHandle
    /// @param[in] subscriberEvent the event which should be detached
    void disableEvent(const SubscriberEvent subscriberEvent) noexcept;

    ///
    /// @brief const accessor of the underlying port
    ///
    const port_t& port() const noexcept;

    ///
    /// @brief accessor of the underlying port
    ///
    port_t& port() noexcept;

  protected:
    port_t m_port{nullptr};
    TriggerHandle m_trigger;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/base_subscriber.inl"

#endif // IOX_POSH_POPO_BASE_SUBSCRIBER_HPP
