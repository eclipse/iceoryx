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
#ifndef IOX_UTILS_POSIX_WRAPPER_SHARED_MEMORY_OBJECT_HPP
#define IOX_UTILS_POSIX_WRAPPER_SHARED_MEMORY_OBJECT_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/design_pattern/creation.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/memory_map.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/shared_memory.hpp"
#include "iceoryx_utils/platform/stat.hpp"

#include <cstdint>

namespace iox
{
namespace posix
{
using byte_t = uint8_t;

enum class SharedMemoryObjectError
{
    INVALID_STATE,
    SHARED_MEMORY_CREATION_FAILED,
    MAPPING_SHARED_MEMORY_FAILED,
};

class SharedMemoryObject : public DesignPattern::Creation<SharedMemoryObject, SharedMemoryObjectError>
{
  public:
    static constexpr void* NO_ADDRESS_HINT = nullptr;
    SharedMemoryObject(const SharedMemoryObject&) = delete;
    SharedMemoryObject& operator=(const SharedMemoryObject&) = delete;
    SharedMemoryObject(SharedMemoryObject&&) = default;
    SharedMemoryObject& operator=(SharedMemoryObject&&) = default;
    ~SharedMemoryObject() = default;

    void* allocate(const uint64_t size, const uint64_t alignment);
    void finalizeAllocation();

    Allocator* getAllocator();
    void* getBaseAddress() const;

    uint64_t getSizeInBytes() const;
    int getFileHandle() const;

    friend class DesignPattern::Creation<SharedMemoryObject, SharedMemoryObjectError>;

  private:
    SharedMemoryObject(const SharedMemory::Name_t& name,
                       const uint64_t memorySizeInBytes,
                       const AccessMode accessMode,
                       const OwnerShip ownerShip,
                       const void* baseAddressHint,
                       const mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    bool isInitialized() const;

  private:
    uint64_t m_memorySizeInBytes;
    cxx::optional<SharedMemory> m_sharedMemory;
    cxx::optional<MemoryMap> m_memoryMap;
    cxx::optional<Allocator> m_allocator;

    bool m_isInitialized;
};
} // namespace posix
} // namespace iox

#endif // IOX_UTILS_POSIX_WRAPPER_SHARED_MEMORY_OBJECT_HPP
