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

#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/platform/fcntl.hpp"
#include "iceoryx_utils/platform/unistd.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <bitset>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>

namespace iox
{
namespace posix
{
constexpr void* SharedMemoryObject::NO_ADDRESS_HINT;
constexpr uint64_t SIGBUS_ERROR_MESSAGE_LENGTH = 1024U;

static char sigbusErrorMessage[SIGBUS_ERROR_MESSAGE_LENGTH];
static std::mutex sigbusHandlerMutex;

static void memsetSigbusHandler(int)
{
    auto result = write(STDERR_FILENO, sigbusErrorMessage, strnlen(sigbusErrorMessage, SIGBUS_ERROR_MESSAGE_LENGTH));
    IOX_DISCARD_RESULT(result);
    _exit(EXIT_FAILURE);
}

SharedMemoryObject::SharedMemoryObject(const SharedMemory::Name_t& name,
                                       const uint64_t memorySizeInBytes,
                                       const AccessMode accessMode,
                                       const OwnerShip ownerShip,
                                       const void* baseAddressHint,
                                       const mode_t permissions)
    : m_memorySizeInBytes(cxx::align(memorySizeInBytes, Allocator::MEMORY_ALIGNMENT))
{
    m_isInitialized = true;

    SharedMemory::create(name, accessMode, ownerShip, permissions, m_memorySizeInBytes)
        .and_then([this](auto& sharedMemory) { m_sharedMemory.emplace(std::move(sharedMemory)); })
        .or_else([this](auto&) {
            std::cerr << "Unable to create SharedMemoryObject since we could not acquire a SharedMemory resource"
                      << std::endl;
            m_isInitialized = false;
            m_errorValue = SharedMemoryObjectError::SHARED_MEMORY_CREATION_FAILED;
        });

    if (m_isInitialized)
    {
        MemoryMap::create(baseAddressHint, m_memorySizeInBytes, m_sharedMemory->getHandle(), accessMode, MAP_SHARED, 0)
            .and_then([this](auto& memoryMap) { m_memoryMap.emplace(std::move(memoryMap)); })
            .or_else([this](auto) {
                std::cerr << "Failed to map created shared memory into process!" << std::endl;
                m_isInitialized = false;
                m_errorValue = SharedMemoryObjectError::MAPPING_SHARED_MEMORY_FAILED;
            });
    }

    if (m_isInitialized == false)
    {
        std::cerr << "Unable to create a shared memory object with the following properties [ name = " << name
                  << ", sizeInBytes = " << memorySizeInBytes
                  << ", access mode = " << ACCESS_MODE_STRING[static_cast<uint64_t>(accessMode)]
                  << ", ownership = " << OWNERSHIP_STRING[static_cast<uint64_t>(ownerShip)]
                  << ", baseAddressHint = " << std::hex << baseAddressHint
                  << ", permissions = " << std::bitset<sizeof(mode_t)>(permissions) << " ]" << std::endl;
        return;
    }

    m_allocator.emplace(m_memoryMap->getBaseAddress(), m_memorySizeInBytes);

    if (ownerShip == OwnerShip::MINE && m_isInitialized)
    {
        std::clog << "Reserving " << m_memorySizeInBytes << " bytes in the shared memory [" << name << "]" << std::endl;
        {
            // this lock is required for the case that multiple threads are creating multiple
            // shared memory objects concurrently
            std::lock_guard<std::mutex> lock(sigbusHandlerMutex);
            auto memsetSigbusGuard = registerSignalHandler(Signal::BUS, memsetSigbusHandler);

            snprintf(
                sigbusErrorMessage,
                SIGBUS_ERROR_MESSAGE_LENGTH,
                "While setting the acquired shared memory to zero a fatal SIGBUS signal appeared caused by memset. The "
                "shared memory object with the following properties [ name = %s, sizeInBytes = %llu, access mode = %s, "
                "ownership = %s, baseAddressHint = %p, permissions = %lu ] maybe requires more memory than it is "
                "currently available in the system.\n",
                name.c_str(),
                static_cast<unsigned long long>(memorySizeInBytes),
                ACCESS_MODE_STRING[static_cast<uint64_t>(accessMode)],
                OWNERSHIP_STRING[static_cast<uint64_t>(ownerShip)],
                baseAddressHint,
                std::bitset<sizeof(mode_t)>(permissions).to_ulong());

            memset(m_memoryMap->getBaseAddress(), 0, m_memorySizeInBytes);
        }
        std::clog << "[ Reserving shared memory successful ] " << std::endl;
    }
}

void* SharedMemoryObject::allocate(const uint64_t size, const uint64_t alignment)
{
    return m_allocator->allocate(size, alignment);
}

void SharedMemoryObject::finalizeAllocation()
{
    m_allocator->finalizeAllocation();
}

bool SharedMemoryObject::isInitialized() const
{
    return m_isInitialized;
}

Allocator* SharedMemoryObject::getAllocator()
{
    return &*m_allocator;
}

void* SharedMemoryObject::getBaseAddress() const
{
    return m_memoryMap->getBaseAddress();
}

uint64_t SharedMemoryObject::getSizeInBytes() const
{
    return m_memorySizeInBytes;
}

int32_t SharedMemoryObject::getFileHandle() const
{
    return m_sharedMemory->getHandle();
}

} // namespace posix
} // namespace iox
