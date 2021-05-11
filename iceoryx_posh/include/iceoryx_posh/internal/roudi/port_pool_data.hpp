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
#ifndef IOX_POSH_ROUDI_PORT_POOL_DATA_HPP
#define IOX_POSH_ROUDI_PORT_POOL_DATA_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/internal/popo/ports/application_port.hpp"
#include "iceoryx_posh/internal/popo/ports/interface_port.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_data.hpp"
#include "iceoryx_posh/internal/runtime/node_data.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

namespace iox
{
namespace roudi
{
/// @brief workaround container until we have a fixed list with the needed functionality
template <typename T, uint64_t Capacity>
class FixedPositionContainer
{
  public:
    static constexpr uint64_t FIRST_ELEMENT = std::numeric_limits<uint64_t>::max();

    bool hasFreeSpace();

    template <typename... Targs>
    T* insert(Targs&&... args);

    void erase(T* const element);

    cxx::vector<T*, Capacity> content();

  private:
    cxx::vector<cxx::optional<T>, Capacity> m_data;
};

struct PortPoolData
{
    FixedPositionContainer<popo::InterfacePortData, MAX_INTERFACE_NUMBER> m_interfacePortMembers;
    FixedPositionContainer<popo::ApplicationPortData, MAX_PROCESS_NUMBER> m_applicationPortMembers;
    FixedPositionContainer<runtime::NodeData, MAX_NODE_NUMBER> m_nodeMembers;
    FixedPositionContainer<popo::ConditionVariableData, MAX_NUMBER_OF_CONDITION_VARIABLES> m_conditionVariableMembers;

    FixedPositionContainer<iox::popo::PublisherPortData, MAX_PUBLISHERS> m_publisherPortMembers;
    FixedPositionContainer<iox::popo::SubscriberPortData, MAX_SUBSCRIBERS> m_subscriberPortMembers;

    // required to be atomic since a service can be offered or stopOffered while reading
    // this variable in a user application
    std::atomic<uint64_t> m_serviceRegistryChangeCounter{0};
};

} // namespace roudi
} // namespace iox

#include "iceoryx_posh/internal/roudi/port_pool_data.inl"

#endif // IOX_POSH_ROUDI_PORT_POOL_DATA_BASE_HPP
