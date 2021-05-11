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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_HPP
#define IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_HPP

#include "base_relocatable_pointer.hpp"

namespace iox
{
namespace rp
{
/// @brief typed version so we can use operator->
template <typename T>
class RelocatablePointer : public BaseRelocatablePointer
{
  public:
    /// @brief default constructs a logical nullptr
    RelocatablePointer() noexcept;

    /// @brief creates a RelocatablePointer pointing to the same pointee as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    RelocatablePointer(const T* ptr) noexcept;

    /// @brief creates a RelocatablePointer from a BaseRelocatablePointer
    /// @param[in] other is the BaseRelocatablePointer
    RelocatablePointer(const BaseRelocatablePointer& other) noexcept;

    /// @brief creates a RelocatablePointer pointing to the same pointee as rawPtr
    /// @param[in] rawPtr the pointer whose pointee shall be the same for this
    RelocatablePointer(T* rawPtr) noexcept;

    /// @brief assign this to point to the same pointee as the BaseRelocatablePointer other
    /// @param[in] other the pointer whose pointee shall be the same for this
    /// @return reference to self
    RelocatablePointer& operator=(const BaseRelocatablePointer& other) noexcept;

    /// @brief dereferencing operator which returns a reference to the pointee
    /// @return a reference to the pointee
    T& operator*() noexcept;

    /// @brief access to the underlying object in shared memory
    /// @return a pointer to the underlying object
    T* operator->() noexcept;

    /// @brief dereferencing operator which returns a const reference to the pointee
    /// @return a const reference to the pointee
    const T& operator*() const noexcept;

    /// @brief read-only access to the underlying object in shared memory
    /// @return a const pointer to the underlying object
    const T* operator->() const noexcept;

    /// @brief returns a reference to the memory location of the underlying object + an offset
    /// @param[in] index is the offset
    /// @return a reference to the memory location of the underlying object + an offset
    T& operator[](uint64_t index) noexcept;

    /// @brief converts the RelocatablePointer to a pointer of the type of the underlying object
    /// @return a pointer of type T pointing to the underlying object
    operator T*() const noexcept;
};
} // namespace rp
} // namespace iox

#include "iceoryx_utils/internal/relocatable_pointer/relocatable_pointer.inl"

#endif // IOX_UTILS_RELOCATABLE_POINTER_RELOCATABLE_POINTER_HPP
