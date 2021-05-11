// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_INTERNAL_ROUDI_PORT_MANAGER_INL
#define IOX_POSH_INTERNAL_ROUDI_PORT_MANAGER_INL

namespace iox
{
namespace roudi
{
template <typename T, std::enable_if_t<std::is_same<T, iox::build::OneToManyPolicy>::value>*>
inline cxx::optional<RuntimeName_t>
PortManager::doesViolateCommunicationPolicy(const capro::ServiceDescription& service) const noexcept
{
    // check if the publisher is already in the list
    for (auto publisherPortData : m_portPool->getPublisherPortDataList())
    {
        popo::PublisherPortRouDi publisherPort(publisherPortData);
        if (service == publisherPort.getCaProServiceDescription())
        {
            return cxx::make_optional<RuntimeName_t>(publisherPortData->m_runtimeName);
        }
    }
    return cxx::nullopt;
}

template <typename T, std::enable_if_t<std::is_same<T, iox::build::ManyToManyPolicy>::value>*>
inline cxx::optional<RuntimeName_t>
PortManager::doesViolateCommunicationPolicy(const capro::ServiceDescription& service IOX_MAYBE_UNUSED) const noexcept
{
    // Duplicates are allowed when using n:m policy
    return cxx::nullopt;
}

} // namespace roudi
} // namespace iox

#endif // IOX_POSH_INTERNAL_ROUDI_PORT_MANAGER_INL
