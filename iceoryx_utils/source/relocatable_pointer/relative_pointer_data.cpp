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

#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer_data.hpp"

namespace iox
{
namespace rp
{
// This is necessary if an supervising application needs to do a cleanup of resources hold by a crashed application. If
// the size is larger than 8 bytes on a 64 bit system, torn writes happens and if the application crashes at the wrong
// time, the supervisor reads corrupt data.
static_assert(sizeof(RelativePointerData) <= 8U, "The RelativePointerData size must not exceed 64 bit!");
// This ensures that the address of the RelativePointerData object is appropriately aligned to be accessed within one
// CPU cycle, i.e. if the size is 8 and the alignment is 4 it could be placed at an address with modulo 4 which would
// also result in torn writes.
static_assert(sizeof(RelativePointerData) == alignof(RelativePointerData),
              "A RelativePointerData must be placed on an address which does not cross the native alignment!");
// This is important for the use in the SOFI where under some conditions the copy operation could work on partially
// obsolet data and therefore non-trivial copy ctor/assignment operator or dtor would work on corrupted data.
static_assert(std::is_trivially_copyable<RelativePointerData>::value,
              "The RelativePointerData must be trivially copyable!");

constexpr RelativePointerData::id_t RelativePointerData::ID_RANGE;
constexpr RelativePointerData::id_t RelativePointerData::NULL_POINTER_ID;
constexpr RelativePointerData::id_t RelativePointerData::MAX_VALID_ID;
constexpr RelativePointerData::offset_t RelativePointerData::OFFSET_RANGE;
constexpr RelativePointerData::offset_t RelativePointerData::NULL_POINTER_OFFSET;
constexpr RelativePointerData::offset_t RelativePointerData::MAX_VALID_OFFSET;
constexpr uint64_t RelativePointerData::LOGICAL_NULLPTR;

RelativePointerData::id_t RelativePointerData::id() const noexcept
{
    return static_cast<id_t>(m_idAndOffset & ID_RANGE);
}

RelativePointerData::offset_t RelativePointerData::offset() const noexcept
{
    return (m_idAndOffset >> 16) & OFFSET_RANGE;
}

void RelativePointerData::reset() noexcept
{
    this->m_idAndOffset = LOGICAL_NULLPTR;
}

bool RelativePointerData::isLogicalNullptr() const noexcept
{
    return m_idAndOffset == LOGICAL_NULLPTR;
}

} // namespace rp
} // namespace iox
