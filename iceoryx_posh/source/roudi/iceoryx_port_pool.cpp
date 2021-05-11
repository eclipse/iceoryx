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

#include "iceoryx_posh/internal/roudi/iceoryx_port_pool.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/roudi/port_pool_data.hpp"

namespace iox
{
namespace roudi
{
IceOryxPortPool::IceOryxPortPool(PortPoolData& portPoolData) noexcept
    : PortPool(portPoolData)
    , m_portPoolData(&portPoolData)
{
}

cxx::vector<SubscriberPortType::MemberType_t*, MAX_SUBSCRIBERS>
IceOryxPortPool::getSubscriberPortDataList() noexcept
{
    return m_portPoolData->m_subscriberPortMembers.content();
}

cxx::expected<PublisherPortRouDiType::MemberType_t*, PortPoolError>
IceOryxPortPool::addPublisherPort(const capro::ServiceDescription& serviceDescription,
                                  const uint64_t& historyCapacity,
                                  mepoo::MemoryManager* const memoryManager,
                                  const RuntimeName_t& runtimeName,
                                  const mepoo::MemoryInfo& memoryInfo) noexcept
{
    if (m_portPoolData->m_publisherPortMembers.hasFreeSpace())
    {
        auto publisherPortData = m_portPoolData->m_publisherPortMembers.insert(
            serviceDescription, runtimeName, memoryManager, historyCapacity, memoryInfo);
        return cxx::success<PublisherPortRouDiType::MemberType_t*>(publisherPortData);
    }
    else
    {
        errorHandler(Error::kPORT_POOL__PUBLISHERLIST_OVERFLOW, nullptr, ErrorLevel::MODERATE);
        return cxx::error<PortPoolError>(PortPoolError::PUBLISHER_PORT_LIST_FULL);
    }
}

cxx::expected<SubscriberPortType::MemberType_t*, PortPoolError>
IceOryxPortPool::addSubscriberPort(const capro::ServiceDescription& serviceDescription,
                                   const uint64_t& historyRequest,
                                   const RuntimeName_t& runtimeName,
                                   const mepoo::MemoryInfo& memoryInfo) noexcept
{
    if (m_portPoolData->m_subscriberPortMembers.hasFreeSpace())
    {
        auto subscriberPortData = m_portPoolData->m_subscriberPortMembers.insert(
            serviceDescription, runtimeName, SUBSCRIBER_PORT_QUEUE_TYPE, historyRequest, memoryInfo);
        return cxx::success<SubscriberPortType::MemberType_t*>(subscriberPortData);
    }
    else
    {
        errorHandler(Error::kPORT_POOL__SUBSCRIBERLIST_OVERFLOW, nullptr, ErrorLevel::MODERATE);
        return cxx::error<PortPoolError>(PortPoolError::SUBSCRIBER_PORT_LIST_FULL);
    }
}

void IceOryxPortPool::removePublisherPort(PublisherPortRouDiType::MemberType_t* const portData) noexcept
{
    m_portPoolData->m_publisherPortMembers.erase(portData);
}

void IceOryxPortPool::removeSubscriberPort(SubscriberPortType::MemberType_t* const portData) noexcept
{
    m_portPoolData->m_subscriberPortMembers.erase(portData);
}

} // namespace roudi
} // namespace iox
