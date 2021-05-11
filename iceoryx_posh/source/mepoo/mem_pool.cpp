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

#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

#include <algorithm>

namespace iox
{
namespace mepoo
{
MemPoolInfo::MemPoolInfo(const uint32_t usedChunks,
                         const uint32_t minFreeChunks,
                         const uint32_t numChunks,
                         const uint32_t chunkSize) noexcept
    : m_usedChunks(usedChunks)
    , m_minFreeChunks(minFreeChunks)
    , m_numChunks(numChunks)
    , m_chunkSize(chunkSize)
{
}

constexpr uint64_t MemPool::CHUNK_MEMORY_ALIGNMENT;

MemPool::MemPool(const cxx::greater_or_equal<uint32_t, CHUNK_MEMORY_ALIGNMENT> chunkSize,
                 const cxx::greater_or_equal<uint32_t, 1> numberOfChunks,
                 posix::Allocator& managementAllocator,
                 posix::Allocator& chunkMemoryAllocator) noexcept
    : m_chunkSize(chunkSize)
    , m_numberOfChunks(numberOfChunks)
    , m_minFree(numberOfChunks)
{
    if (isMultipleOfAlignment(chunkSize))
    {
        m_rawMemory = static_cast<uint8_t*>(chunkMemoryAllocator.allocate(
            static_cast<uint64_t>(m_numberOfChunks) * m_chunkSize, CHUNK_MEMORY_ALIGNMENT));
        auto memoryLoFFLi =
            managementAllocator.allocate(freeList_t::requiredIndexMemorySize(m_numberOfChunks), CHUNK_MEMORY_ALIGNMENT);
        m_freeIndices.init(static_cast<concurrent::LoFFLi::Index_t*>(memoryLoFFLi), m_numberOfChunks);
    }
    else
    {
        std::cerr << chunkSize << " :: " << numberOfChunks << std::endl;
        errorHandler(Error::kMEPOO__MEMPOOL_CHUNKSIZE_MUST_BE_MULTIPLE_OF_CHUNK_MEMORY_ALIGNMENT);
    }
}

bool MemPool::isMultipleOfAlignment(const uint32_t value) const noexcept
{
    return (value % CHUNK_MEMORY_ALIGNMENT == 0U);
}

void MemPool::adjustMinFree() noexcept
{
    // @todo rethink the concurrent change that can happen. do we need a CAS loop?
    m_minFree.store(std::min(m_numberOfChunks - m_usedChunks.load(std::memory_order_relaxed),
                             m_minFree.load(std::memory_order_relaxed)));
}

void* MemPool::getChunk() noexcept
{
    uint32_t l_index{0U};
    if (!m_freeIndices.pop(l_index))
    {
        std::cerr << "Mempool [m_chunkSize = " << m_chunkSize << ", numberOfChunks = " << m_numberOfChunks
                  << ", used_chunks = " << m_usedChunks << " ] has no more space left" << std::endl;
        return nullptr;
    }

    /// @todo: verify that m_usedChunk is not changed during adjustMInFree
    ///         without changing m_minFree
    m_usedChunks.fetch_add(1U, std::memory_order_relaxed);
    adjustMinFree();

    return m_rawMemory + l_index * m_chunkSize;
}

void MemPool::freeChunk(const void* chunk) noexcept
{
    cxx::Expects(m_rawMemory <= chunk
                 && chunk <= m_rawMemory + (static_cast<uint64_t>(m_chunkSize) * (m_numberOfChunks - 1U)));

    auto offset = static_cast<const uint8_t*>(chunk) - m_rawMemory;
    cxx::Expects(offset % m_chunkSize == 0);

    uint32_t index = static_cast<uint32_t>(offset / m_chunkSize);

    if (!m_freeIndices.push(index))
    {
        errorHandler(Error::kPOSH__MEMPOOL_POSSIBLE_DOUBLE_FREE);
    }

    m_usedChunks.fetch_sub(1U, std::memory_order_relaxed);
}

uint32_t MemPool::getChunkSize() const noexcept
{
    return m_chunkSize;
}

uint32_t MemPool::getChunkCount() const noexcept
{
    return m_numberOfChunks;
}

uint32_t MemPool::getUsedChunks() const noexcept
{
    return m_usedChunks.load(std::memory_order_relaxed);
}

uint32_t MemPool::getMinFree() const noexcept
{
    return m_minFree.load(std::memory_order_relaxed);
}

MemPoolInfo MemPool::getInfo() const noexcept
{
    return {m_usedChunks.load(std::memory_order_relaxed),
            m_minFree.load(std::memory_order_relaxed),
            m_numberOfChunks,
            m_chunkSize};
}

} // namespace mepoo
} // namespace iox
