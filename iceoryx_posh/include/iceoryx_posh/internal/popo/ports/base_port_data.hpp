// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_PORTS_BASE_PORT_DATA_HPP
#define IOX_POSH_POPO_PORTS_BASE_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/typed_unique_id.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"

#include <atomic>

namespace iox
{
namespace popo
{
/// @brief Defines different base port data
struct BasePortData
{
    /// @brief Constructor for base port data members
    BasePortData() = default;

    /// @brief Constructor
    /// @param[in] serviceDescription creates the service service description
    /// @param[in] runtimeName Name of the application's runtime
    /// @param[in] nodeName Name of the node
    BasePortData(const capro::ServiceDescription& serviceDescription,
                 const RuntimeName_t& runtimeName,
                 const NodeName_t& nodeName) noexcept;

    BasePortData(const BasePortData&) = delete;
    BasePortData& operator=(const BasePortData&) = delete;
    BasePortData(BasePortData&&) = delete;
    BasePortData& operator=(BasePortData&&) = delete;
    ~BasePortData() = default;

    capro::ServiceDescription m_serviceDescription;
    RuntimeName_t m_runtimeName;
    NodeName_t m_nodeName;
    UniquePortId m_uniqueId;
    std::atomic_bool m_toBeDestroyed{false};
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_BASE_PORT_DATA_HPP
