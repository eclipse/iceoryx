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

#include "iceoryx_posh/roudi/memory/memory_provider.hpp"

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/roudi/memory/memory_block.hpp"

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/base_relative_pointer.hpp"

/// @todo this should probably be moved to iceoryx_utils/allocator/bump_allocator.hpp
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

namespace iox
{
namespace roudi
{
MemoryProvider::~MemoryProvider() noexcept
{
    // destroy has to be called manually from outside, since it calls a pure virtual function
}

cxx::expected<MemoryProviderError> MemoryProvider::addMemoryBlock(cxx::not_null<MemoryBlock*> memoryBlock) noexcept
{
    if (isAvailable())
    {
        return cxx::error<MemoryProviderError>(MemoryProviderError::MEMORY_ALREADY_CREATED);
    }

    if (m_memoryBlocks.push_back(memoryBlock))
    {
        return cxx::success<void>();
    }
    return cxx::error<MemoryProviderError>(MemoryProviderError::MEMORY_BLOCKS_EXHAUSTED);
}

cxx::expected<MemoryProviderError> MemoryProvider::create() noexcept
{
    if (m_memoryBlocks.empty())
    {
        return cxx::error<MemoryProviderError>(MemoryProviderError::NO_MEMORY_BLOCKS_PRESENT);
    }

    if (isAvailable())
    {
        return cxx::error<MemoryProviderError>(MemoryProviderError::MEMORY_ALREADY_CREATED);
    }

    uint64_t totalSize = 0u;
    uint64_t maxAlignment = 1;
    for (auto memoryBlock : m_memoryBlocks)
    {
        auto alignment = memoryBlock->alignment();
        if (alignment > maxAlignment)
        {
            maxAlignment = alignment;
        }

        // just in case the memory block doesn't calculate its size as multiple of the alignment
        // this shouldn't be necessary, but also doesn't harm
        auto size = cxx::align(memoryBlock->size(), alignment);
        totalSize = cxx::align(totalSize, alignment) + size;
    }

    auto memoryResult = createMemory(totalSize, maxAlignment);

    if (memoryResult.has_error())
    {
        return memoryResult;
    }

    m_memory = memoryResult.value();
    m_size = totalSize;
    m_segmentId = rp::BaseRelativePointer::registerPtr(m_memory, m_size);

    LogDebug() << "Registered memory segment " << iox::log::HexFormat(reinterpret_cast<uint64_t>(m_memory))
               << " with size " << m_size << " to id " << m_segmentId;

    iox::posix::Allocator allocator(m_memory, m_size);

    for (auto memoryBlock : m_memoryBlocks)
    {
        memoryBlock->m_memory = allocator.allocate(memoryBlock->size(), memoryBlock->alignment());
    }

    return cxx::success<void>();
}

cxx::expected<MemoryProviderError> MemoryProvider::destroy() noexcept
{
    if (!isAvailable())
    {
        return cxx::error<MemoryProviderError>(MemoryProviderError::MEMORY_NOT_AVAILABLE);
    }

    for (auto memoryBlock : m_memoryBlocks)
    {
        memoryBlock->destroy();
    }

    auto destructionResult = destroyMemory();

    if (!destructionResult.has_error())
    {
        rp::BaseRelativePointer::unregisterPtr(m_segmentId);
        m_memory = nullptr;
        m_size = 0u;
    }

    return destructionResult;
}

cxx::optional<void*> MemoryProvider::baseAddress() const noexcept
{
    return isAvailable() ? cxx::make_optional<void*>(m_memory) : cxx::nullopt_t();
}

uint64_t MemoryProvider::size() const noexcept
{
    return m_size;
}

cxx::optional<uint64_t> MemoryProvider::segmentId() const noexcept
{
    return isAvailable() ? cxx::make_optional<uint64_t>(m_segmentId) : cxx::nullopt_t();
}

void MemoryProvider::announceMemoryAvailable() noexcept
{
    if (!m_memoryAvailableAnnounced)
    {
        for (auto memoryBlock : m_memoryBlocks)
        {
            memoryBlock->memoryAvailable(memoryBlock->m_memory);
        }

        m_memoryAvailableAnnounced = true;
    }
}

bool MemoryProvider::isAvailable() const noexcept
{
    return m_memory != nullptr;
}

bool MemoryProvider::isAvailableAnnounced() const noexcept
{
    return m_memoryAvailableAnnounced;
}


const char* MemoryProvider::getErrorString(const MemoryProviderError error)
{
    switch (error)
    {
    case MemoryProviderError::INVALID_STATE:
        return "MEMORY_BLOCKS_INVALID_STATE";
    case MemoryProviderError::MEMORY_BLOCKS_EXHAUSTED:
        return "MEMORY_BLOCKS_EXHAUSTED";
    case MemoryProviderError::NO_MEMORY_BLOCKS_PRESENT:
        return "NO_MEMORY_BLOCKS_PRESENT";
    case MemoryProviderError::MEMORY_ALREADY_CREATED:
        return "MEMORY_ALREADY_CREATED";
    case MemoryProviderError::MEMORY_CREATION_FAILED:
        return "MEMORY_CREATION_FAILED";
    case MemoryProviderError::PAGE_SIZE_CHECK_ERROR:
        return "PAGE_SIZE_CHECK_ERROR";
    case MemoryProviderError::MEMORY_ALIGNMENT_EXCEEDS_PAGE_SIZE:
        return "MEMORY_ALIGNMENT_EXCEEDS_PAGE_SIZE";
    case MemoryProviderError::MEMORY_ALLOCATION_FAILED:
        return "MEMORY_ALLOCATION_FAILED";
    case MemoryProviderError::MEMORY_MAPPING_FAILED:
        return "MEMORY_MAPPING_FAILED";
    case MemoryProviderError::MEMORY_NOT_AVAILABLE:
        return "MEMORY_NOT_AVAILABLE";
    case MemoryProviderError::MEMORY_DESTRUCTION_FAILED:
        return "MEMORY_DESTRUCTION_FAILED";
    case MemoryProviderError::MEMORY_DEALLOCATION_FAILED:
        return "MEMORY_DEALLOCATION_FAILED";
    case MemoryProviderError::MEMORY_UNMAPPING_FAILED:
        return "MEMORY_UNMAPPING_FAILED";
    case MemoryProviderError::SIGACTION_CALL_FAILED:
        return "SIGACTION_CALL_FAILED";
    }

    // this will actually never be reached, but the compiler issues a warning
    return "UNDEFINED";
}

} // namespace roudi
} // namespace iox
