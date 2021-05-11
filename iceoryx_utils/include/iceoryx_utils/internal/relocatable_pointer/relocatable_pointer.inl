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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_INL
#define IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_INL

#include "iceoryx_utils/internal/relocatable_pointer/relocatable_pointer.hpp"

namespace iox
{
namespace rp
{
template <typename T>
inline RelocatablePointer<T>::RelocatablePointer() noexcept
    : BaseRelocatablePointer()
{
}

template <typename T>
inline RelocatablePointer<T>::RelocatablePointer(const T* ptr) noexcept
    : BaseRelocatablePointer(ptr)
{
}

template <typename T>
inline RelocatablePointer<T>::RelocatablePointer(const BaseRelocatablePointer& other) noexcept
{
    m_offset = computeOffset(other.computeRawPtr());
}

template <typename T>
inline RelocatablePointer<T>::RelocatablePointer(T* rawPtr) noexcept
{
    m_offset = computeOffset(rawPtr);
}

template <typename T>
inline RelocatablePointer<T>& RelocatablePointer<T>::operator=(const BaseRelocatablePointer& other) noexcept
{
    m_offset = computeOffset(other.computeRawPtr());

    return *this;
}

template <typename T>
inline T& RelocatablePointer<T>::operator*() noexcept
{
    return *(static_cast<T*>(computeRawPtr()));
}

template <typename T>
inline T* RelocatablePointer<T>::operator->() noexcept
{
    return static_cast<T*>(computeRawPtr());
}

template <typename T>
inline const T& RelocatablePointer<T>::operator*() const noexcept
{
    return *(static_cast<T*>(computeRawPtr()));
}

template <typename T>
inline const T* RelocatablePointer<T>::operator->() const noexcept
{
    return static_cast<T*>(computeRawPtr());
}

template <typename T>
inline T& RelocatablePointer<T>::operator[](uint64_t index) noexcept
{
    auto ptr = static_cast<T*>(computeRawPtr());
    return *(ptr + index);
}

template <typename T>
inline RelocatablePointer<T>::operator T*() const noexcept
{
    return reinterpret_cast<T*>(computeRawPtr());
}

} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_INL

