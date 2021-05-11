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
#ifndef IOX_POSH_POPO_PORTS_INTERFACE_PORT_DATA_HPP
#define IOX_POSH_POPO_PORTS_INTERFACE_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port_data.hpp"
#include "iceoryx_utils/internal/concurrent/fifo.hpp"

namespace iox
{
namespace popo
{
struct InterfacePortData : public BasePortData
{
    InterfacePortData() = default;
    InterfacePortData(const RuntimeName_t& runtimeName, const capro::Interfaces interface) noexcept;

    concurrent::FiFo<capro::CaproMessage, MAX_INTERFACE_CAPRO_FIFO_SIZE> m_caproMessageFiFo;
    bool m_doInitialOfferForward{true};
};
} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_INTERFACE_PORT_DATA_HPP
