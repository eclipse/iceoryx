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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_INL
#define IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_INL

#include "iceoryx_utils/internal/relocatable_pointer/pointer_repository.hpp"

namespace iox
{
namespace rp
{
template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline PointerRepository<id_t, ptr_t, CAPACITY>::PointerRepository() noexcept
    : m_info(CAPACITY)
{
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline bool PointerRepository<id_t, ptr_t, CAPACITY>::registerPtr(id_t id, ptr_t ptr, uint64_t size) noexcept
{
    if (id > MAX_ID)
    {
        return false;
    }
    if (m_info[id].basePtr == nullptr)
    {
        m_info[id].basePtr = ptr;
        m_info[id].endPtr = reinterpret_cast<ptr_t>(reinterpret_cast<uintptr_t>(ptr) + size - 1U);
        if (id > m_maxRegistered)
        {
            m_maxRegistered = id;
        }
        return true;
    }
    return false;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline id_t PointerRepository<id_t, ptr_t, CAPACITY>::registerPtr(const ptr_t ptr, uint64_t size) noexcept
{
    for (id_t id = 1U; id <= MAX_ID; ++id)
    {
        if (m_info[id].basePtr == nullptr)
        {
            m_info[id].basePtr = ptr;
            m_info[id].endPtr = reinterpret_cast<ptr_t>(reinterpret_cast<uintptr_t>(ptr) + size - 1U);
            if (id > m_maxRegistered)
            {
                m_maxRegistered = id;
            }
            return id;
        }
    }

    return INVALID_ID;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline bool PointerRepository<id_t, ptr_t, CAPACITY>::unregisterPtr(id_t id) noexcept
{
    if (id <= MAX_ID && id >= MIN_ID)
    {
        if (m_info[id].basePtr != nullptr)
        {
            m_info[id].basePtr = nullptr;

            /// @note do not search for next lower registered index but we could do it here
            return true;
        }
    }

    return false;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline void PointerRepository<id_t, ptr_t, CAPACITY>::unregisterAll() noexcept
{
    for (auto& info : m_info)
    {
        info.basePtr = nullptr;
    }
    m_maxRegistered = 0U;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline ptr_t PointerRepository<id_t, ptr_t, CAPACITY>::getBasePtr(id_t id) const noexcept
{
    if (id <= MAX_ID && id >= MIN_ID)
    {
        return m_info[id].basePtr;
    }

    /// @note for id 0 nullptr is returned, meaning we will later interpret a relative pointer by casting the offset
    /// into a pointer (i.e. we measure relative to 0)

    /// @note we cannot distinguish between not registered and nullptr registered, but we do not need to
    return nullptr;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline id_t PointerRepository<id_t, ptr_t, CAPACITY>::searchId(ptr_t ptr) const noexcept
{
    for (id_t id = 1U; id <= m_maxRegistered; ++id)
    {
        // return first id where the ptr is in the corresponding interval
        if (ptr >= m_info[id].basePtr && ptr <= m_info[id].endPtr)
        {
            return id;
        }
    }
    /// @note implicitly interpret the pointer as a regular pointer if not found
    /// by setting id to 0
    /// rationale: test cases work without registered shared memory and require
    /// this at the moment to avoid fundamental changes
    return 0U;
    // return INVALID_ID;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline bool PointerRepository<id_t, ptr_t, CAPACITY>::isValid(id_t id) const noexcept
{
    return id != INVALID_ID;
}

template <typename id_t, typename ptr_t, uint64_t CAPACITY>
inline void PointerRepository<id_t, ptr_t, CAPACITY>::print() const noexcept
{
    for (id_t id = 0U; id < m_info.size(); ++id)
    {
        auto ptr = m_info[id].basePtr;
        if (ptr != nullptr)
        {
            std::cout << id << " ---> " << ptr << std::endl;
        }
    }
}

} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_INL

