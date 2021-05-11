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

#ifndef IOX_BINDING_C_CPP2C_SUBSCRIBER_HPP
#define IOX_BINDING_C_CPP2C_SUBSCRIBER_HPP

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/popo/base_subscriber.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"

struct cpp2c_Subscriber
{
    cpp2c_Subscriber() = default;
    cpp2c_Subscriber(const cpp2c_Subscriber&) = delete;
    cpp2c_Subscriber(cpp2c_Subscriber&& rhs) = delete;
    ~cpp2c_Subscriber();

    cpp2c_Subscriber& operator=(const cpp2c_Subscriber&) = delete;
    cpp2c_Subscriber& operator=(cpp2c_Subscriber&& rhs) = delete;

    void enableEvent(iox::popo::TriggerHandle&& triggerHandle,
                     const iox::popo::SubscriberEvent subscriberEvent) noexcept;

    void disableEvent(const iox::popo::SubscriberEvent subscriberEvent) noexcept;

    void enableState(iox::popo::TriggerHandle&& triggerHandle,
                     const iox::popo::SubscriberState subscriberState) noexcept;

    void disableState(const iox::popo::SubscriberState subscriberState) noexcept;

    void invalidateTrigger(const uint64_t uniqueTriggerId) noexcept;

    bool hasSamples() const noexcept;

    iox::popo::WaitSetIsConditionSatisfiedCallback
    getCallbackForIsStateConditionSatisfied(const iox::popo::SubscriberState subscriberState) const noexcept;


    iox::popo::SubscriberPortData* m_portData{nullptr};
    iox::popo::TriggerHandle m_trigger;
};
#endif
