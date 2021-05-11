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
#ifndef IOX_POSH_MEPOO_SHARED_CHUNK_HPP
#define IOX_POSH_MEPOO_SHARED_CHUNK_HPP

#include "iceoryx_posh/internal/mepoo/chunk_management.hpp"
#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"

namespace iox
{
namespace mepoo
{
template <typename>
class SharedPointer;

/// @brief WARNING: SharedChunk is not thread safe! Don't share SharedChunk objects between threads! Use for each thread
/// a separate copy
class SharedChunk
{
  public:
    SharedChunk() = default;
    SharedChunk(ChunkManagement* const resource) noexcept;
    ~SharedChunk() noexcept;

    SharedChunk(const SharedChunk& rhs) noexcept;
    SharedChunk(SharedChunk&& rhs) noexcept;

    SharedChunk& operator=(const SharedChunk& rhs) noexcept;
    SharedChunk& operator=(SharedChunk&& rhs) noexcept;

    ChunkHeader* getChunkHeader() const noexcept;
    void* getUserPayload() const noexcept;

    ChunkManagement* release() noexcept;

    bool operator==(const SharedChunk& rhs) const noexcept;
    /// @todo use the newtype pattern to avoid the void pointer
    bool operator==(const void* const rhs) const noexcept;

    bool operator!=(const SharedChunk& rhs) const noexcept;
    bool operator!=(const void* const rhs) const noexcept;

    operator bool() const noexcept;

    template <typename>
    friend class SharedPointer;

  private:
    void decrementReferenceCounter() noexcept;
    void incrementReferenceCounter() noexcept;
    void freeChunk() noexcept;

  private:
    ChunkManagement* m_chunkManagement{nullptr};
};
} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_SHARED_CHUNK_HPP
