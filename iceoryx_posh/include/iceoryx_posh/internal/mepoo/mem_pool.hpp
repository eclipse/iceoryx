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
#ifndef IOX_POSH_MEPOO_MEM_POOL_HPP
#define IOX_POSH_MEPOO_MEM_POOL_HPP

#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/internal/concurrent/loffli.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"

#include <atomic>
#include <cstdint>


namespace iox
{
namespace mepoo
{
struct MemPoolInfo
{
    MemPoolInfo(const uint32_t usedChunks,
                const uint32_t minFreeChunks,
                const uint32_t numChunks,
                const uint32_t chunkSize) noexcept;

    uint32_t m_usedChunks{0};
    uint32_t m_minFreeChunks{0};
    uint32_t m_numChunks{0};
    uint32_t m_chunkSize{0};
};

class MemPool
{
  public:
    using freeList_t = concurrent::LoFFLi;
    static constexpr uint64_t CHUNK_MEMORY_ALIGNMENT = 8U; // default alignment for 64 bit

    MemPool(const cxx::greater_or_equal<uint32_t, CHUNK_MEMORY_ALIGNMENT> chunkSize,
            const cxx::greater_or_equal<uint32_t, 1> numberOfChunks,
            posix::Allocator& managementAllocator,
            posix::Allocator& chunkMemoryAllocator) noexcept;

    MemPool(const MemPool&) = delete;
    MemPool(MemPool&&) = delete;
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(MemPool&&) = delete;

    void* getChunk() noexcept;
    uint32_t getChunkSize() const noexcept;
    uint32_t getChunkCount() const noexcept;
    uint32_t getUsedChunks() const noexcept;
    uint32_t getMinFree() const noexcept;
    MemPoolInfo getInfo() const noexcept;

    void freeChunk(const void* chunk) noexcept;

  private:
    void adjustMinFree() noexcept;
    bool isMultipleOfAlignment(const uint32_t value) const noexcept;

    rp::RelativePointer<uint8_t> m_rawMemory;

    uint32_t m_chunkSize{0U};
    /// needs to be 32 bit since loffli supports only 32 bit numbers
    /// (cas is only 64 bit and we need the other 32 bit for the aba counter)
    uint32_t m_numberOfChunks{0U};

    /// @todo: put this into one struct and in a separate class in concurrent.
    std::atomic<uint32_t> m_usedChunks{0U};
    std::atomic<uint32_t> m_minFree{0U};
    /// @todo: end

    freeList_t m_freeIndices;
};

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_MEM_POOL_HPP
