// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/cxx/algorithm.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

#include <algorithm>
#include <cstdint>

namespace iox
{
namespace mepoo
{
void MemoryManager::printMemPoolVector(log::LogStream& log) const noexcept
{
    for (auto& l_mempool : m_memPoolVector)
    {
        log << "  MemPool [ ChunkSize = " << l_mempool.getChunkSize()
            << ", ChunkPayloadSize = " << l_mempool.getChunkSize() - sizeof(ChunkHeader)
            << ", ChunkCount = " << l_mempool.getChunkCount() << " ]";
    }
}

void MemoryManager::addMemPool(posix::Allocator& managementAllocator,
                               posix::Allocator& chunkMemoryAllocator,
                               const cxx::greater_or_equal<uint32_t, MemPool::CHUNK_MEMORY_ALIGNMENT> chunkPayloadSize,
                               const cxx::greater_or_equal<uint32_t, 1> numberOfChunks) noexcept
{
    uint32_t adjustedChunkSize = sizeWithChunkHeaderStruct(static_cast<uint32_t>(chunkPayloadSize));
    if (m_denyAddMemPool)
    {
        LogFatal() << "After the generation of the chunk management pool you are not allowed to create new mempools.";
        errorHandler(Error::kMEPOO__MEMPOOL_ADDMEMPOOL_AFTER_GENERATECHUNKMANAGEMENTPOOL);
    }
    else if (m_memPoolVector.size() > 0 && adjustedChunkSize <= m_memPoolVector.back().getChunkSize())
    {
        auto log = LogFatal();
        log << "The following mempools were already added to the mempool handler:";
        printMemPoolVector(log);
        log << "These mempools must be added in an increasing chunk size ordering. The newly added  MemPool [ "
               "ChunkSize = "
            << adjustedChunkSize << ", ChunkPayloadSize = " << static_cast<uint32_t>(chunkPayloadSize)
            << ", ChunkCount = " << static_cast<uint32_t>(numberOfChunks) << "] breaks that requirement!";
        log.Flush();
        errorHandler(Error::kMEPOO__MEMPOOL_CONFIG_MUST_BE_ORDERED_BY_INCREASING_SIZE);
    }

    m_memPoolVector.emplace_back(adjustedChunkSize, numberOfChunks, managementAllocator, chunkMemoryAllocator);
    m_totalNumberOfChunks += numberOfChunks;
}

void MemoryManager::generateChunkManagementPool(posix::Allocator& managementAllocator) noexcept
{
    m_denyAddMemPool = true;
    uint32_t chunkSize = sizeof(ChunkManagement);
    m_chunkManagementPool.emplace_back(chunkSize, m_totalNumberOfChunks, managementAllocator, managementAllocator);
}

uint32_t MemoryManager::getNumberOfMemPools() const noexcept
{
    return static_cast<uint32_t>(m_memPoolVector.size());
}

MemPoolInfo MemoryManager::getMemPoolInfo(const uint32_t index) const noexcept
{
    if (index >= m_memPoolVector.size())
    {
        return {0, 0, 0, 0};
    }
    return m_memPoolVector[index].getInfo();
}

uint32_t MemoryManager::sizeWithChunkHeaderStruct(const MaxChunkPayloadSize_t size) noexcept
{
    return size + static_cast<uint32_t>(sizeof(ChunkHeader));
}

uint64_t MemoryManager::requiredChunkMemorySize(const MePooConfig& mePooConfig) noexcept
{
    uint64_t memorySize{0};
    for (const auto& mempoolConfig : mePooConfig.m_mempoolConfig)
    {
        // for the required chunk memory size only the size of the ChunkHeader
        // and the the chunk-payload size is taken into account;
        // the user has the option to further partition the chunk-payload with
        // a user-header and therefore reduce the user-payload size
        memorySize += cxx::align(static_cast<uint64_t>(mempoolConfig.m_chunkCount)
                                     * MemoryManager::sizeWithChunkHeaderStruct(mempoolConfig.m_size),
                                 MemPool::CHUNK_MEMORY_ALIGNMENT);
    }
    return memorySize;
}

uint64_t MemoryManager::requiredManagementMemorySize(const MePooConfig& mePooConfig) noexcept
{
    uint64_t memorySize{0U};
    uint32_t sumOfAllChunks{0U};
    for (const auto& mempool : mePooConfig.m_mempoolConfig)
    {
        sumOfAllChunks += mempool.m_chunkCount;
        memorySize +=
            cxx::align(static_cast<uint64_t>(MemPool::freeList_t::requiredIndexMemorySize(mempool.m_chunkCount)),
                       MemPool::CHUNK_MEMORY_ALIGNMENT);
    }

    memorySize +=
        cxx::align(static_cast<uint64_t>(sumOfAllChunks * sizeof(ChunkManagement)), MemPool::CHUNK_MEMORY_ALIGNMENT);
    memorySize += cxx::align(static_cast<uint64_t>(MemPool::freeList_t::requiredIndexMemorySize(sumOfAllChunks)),
                             MemPool::CHUNK_MEMORY_ALIGNMENT);

    return memorySize;
}

uint64_t MemoryManager::requiredFullMemorySize(const MePooConfig& mePooConfig) noexcept
{
    return requiredManagementMemorySize(mePooConfig) + requiredChunkMemorySize(mePooConfig);
}

void MemoryManager::configureMemoryManager(const MePooConfig& mePooConfig,
                                           posix::Allocator& managementAllocator,
                                           posix::Allocator& chunkMemoryAllocator) noexcept
{
    for (auto entry : mePooConfig.m_mempoolConfig)
    {
        addMemPool(managementAllocator, chunkMemoryAllocator, entry.m_size, entry.m_chunkCount);
    }

    generateChunkManagementPool(managementAllocator);
}

SharedChunk MemoryManager::getChunk(const ChunkSettings& chunkSettings) noexcept
{
    void* chunk{nullptr};
    MemPool* memPoolPointer{nullptr};
    const auto requiredChunkSize = chunkSettings.requiredChunkSize();

    uint32_t aquiredChunkSize = 0U;

    for (auto& memPool : m_memPoolVector)
    {
        uint32_t chunkSizeOfMemPool = memPool.getChunkSize();
        if (chunkSizeOfMemPool >= requiredChunkSize)
        {
            chunk = memPool.getChunk();
            memPoolPointer = &memPool;
            aquiredChunkSize = chunkSizeOfMemPool;
            break;
        }
    }

    if (m_memPoolVector.size() == 0)
    {
        LogFatal() << "There are no mempools available!";

        errorHandler(Error::kMEPOO__MEMPOOL_GETCHUNK_CHUNK_WITHOUT_MEMPOOL, nullptr, ErrorLevel::SEVERE);
        return SharedChunk(nullptr);
    }
    else if (memPoolPointer == nullptr)
    {
        auto log = LogFatal();
        log << "The following mempools are available:";
        printMemPoolVector(log);
        log << "Could not find a fitting mempool for a chunk of size " << requiredChunkSize;
        log.Flush();

        errorHandler(Error::kMEPOO__MEMPOOL_GETCHUNK_CHUNK_IS_TOO_LARGE, nullptr, ErrorLevel::SEVERE);
        return SharedChunk(nullptr);
    }
    else if (chunk == nullptr)
    {
        auto log = LogError();
        log << "MemoryManager: unable to acquire a chunk with a chunk-payload size of "
            << chunkSettings.userPayloadSize();
        log << "The following mempools are available:";
        printMemPoolVector(log);
        log.Flush();
        errorHandler(Error::kMEPOO__MEMPOOL_GETCHUNK_POOL_IS_RUNNING_OUT_OF_CHUNKS, nullptr, ErrorLevel::MODERATE);
        return SharedChunk(nullptr);
    }
    else
    {
        auto chunkHeader = new (chunk) ChunkHeader(aquiredChunkSize, chunkSettings);
        auto chunkManagement = new (m_chunkManagementPool.front().getChunk())
            ChunkManagement(chunkHeader, memPoolPointer, &m_chunkManagementPool.front());
        return SharedChunk(chunkManagement);
    }
}
} // namespace mepoo
} // namespace iox
