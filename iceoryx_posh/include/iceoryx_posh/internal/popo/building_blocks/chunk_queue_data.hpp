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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_DATA_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_DATA_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/shm_safe_unmanaged_chunk.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/port_queue_policies.hpp"
#include "iceoryx_utils/cxx/variant_queue.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

namespace iox
{
namespace popo
{
template <typename ChunkQueueDataProperties, typename LockingPolicy>
struct ChunkQueueData : public LockingPolicy
{
    using ThisType_t = ChunkQueueData<ChunkQueueDataProperties, LockingPolicy>;
    using LockGuard_t = std::lock_guard<const ThisType_t>;
    using ChunkQueueDataProperties_t = ChunkQueueDataProperties;

    ChunkQueueData(const QueueFullPolicy policy, const cxx::VariantQueueTypes queueType) noexcept;

    static constexpr uint64_t MAX_CAPACITY = ChunkQueueDataProperties_t::MAX_QUEUE_CAPACITY;
    cxx::VariantQueue<mepoo::ShmSafeUnmanagedChunk, MAX_CAPACITY> m_queue;
    std::atomic_bool m_queueHasLostChunks{false};

    rp::RelativePointer<ConditionVariableData> m_conditionVariableDataPtr;
    cxx::optional<uint64_t> m_conditionVariableNotificationIndex;
    const QueueFullPolicy m_queueFullPolicy;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.inl"

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_DATA_HPP
