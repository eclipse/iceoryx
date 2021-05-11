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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_PUSHER_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_PUSHER_HPP

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace popo
{
/// @brief The ChunkQueuePusher is the low layer building block to push SharedChunks in a chunk queue.
/// Together with the ChunkDistributor and ChunkQueuePopper the ChunkQueuePusher builds the infrastructure
/// to exchange memory chunks between different data producers and consumers that could be located in different
/// processes. A ChunkQueuePusher is the part of the chunk queue that is knwon by the ChunkDistributor
template <typename ChunkQueueDataType>
class ChunkQueuePusher
{
  public:
    using MemberType_t = ChunkQueueDataType;

    explicit ChunkQueuePusher(cxx::not_null<MemberType_t* const> chunkQueueDataPtr) noexcept;

    ChunkQueuePusher(const ChunkQueuePusher& other) = delete;
    ChunkQueuePusher& operator=(const ChunkQueuePusher&) = delete;
    ChunkQueuePusher(ChunkQueuePusher&& rhs) = default;
    ChunkQueuePusher& operator=(ChunkQueuePusher&& rhs) = default;
    ~ChunkQueuePusher() = default;

    /// @brief push a new chunk to the chunk queue
    /// @param[in] shared chunk object
    /// @return false if a queue overflow occurred, otherwise true
    bool push(mepoo::SharedChunk chunk) noexcept;

    /// @brief tell the queue that it lost a chunk (e.g. because push failed and there will be no retry)
    void lostAChunk() noexcept;

  protected:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

  private:
    MemberType_t* m_chunkQueueDataPtr{nullptr};
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.inl"

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_PUSHER_HPP
