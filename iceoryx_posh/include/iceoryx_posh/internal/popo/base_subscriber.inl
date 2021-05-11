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

#ifndef IOX_POSH_POPO_BASE_SUBSCRIBER_INL
#define IOX_POSH_POPO_BASE_SUBSCRIBER_INL

namespace iox
{
namespace popo
{
// ============================== BaseSubscriber ============================== //

template <typename port_t>
inline BaseSubscriber<port_t>::BaseSubscriber() noexcept
{
}

template <typename port_t>
inline BaseSubscriber<port_t>::BaseSubscriber(const capro::ServiceDescription& service,
                                              const SubscriberOptions& subscriberOptions) noexcept
    : m_port(iox::runtime::PoshRuntime::getInstance().getMiddlewareSubscriber(service, subscriberOptions))
{
}

template <typename port_t>
inline BaseSubscriber<port_t>::~BaseSubscriber()
{
    m_port.destroy();
}

template <typename port_t>
inline uid_t BaseSubscriber<port_t>::getUid() const noexcept
{
    return m_port.getUniqueID();
}

template <typename port_t>
inline capro::ServiceDescription /// todo #25 make this a reference.
BaseSubscriber<port_t>::getServiceDescription() const noexcept
{
    return m_port.getCaProServiceDescription();
}

template <typename port_t>
inline void BaseSubscriber<port_t>::subscribe() noexcept
{
    m_port.subscribe();
}

template <typename port_t>
inline SubscribeState BaseSubscriber<port_t>::getSubscriptionState() const noexcept
{
    return m_port.getSubscriptionState();
}

template <typename port_t>
inline void BaseSubscriber<port_t>::unsubscribe() noexcept
{
    m_port.unsubscribe();
}

template <typename port_t>
inline bool BaseSubscriber<port_t>::hasData() const noexcept
{
    return m_port.hasNewChunks();
}

template <typename port_t>
inline bool BaseSubscriber<port_t>::hasMissedData() noexcept
{
    return m_port.hasLostChunksSinceLastCall();
}

template <typename port_t>
inline cxx::expected<const mepoo::ChunkHeader*, ChunkReceiveResult> BaseSubscriber<port_t>::takeChunk() noexcept
{
    return m_port.tryGetChunk();
}

template <typename port_t>
inline void BaseSubscriber<port_t>::releaseQueuedData() noexcept
{
    m_port.releaseQueuedChunks();
}

template <typename port_t>
inline void BaseSubscriber<port_t>::invalidateTrigger(const uint64_t uniqueTriggerId) noexcept
{
    if (m_trigger.getUniqueId() == uniqueTriggerId)
    {
        m_port.unsetConditionVariable();
        m_trigger.invalidate();
    }
}

template <typename port_t>
inline void BaseSubscriber<port_t>::enableState(iox::popo::TriggerHandle&& triggerHandle,
                                                IOX_MAYBE_UNUSED const SubscriberState subscriberState) noexcept

{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        if (m_trigger)
        {
            LogWarn()
                << "The subscriber is already attached with either the SubscriberState::HAS_DATA or "
                   "SubscriberEvent::DATA_RECEIVED to a WaitSet/Listener. Detaching it from previous one and "
                   "attaching it to the new one with SubscriberState::HAS_DATA. Best practice is to call detach first.";

            errorHandler(
                Error::kPOPO__BASE_SUBSCRIBER_OVERRIDING_WITH_STATE_SINCE_HAS_DATA_OR_DATA_RECEIVED_ALREADY_ATTACHED,
                nullptr,
                ErrorLevel::MODERATE);
        }
        m_trigger = std::move(triggerHandle);
        m_port.setConditionVariable(*m_trigger.getConditionVariableData(), m_trigger.getUniqueId());
        break;
    }
}


template <typename port_t>
inline WaitSetIsConditionSatisfiedCallback
BaseSubscriber<port_t>::getCallbackForIsStateConditionSatisfied(const SubscriberState subscriberState) const noexcept
{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        return {*this, &SelfType::hasData};
    }
    return {};
}

template <typename port_t>
inline void BaseSubscriber<port_t>::disableState(const SubscriberState subscriberState) noexcept
{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        m_trigger.reset();
        m_port.unsetConditionVariable();
        break;
    }
}

template <typename port_t>
inline void BaseSubscriber<port_t>::enableEvent(iox::popo::TriggerHandle&& triggerHandle,
                                                const SubscriberEvent subscriberEvent) noexcept
{
    switch (subscriberEvent)
    {
    case SubscriberEvent::DATA_RECEIVED:
        if (m_trigger)
        {
            LogWarn()
                << "The subscriber is already attached with either the SubscriberState::HAS_DATA or "
                   "SubscriberEvent::DATA_RECEIVED to a WaitSet/Listener. Detaching it from previous one and "
                   "attaching it to the new one with SubscriberEvent::DATA_RECEIVED. Best practice is to call detach "
                   "first.";
            errorHandler(
                Error::kPOPO__BASE_SUBSCRIBER_OVERRIDING_WITH_EVENT_SINCE_HAS_DATA_OR_DATA_RECEIVED_ALREADY_ATTACHED,
                nullptr,
                ErrorLevel::MODERATE);
        }
        m_trigger = std::move(triggerHandle);
        m_port.setConditionVariable(*m_trigger.getConditionVariableData(), m_trigger.getUniqueId());
        break;
    }
}

template <typename port_t>
inline void BaseSubscriber<port_t>::disableEvent(const SubscriberEvent subscriberEvent) noexcept
{
    switch (subscriberEvent)
    {
    case SubscriberEvent::DATA_RECEIVED:
        m_trigger.reset();
        m_port.unsetConditionVariable();
        break;
    }
}

template <typename port_t>
inline const port_t& BaseSubscriber<port_t>::port() const noexcept
{
    return m_port;
}

template <typename port_t>
inline port_t& BaseSubscriber<port_t>::port() noexcept
{
    return m_port;
}


} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BASE_SUBSCRIBER_INL
