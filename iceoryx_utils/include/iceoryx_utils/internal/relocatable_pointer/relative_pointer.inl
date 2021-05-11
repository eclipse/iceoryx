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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_RELATIVE_POINTER_INL
#define IOX_UTILS_RELOCATABLE_POINTER_RELATIVE_POINTER_INL

#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"

namespace iox
{
namespace rp
{
template <typename T>
inline RelativePointer<T>::RelativePointer(ptr_t ptr, id_t id) noexcept
    : BaseRelativePointer(ptr, id)
{
}

template <typename T>
inline RelativePointer<T>::RelativePointer(offset_t offset, id_t id) noexcept
    : BaseRelativePointer(offset, id)
{
}

template <typename T>
inline RelativePointer<T>::RelativePointer(ptr_t ptr) noexcept
    : BaseRelativePointer(ptr)
{
}


template <typename T>
inline RelativePointer<T>::RelativePointer(const BaseRelativePointer& other) noexcept
    : BaseRelativePointer(other)
{
}

template <typename T>
inline RelativePointer<T>& RelativePointer<T>::operator=(const BaseRelativePointer& other) noexcept
{
    BaseRelativePointer::operator=(other);

    return *this;
}

template <typename T>
inline RelativePointer<T>& RelativePointer<T>::operator=(ptr_t ptr) noexcept
{
    m_id = searchId(ptr);
    m_offset = computeOffset(ptr);

    return *this;
}

template <typename T>
template <typename U>
inline typename std::enable_if<!std::is_void<U>::value, U&>::type RelativePointer<T>::operator*() noexcept
{
    return *get();
}

template <typename T>
inline T* RelativePointer<T>::operator->() noexcept
{
    return get();
}

template <typename T>
template <typename U>
inline typename std::enable_if<!std::is_void<U>::value, const U&>::type RelativePointer<T>::operator*() const noexcept
{
    return *get();
}

template <typename T>
inline T* RelativePointer<T>::operator->() const noexcept
{
    return get();
}

template <typename T>
inline T* RelativePointer<T>::get() const noexcept
{
    return static_cast<T*>(computeRawPtr());
}

template <typename T>
inline RelativePointer<T>::operator T*() const noexcept
{
    return get();
}

template <typename T>
inline bool RelativePointer<T>::operator==(T* const ptr) const noexcept
{
    return ptr == get();
}

template <typename T>
inline bool RelativePointer<T>::operator!=(T* const ptr) const noexcept
{
    return ptr != get();
}
} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_RELATIVE_POINTER_INL

