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
#ifndef IOX_POSH_POPO_PORTS_CLIENT_PORT_DATA_HPP
#define IOX_POSH_POPO_PORTS_CLIENT_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/client_server_port_types.hpp"

#include <atomic>
#include <cstdint>

namespace iox
{
namespace popo
{
struct ClientPortData : public BasePortData
{
    ClientPortData(const capro::ServiceDescription& serviceDescription,
                   const RuntimeName_t& runtimeName,
                   const NodeName_t& nodeName,
                   mepoo::MemoryManager* const memoryManager,
                   const mepoo::MemoryInfo& memoryInfo = mepoo::MemoryInfo()) noexcept;

    static constexpr SubscriberTooSlowPolicy CLIENT_SUBSCRIBER_POLICY = SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA;
    static constexpr QueueFullPolicy CLIENT_PUBLISHER_POLICY = QueueFullPolicy::DISCARD_OLDEST_DATA;
    ClientChunkSenderData_t m_chunkSenderData;
    ClientChunkReceiverData_t m_chunkReceiverData;
    std::atomic_bool m_connectRequested{false};
    std::atomic<ConnectionState> m_connectionState{ConnectionState::NOT_CONNECTED};
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_CLIENT_PORT_DATA_HPP
