// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/popo/ports/subscriber_port_roudi.hpp"

namespace iox
{
namespace popo
{
SubscriberPortRouDi::SubscriberPortRouDi(cxx::not_null<MemberType_t* const> subscriberPortDataPtr) noexcept
    : BasePort(subscriberPortDataPtr)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)
{
}

QueueFullPolicy SubscriberPortRouDi::getQueueFullPolicy() const noexcept
{
    return getMembers()->m_chunkReceiverData.m_queueFullPolicy;
}

const SubscriberPortRouDi::MemberType_t* SubscriberPortRouDi::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

SubscriberPortRouDi::MemberType_t* SubscriberPortRouDi::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

void SubscriberPortRouDi::releaseAllChunks() noexcept
{
    m_chunkReceiver.releaseAll();
}

} // namespace popo
} // namespace iox
