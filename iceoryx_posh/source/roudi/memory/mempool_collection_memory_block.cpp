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

#include "iceoryx_posh/internal/roudi/memory/mempool_collection_memory_block.hpp"

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"

#include "iceoryx_utils/cxx/algorithm.hpp"
#include "iceoryx_utils/internal/concurrent/loffli.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

namespace iox
{
namespace roudi
{
MemPoolCollectionMemoryBlock::MemPoolCollectionMemoryBlock(const mepoo::MePooConfig& memPoolConfig) noexcept
    : m_memPoolConfig(memPoolConfig)
{
}

MemPoolCollectionMemoryBlock::~MemPoolCollectionMemoryBlock() noexcept
{
    destroy();
}

uint64_t MemPoolCollectionMemoryBlock::size() const noexcept
{
    return cxx::align(static_cast<uint64_t>(sizeof(mepoo::MemoryManager)), mepoo::MemPool::CHUNK_MEMORY_ALIGNMENT)
           + mepoo::MemoryManager::requiredFullMemorySize(m_memPoolConfig);
}

uint64_t MemPoolCollectionMemoryBlock::alignment() const noexcept
{
    // algorithm::align doesn't like constexpr values
    auto memPoolAlignment = mepoo::MemPool::CHUNK_MEMORY_ALIGNMENT;
    return algorithm::max(static_cast<uint64_t>(alignof(mepoo::MemoryManager)), memPoolAlignment);
}

void MemPoolCollectionMemoryBlock::memoryAvailable(void* memory) noexcept
{
    posix::Allocator allocator(memory, size());
    auto memoryManager = allocator.allocate(sizeof(mepoo::MemoryManager), alignof(mepoo::MemoryManager));
    m_memoryManager = new (memoryManager) mepoo::MemoryManager;

    m_memoryManager->configureMemoryManager(m_memPoolConfig, allocator, allocator);
}

void MemPoolCollectionMemoryBlock::destroy() noexcept
{
    if (m_memoryManager)
    {
        m_memoryManager->~MemoryManager();
        m_memoryManager = nullptr;
    }
}

cxx::optional<mepoo::MemoryManager*> MemPoolCollectionMemoryBlock::memoryManager() const noexcept
{
    return m_memoryManager ? cxx::make_optional<mepoo::MemoryManager*>(m_memoryManager) : cxx::nullopt_t();
}

} // namespace roudi
} // namespace iox
