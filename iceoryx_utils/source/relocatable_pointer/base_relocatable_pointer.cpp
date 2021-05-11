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

#include "iceoryx_utils/internal/relocatable_pointer/base_relocatable_pointer.hpp"

namespace iox
{
namespace rp
{
BaseRelocatablePointer::BaseRelocatablePointer() noexcept
{
}

BaseRelocatablePointer::BaseRelocatablePointer(const void* ptr) noexcept
    : m_offset(computeOffset(ptr))
{
}

BaseRelocatablePointer::BaseRelocatablePointer(const BaseRelocatablePointer& other) noexcept
    : m_offset(computeOffset(other.computeRawPtr()))
{
}

BaseRelocatablePointer::BaseRelocatablePointer(BaseRelocatablePointer&& other) noexcept
    : m_offset(computeOffset(other.computeRawPtr()))
{
    /// @note could set other to null but there is no advantage in moving RelocatablePointers since they are
    /// lightweight and in principle other is allowed to still be functional (you just cannot rely on it)
}

BaseRelocatablePointer& BaseRelocatablePointer::operator=(const BaseRelocatablePointer& other) noexcept
{
    if (this != &other)
    {
        m_offset = computeOffset(other.computeRawPtr());
    }
    return *this;
}

BaseRelocatablePointer& BaseRelocatablePointer::operator=(const void* rawPtr) noexcept
{
    m_offset = computeOffset(rawPtr);
    return *this;
}

BaseRelocatablePointer& BaseRelocatablePointer::operator=(BaseRelocatablePointer&& other) noexcept
{
    m_offset = computeOffset(other.computeRawPtr());
    return *this;
}

const void* BaseRelocatablePointer::operator*() const noexcept
{
    return computeRawPtr();
}

BaseRelocatablePointer::operator bool() const noexcept
{
    return m_offset != NULL_POINTER_OFFSET;
}

bool BaseRelocatablePointer::operator!() const noexcept
{
    return !(operator bool());
}

const void* BaseRelocatablePointer::get() const noexcept
{
    return computeRawPtr();
}

BaseRelocatablePointer::offset_t BaseRelocatablePointer::getOffset() const noexcept
{
    return m_offset;
}

BaseRelocatablePointer::offset_t BaseRelocatablePointer::computeOffset(const void* ptr) const noexcept
{
    /// @todo find most efficient way to do this and check the valid range (signed/unsigned issues)
    /// this implies that the absolute difference cannot be larger than 2^63 which is probably true in any shared memory
    /// we use otherwise we would need to use unsigned for differences and use one extra bit from somewhere else to
    /// indicate the sign

    /// this suffices if both addresses are not too far apart, e.g. when they point to data in a sufficiently "small"
    /// shared memory (if the shared memory is small, the difference does never underflow)

    /// @todo better first cast to unsigned, then cast to signed later (extends range where it)
    return reinterpret_cast<offset_t>(&m_offset) - reinterpret_cast<offset_t>(ptr);
}

void* BaseRelocatablePointer::computeRawPtr() const noexcept
{
    if (m_offset == NULL_POINTER_OFFSET)
        return nullptr;

    /// @todo find most efficient way to do this (see above)
    return reinterpret_cast<void*>(reinterpret_cast<offset_t>(&m_offset) - m_offset);
}
} // namespace rp
} // namespace iox

