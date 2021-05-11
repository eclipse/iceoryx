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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_INL
#define IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_INL

#include "iceoryx_utils/internal/relocatable_pointer/atomic_relocatable_pointer.hpp"

namespace iox
{
namespace rp
{
template <typename T>
inline AtomicRelocatablePointer<T>::AtomicRelocatablePointer(const T* ptr) noexcept
    : m_offset(computeOffset(ptr))
{
}

template <typename T>
inline AtomicRelocatablePointer<T>& AtomicRelocatablePointer<T>::operator=(const T* ptr) noexcept
{
    m_offset.store(computeOffset(ptr), std::memory_order_relaxed);
    return *this;
}

template <typename T>
inline T* AtomicRelocatablePointer<T>::operator->() const noexcept
{
    return computeRawPtr();
}

template <typename T>
inline T& AtomicRelocatablePointer<T>::operator*() const noexcept
{
    return *computeRawPtr();
}


template <typename T>
inline AtomicRelocatablePointer<T>::operator T*() const noexcept
{
    return computeRawPtr();
}

template <typename T>
inline T* AtomicRelocatablePointer<T>::computeRawPtr() const noexcept
{
    auto offset = m_offset.load(std::memory_order_relaxed);
    if (offset == NULL_POINTER_OFFSET)
    {
        return nullptr;
    }

    return reinterpret_cast<T*>(reinterpret_cast<offset_t>(&m_offset) - offset);
}

template <typename T>
inline typename AtomicRelocatablePointer<T>::offset_t
AtomicRelocatablePointer<T>::computeOffset(const void* ptr) const noexcept
{
    if (ptr == nullptr)
    {
        return NULL_POINTER_OFFSET;
    }
    return reinterpret_cast<offset_t>(&m_offset) - reinterpret_cast<offset_t>(ptr);
}
} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_INL
