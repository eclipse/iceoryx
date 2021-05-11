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
#ifndef IOX_POSH_POPO_PORTS_PUBLISHER_PORT_DATA_HPP
#define IOX_POSH_POPO_PORTS_PUBLISHER_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_data.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_posh/popo/publisher_options.hpp"

#include <atomic>
#include <cstdint>

namespace iox
{
namespace popo
{
struct PublisherPortData : public BasePortData
{
    PublisherPortData(const capro::ServiceDescription& serviceDescription,
                      const RuntimeName_t& runtimeName,
                      mepoo::MemoryManager* const memoryManager,
                      const PublisherOptions& publisherOptions,
                      const mepoo::MemoryInfo& memoryInfo = mepoo::MemoryInfo()) noexcept;

    using ChunkQueueData_t = SubscriberPortData::ChunkQueueData_t;
    using ChunkDistributorData_t =
        ChunkDistributorData<DefaultChunkDistributorConfig, ThreadSafePolicy, ChunkQueuePusher<ChunkQueueData_t>>;
    using ChunkSenderData_t =
        ChunkSenderData<MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY, ChunkDistributorData_t>;

    ChunkSenderData_t m_chunkSenderData;
    std::atomic_bool m_offeringRequested{false};
    std::atomic_bool m_offered{false};
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_PUBLISHER_PORT_DATA_HPP
