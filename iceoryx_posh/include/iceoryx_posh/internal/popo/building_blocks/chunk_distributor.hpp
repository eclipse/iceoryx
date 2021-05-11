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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_HPP

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

#include <thread>

namespace iox
{
namespace popo
{
enum class ChunkDistributorError
{
    INVALID_STATE,
    QUEUE_CONTAINER_OVERFLOW,
    QUEUE_NOT_IN_CONTAINER
};

/// @brief The ChunkDistributor is the low layer building block to send SharedChunks to a dynamic number of ChunkQueus.
/// Together with the ChunkQueuePusher, the ChunkDistributor builds the infrastructure to exchange memory chunks between
/// different data producers and consumers that could be located in different processes. Besides a modifiable container
/// of ChunkQueues to which a SharedChunk can be deliverd, it holds a configurable history of last sent chunks. This
/// allows to provide a newly added queue a number of last chunks to start from. This is needed for functionality
/// known as latched topic in ROS or field in ara::com. A ChunkDistributor is used to build elements of higher
/// abstraction layers that also do memory managemet and provide an API towards the real user
///
/// About Concurrency:
/// This ChunkDistributor can be used with different LockingPolicies for different scenarios
/// When different threads operate on it (e.g. application sends chunks and RouDi adds and removes queues),
/// a locking policy must be used that ensures consistent data in the ChunkDistributorData.
/// @todo There are currently some challenge:
/// For the stored queues and the history, containers are used which are not thread safe. Therefore we use an
/// inter-process mutex. But this can lead to deadlocks if a user process gets terminated while one of its
/// threads is in the ChunkDistributor and holds a lock. An easier setup would be if changing the queues
/// by a middleware thread and sending chunks by the user process would not interleave. I.e. there is no concurrent
/// access to the containers. Then a memory synchronization would be sufficient.
/// The cleanup() call is the biggest challenge. This is used to free chunks that are still held by a not properly
/// terminated user application. Even if access from middleware and user threads do not overlap, the history
/// container to cleanup could be in an inconsistent state as the application was hard terminated while changing it.
/// We would need a container like the UsedChunkList to have one that is robust against such inconsistencies....
/// A perfect job for our future selves
template <typename ChunkDistributorDataType>
class ChunkDistributor
{
  public:
    using MemberType_t = ChunkDistributorDataType;
    using ChunkQueueData_t = typename ChunkDistributorDataType::ChunkQueueData_t;
    using ChunkQueuePusher_t = typename ChunkDistributorDataType::ChunkQueuePusher_t;

    explicit ChunkDistributor(cxx::not_null<MemberType_t* const> chunkDistrubutorDataPtr) noexcept;

    ChunkDistributor(const ChunkDistributor& other) = delete;
    ChunkDistributor& operator=(const ChunkDistributor&) = delete;
    ChunkDistributor(ChunkDistributor&& rhs) = default;
    ChunkDistributor& operator=(ChunkDistributor&& rhs) = default;
    virtual ~ChunkDistributor() = default;

    /// @brief Add a queue to the internal list of chunk queues to which chunks are delivered when calling
    /// deliverToAllStoredQueues
    /// @param[in] queueToAdd chunk queue to add to the list
    /// @param[in] requestedHistory number of last chunks from history to send if available. If history size is smaller
    /// then the available history size chunks are provided
    /// @return if the queue could be added it returns success, otherwiese a ChunkDistributor error
    cxx::expected<ChunkDistributorError> tryAddQueue(cxx::not_null<ChunkQueueData_t* const> queueToAdd,
                                                     const uint64_t requestedHistory = 0u) noexcept;

    /// @brief Remove a queue from the internal list of chunk queues
    /// @param[in] chunk queue to remove from the list
    /// @return if the queue could be removed it returns success, otherwiese a ChunkDistributor error
    cxx::expected<ChunkDistributorError> tryRemoveQueue(cxx::not_null<ChunkQueueData_t* const> queueToRemove) noexcept;

    /// @brief Delete all the stored chunk queues
    void removeAllQueues() noexcept;

    /// @brief Get the information whether there are any stored chunk queues
    /// @return true if there are stored chunk queues, false if not
    bool hasStoredQueues() const noexcept;

    /// @brief Deliver the provided shared chunk to all the stored chunk queues. The chunk will be added to the chunk
    /// history
    /// @param[in] shared chunk to be delivered
    void deliverToAllStoredQueues(mepoo::SharedChunk chunk) noexcept;

    /// @brief Deliver the provided shared chunk to the provided chunk queue. The chunk will NOT be added to the chunk
    /// history
    /// @param[in] chunk queue to which this chunk shall be delivered
    /// @param[in] shared chunk to be delivered
    /// @return false if a queue overflow occured, otherwise true
    bool deliverToQueue(cxx::not_null<ChunkQueueData_t* const> queue, mepoo::SharedChunk chunk) noexcept;

    /// @brief Update the chunk history but do not deliver the chunk to any chunk queue. E.g. use case is to to update a
    /// non offered field in ara
    /// @param[in] shared chunk add to the chunk history
    void addToHistoryWithoutDelivery(mepoo::SharedChunk chunk) noexcept;

    /// @brief Get the current size of the chunk history
    /// @return chunk history size
    uint64_t getHistorySize() noexcept;

    /// @brief Get the capacity of the chunk history
    /// @return chunk history capacity
    uint64_t getHistoryCapacity() const noexcept;

    /// @brief Clears the chunk history
    void clearHistory() noexcept;

    /// @brief cleanup the used shrared memory chunks
    void cleanup() noexcept;

  protected:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

  private:
    MemberType_t* m_chunkDistrubutorDataPtr{nullptr};
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor.inl"

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_HPP
