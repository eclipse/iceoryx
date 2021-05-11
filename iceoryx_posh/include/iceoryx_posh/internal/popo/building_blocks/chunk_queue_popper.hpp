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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_POPPER_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_POPPER_HPP

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

namespace iox
{
namespace popo
{
/// @brief The ChunkQueuePopper is the low layer building block to receive SharedChunks. It follows a first-in-first-out
/// principle. Together with the ChunkDistributor and the ChunkQueuePusher, the ChunkQueuePopper builds the
/// infrastructure to exchange memory chunks between different data producers and consumers that could be located in
/// different processes. A ChunkQueuePopper is used to build elements of higher abstraction layers that also do memory
/// managemet and provide an API towards the real user
template <typename ChunkQueueDataType>
class ChunkQueuePopper
{
  public:
    using MemberType_t = ChunkQueueDataType;

    explicit ChunkQueuePopper(cxx::not_null<MemberType_t* const> chunkQueueDataPtr) noexcept;

    ChunkQueuePopper(const ChunkQueuePopper& other) = delete;
    ChunkQueuePopper& operator=(const ChunkQueuePopper&) = delete;
    ChunkQueuePopper(ChunkQueuePopper&& rhs) = default;
    ChunkQueuePopper& operator=(ChunkQueuePopper&& rhs) = default;
    virtual ~ChunkQueuePopper() = default;

    /// @brief pop a chunk from the chunk queue
    /// @return optional for a shared chunk that is set if the queue is not empty
    cxx::optional<mepoo::SharedChunk> tryPop() noexcept;

    /// @brief check if chunks were lost and reset flag
    /// @return true if the underlying queue has lost chunks due to an overflow since the last call of this method
    bool hasLostChunks() noexcept;

    /// @brief pop a chunk from the chunk queue
    /// @return if the queue is empty return true, otherwise false
    bool empty() const noexcept;

    /// @brief get the current size of the queue. Caution, another thread can have changed the size just after reading
    /// it
    /// @return queue size
    uint64_t size() noexcept;

    /// @brief set the capacity of the queue
    /// @param[in] newCapacity valid values are 0 < newCapacity < MAX_SUBSCRIBER_QUEUE_CAPACITY
    /// @pre it is important that no pop or push calls occur during this call
    /// @concurrent not thread safe
    void setCapacity(const uint64_t newCapacity) noexcept;

    /// @brief get the current capacity of the queue.
    /// @return current queue capacity
    uint64_t getCurrentCapacity() const noexcept;

    /// @brief get the maximum capacity of the queue.
    /// @return maximum capacity of this queue
    uint64_t getMaximumCapacity() const noexcept;

    /// @brief clear the queue
    void clear() noexcept;

    /// @brief Attaches a condition variable
    /// @param[in] ConditionVariableDataPtr, pointer to an condition variable data object
    void setConditionVariable(ConditionVariableData& conditionVariableDataRef,
                              const uint64_t notificationIndex) noexcept;

    /// @brief Detaches a condition variable
    void unsetConditionVariable() noexcept;

    /// @brief Returns the information whether a condition variable is attached
    /// @return true if condition variable is set, false if not
    bool isConditionVariableSet() const noexcept;

  protected:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

  private:
    MemberType_t* m_chunkQueueDataPtr;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.inl"

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_POPPER_HPP
