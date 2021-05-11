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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_POPPER_INL
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_POPPER_INL

#include "iceoryx_posh/internal/log/posh_logging.hpp"

namespace iox
{
namespace popo
{
template <typename ChunkQueueDataType>
inline ChunkQueuePopper<ChunkQueueDataType>::ChunkQueuePopper(
    cxx::not_null<MemberType_t* const> chunkQueueDataPtr) noexcept
    : m_chunkQueueDataPtr(chunkQueueDataPtr)
{
}

template <typename ChunkQueueDataType>
inline const typename ChunkQueuePopper<ChunkQueueDataType>::MemberType_t*
ChunkQueuePopper<ChunkQueueDataType>::getMembers() const noexcept
{
    return m_chunkQueueDataPtr;
}

template <typename ChunkQueueDataType>
inline typename ChunkQueuePopper<ChunkQueueDataType>::MemberType_t*
ChunkQueuePopper<ChunkQueueDataType>::getMembers() noexcept
{
    return m_chunkQueueDataPtr;
}

template <typename ChunkQueueDataType>
inline cxx::optional<mepoo::SharedChunk> ChunkQueuePopper<ChunkQueueDataType>::tryPop() noexcept
{
    auto retVal = getMembers()->m_queue.pop();

    // check if queue had an element that was poped and return if so
    if (retVal.has_value())
    {
        auto chunk = retVal.value().releaseToSharedChunk();

        auto receivedChunkHeaderVersion = chunk.getChunkHeader()->chunkHeaderVersion();
        if (receivedChunkHeaderVersion != mepoo::ChunkHeader::CHUNK_HEADER_VERSION)
        {
            LogError() << "Received chunk with CHUNK_HEADER_VERSION '" << receivedChunkHeaderVersion
                       << "' but expected '" << mepoo::ChunkHeader::CHUNK_HEADER_VERSION << "'! Dropping chunk!";
            errorHandler(Error::kPOPO__CHUNK_QUEUE_POPPER_CHUNK_WITH_INCOMPATIBLE_CHUNK_HEADER_VERSION,
                         nullptr,
                         ErrorLevel::SEVERE);
            return cxx::nullopt_t();
        }
        return cxx::make_optional<mepoo::SharedChunk>(chunk);
    }
    else
    {
        return cxx::nullopt_t();
    }
}

template <typename ChunkQueueDataType>
inline bool ChunkQueuePopper<ChunkQueueDataType>::hasLostChunks() noexcept
{
    if (getMembers()->m_queueHasLostChunks.load(std::memory_order_relaxed))
    {
        getMembers()->m_queueHasLostChunks.store(false, std::memory_order_relaxed);
        return true;
    }
    return false;
}

template <typename ChunkQueueDataType>
inline bool ChunkQueuePopper<ChunkQueueDataType>::empty() const noexcept
{
    return getMembers()->m_queue.empty();
}

template <typename ChunkQueueDataType>
inline uint64_t ChunkQueuePopper<ChunkQueueDataType>::size() noexcept
{
    return getMembers()->m_queue.size();
}

template <typename ChunkQueueDataType>
inline void ChunkQueuePopper<ChunkQueueDataType>::setCapacity(const uint64_t newCapacity) noexcept
{
    getMembers()->m_queue.setCapacity(newCapacity);
}

template <typename ChunkQueueDataType>
inline uint64_t ChunkQueuePopper<ChunkQueueDataType>::getCurrentCapacity() const noexcept
{
    return getMembers()->m_queue.capacity();
}

template <typename ChunkQueueDataType>
inline uint64_t ChunkQueuePopper<ChunkQueueDataType>::getMaximumCapacity() const noexcept
{
    return MemberType_t::MAX_CAPACITY;
}

template <typename ChunkQueueDataType>
inline void ChunkQueuePopper<ChunkQueueDataType>::clear() noexcept
{
    while (auto maybeUnmanagedChunk = getMembers()->m_queue.pop())
    {
        // PRQA S 4117 4 # d'tor of SharedChunk will release the memory, so RAII has the side effect here
        maybeUnmanagedChunk.value().releaseToSharedChunk();
    }
}

template <typename ChunkQueueDataType>
inline void ChunkQueuePopper<ChunkQueueDataType>::setConditionVariable(ConditionVariableData& conditionVariableDataRef,
                                                                       const uint64_t notificationIndex) noexcept
{
    typename MemberType_t::LockGuard_t lock(*getMembers());

    getMembers()->m_conditionVariableDataPtr = &conditionVariableDataRef;
    getMembers()->m_conditionVariableNotificationIndex.emplace(notificationIndex);
}

template <typename ChunkQueueDataType>
inline void ChunkQueuePopper<ChunkQueueDataType>::unsetConditionVariable() noexcept
{
    typename MemberType_t::LockGuard_t lock(*getMembers());

    getMembers()->m_conditionVariableDataPtr = nullptr;
    getMembers()->m_conditionVariableNotificationIndex.reset();
}

template <typename ChunkQueueDataType>
inline bool ChunkQueuePopper<ChunkQueueDataType>::isConditionVariableSet() const noexcept
{
    return getMembers()->m_conditionVariableDataPtr;
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_QUEUE_PUSHER_INL
