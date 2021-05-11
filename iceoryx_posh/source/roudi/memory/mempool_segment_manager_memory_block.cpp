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

#include "iceoryx_posh/internal/roudi/memory/mempool_segment_manager_memory_block.hpp"

#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

namespace iox
{
namespace roudi
{
MemPoolSegmentManagerMemoryBlock::MemPoolSegmentManagerMemoryBlock(const mepoo::SegmentConfig& segmentConfig) noexcept
    : m_segmentConfig(segmentConfig)
{
}

MemPoolSegmentManagerMemoryBlock::~MemPoolSegmentManagerMemoryBlock() noexcept
{
    destroy();
}

uint64_t MemPoolSegmentManagerMemoryBlock::size() const noexcept
{
    return cxx::align(static_cast<uint64_t>(sizeof(mepoo::SegmentManager<>)), mepoo::MemPool::CHUNK_MEMORY_ALIGNMENT)
           + mepoo::SegmentManager<>::requiredManagementMemorySize(m_segmentConfig);
}

uint64_t MemPoolSegmentManagerMemoryBlock::alignment() const noexcept
{
    return algorithm::max(static_cast<uint64_t>(alignof(mepoo::SegmentManager<>)),
                          mepoo::MemPool::CHUNK_MEMORY_ALIGNMENT);
}

void MemPoolSegmentManagerMemoryBlock::memoryAvailable(void* memory) noexcept
{
    posix::Allocator allocator(memory, size());
    auto segmentManager = allocator.allocate(sizeof(mepoo::SegmentManager<>), alignof(mepoo::SegmentManager<>));
    m_segmentManager = new (segmentManager) mepoo::SegmentManager<>(m_segmentConfig, &allocator);
}

void MemPoolSegmentManagerMemoryBlock::destroy() noexcept
{
    if (m_segmentManager)
    {
        m_segmentManager->~SegmentManager<>();
        m_segmentManager = nullptr;
    }
}

cxx::optional<mepoo::SegmentManager<>*> MemPoolSegmentManagerMemoryBlock::segmentManager() const noexcept
{
    return m_segmentManager ? cxx::make_optional<mepoo::SegmentManager<>*>(m_segmentManager) : cxx::nullopt_t();
}

} // namespace roudi
} // namespace iox
