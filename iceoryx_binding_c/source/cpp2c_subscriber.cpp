// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 Apex.AI Inc. All rights reserved.
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

#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"

using namespace iox::popo;

cpp2c_Subscriber::~cpp2c_Subscriber()
{
    if (m_portData)
    {
        iox::popo::SubscriberPortUser(m_portData).destroy();
    }
}

void cpp2c_Subscriber::enableEvent(iox::popo::TriggerHandle&& triggerHandle,
                                   const iox::popo::SubscriberEvent subscriberEvent) noexcept
{
    switch (subscriberEvent)
    {
    case SubscriberEvent::DATA_RECEIVED:
        m_trigger = std::move(triggerHandle);
        iox::popo::SubscriberPortUser(m_portData)
            .setConditionVariable(*m_trigger.getConditionVariableData(), m_trigger.getUniqueId());
        break;
    }
}

void cpp2c_Subscriber::disableEvent(const SubscriberEvent subscriberEvent) noexcept
{
    switch (subscriberEvent)
    {
    case SubscriberEvent::DATA_RECEIVED:
        m_trigger.reset();
        break;
    }
}

void cpp2c_Subscriber::enableState(iox::popo::TriggerHandle&& triggerHandle,
                                   const iox::popo::SubscriberState subscriberState) noexcept
{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        m_trigger = std::move(triggerHandle);
        iox::popo::SubscriberPortUser(m_portData)
            .setConditionVariable(*m_trigger.getConditionVariableData(), m_trigger.getUniqueId());
        break;
    }
}

void cpp2c_Subscriber::disableState(const SubscriberState subscriberState) noexcept
{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        m_trigger.reset();
        break;
    }
}

iox::popo::WaitSetIsConditionSatisfiedCallback
cpp2c_Subscriber::getCallbackForIsStateConditionSatisfied(const SubscriberState subscriberState) const noexcept
{
    switch (subscriberState)
    {
    case SubscriberState::HAS_DATA:
        return {*this, &cpp2c_Subscriber::hasSamples};
    }

    return {};
}

void cpp2c_Subscriber::invalidateTrigger(const uint64_t uniqueTriggerId) noexcept
{
    if (m_trigger.getUniqueId() == uniqueTriggerId)
    {
        iox::popo::SubscriberPortUser(m_portData).unsetConditionVariable();
        m_trigger.invalidate();
    }
}

bool cpp2c_Subscriber::hasSamples() const noexcept
{
    return iox::popo::SubscriberPortUser(m_portData).hasNewChunks();
}

