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

#ifndef IOX_POSH_POPO_PORTS_SUBSCRIBER_PORT_DATA_HPP
#define IOX_POSH_POPO_PORTS_SUBSCRIBER_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port_data.hpp"
#include "iceoryx_utils/cxx/variant_queue.hpp"

#include <atomic>

namespace iox
{
namespace popo
{
struct SubscriberOptions;

struct SubscriberPortData : public BasePortData
{
    SubscriberPortData(const capro::ServiceDescription& serviceDescription,
                       const RuntimeName_t& runtimeName,
                       cxx::VariantQueueTypes queueType,
                       const SubscriberOptions& subscriberOptions,
                       const mepoo::MemoryInfo& memoryInfo = mepoo::MemoryInfo()) noexcept;

    using ChunkQueueData_t = ChunkQueueData<DefaultChunkQueueConfig, ThreadSafePolicy>;
    using ChunkReceiverData_t = ChunkReceiverData<MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY, ChunkQueueData_t>;

    ChunkReceiverData_t m_chunkReceiverData;
    const uint64_t m_historyRequest;
    std::atomic_bool m_subscribeRequested{false};
    std::atomic<SubscribeState> m_subscriptionState{SubscribeState::NOT_SUBSCRIBED};
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_SUBSCRIBER_PORT_DATA_HPP
