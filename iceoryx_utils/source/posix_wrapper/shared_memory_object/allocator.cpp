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

#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/platform/platform_correction.hpp"

#include <iostream>

namespace iox
{
namespace posix
{
constexpr uint64_t Allocator::MEMORY_ALIGNMENT;
Allocator::Allocator(void* const startAddress, const uint64_t length) noexcept
    : m_startAddress(static_cast<byte_t*>(startAddress))
    , m_length(length)
{
    /// @todo memset to set memory and to avoid the usage of unavailable memory
}

void* Allocator::allocate(const uint64_t size, const uint64_t alignment) noexcept
{
    cxx::Expects(size > 0);

    cxx::Expects(
        !m_allocationFinalized
        && "allocate() call after finalizeAllocation()! You are not allowed to acquire shared memory chunks anymore");

    uint64_t currentAddress = reinterpret_cast<uint64_t>(m_startAddress) + m_currentPosition;
    uint64_t alignedPosition = cxx::align(currentAddress, static_cast<uint64_t>(alignment));
    alignedPosition -= reinterpret_cast<uint64_t>(m_startAddress);

    byte_t* l_returnValue = nullptr;

    if (m_length >= alignedPosition + size)
    {
        l_returnValue = m_startAddress + alignedPosition;
        m_currentPosition = alignedPosition + size;
    }
    else
    {
        std::cerr << "Trying to allocate additional " << size << " bytes in the shared memory of capacity " << m_length
                  << " when there are already " << alignedPosition << " aligned bytes in use." << std::endl;
        std::cerr << "Only " << m_length - alignedPosition << " bytes left." << std::endl;

        cxx::Expects(false && "Not enough space left in shared memory");
    }

    return static_cast<void*>(l_returnValue);
}

void Allocator::finalizeAllocation() noexcept
{
    m_allocationFinalized = true;
}

} // namespace posix
} // namespace iox
