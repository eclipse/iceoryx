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

#include "iceoryx_posh/internal/mepoo/chunk_management.hpp"
#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"

namespace iox
{
namespace mepoo
{
ChunkManagement::ChunkManagement(const cxx::not_null<base_t*> chunkHeader,
                                 const cxx::not_null<MemPool*> mempool,
                                 const cxx::not_null<MemPool*> chunkManagementPool) noexcept
    : m_chunkHeader(chunkHeader)
    , m_mempool(mempool)
    , m_chunkManagementPool(chunkManagementPool)
{
    static_assert(alignof(ChunkManagement) <= mepoo::MemPool::CHUNK_MEMORY_ALIGNMENT,
                  "The ChunkManagement must not exceed the alignment of the mempool chunks, which are aligned to "
                  "'MemPool::CHUNK_MEMORY_ALIGNMENT'!");
}


} // namespace mepoo
} // namespace iox
