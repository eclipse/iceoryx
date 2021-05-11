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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_BASE_RELOCATABLE_POINTER_HPP
#define IOX_UTILS_RELOCATABLE_POINTER_BASE_RELOCATABLE_POINTER_HPP

#include <cstdint>
#include <limits>

namespace iox
{
namespace rp
{
/// @brief pointer class to use when pointer and pointee are located in the same shared memory segment
/// We can have the following scenario:
/// Pointer p points to object X of type T and both are stored in shared memory segment S.
///
/// Shared Memory   S:    p                  X
///                       |__________________^
/// App1            a1    b1                 c1
/// App2            a2    b2                 c2
///
/// Let a1, b1, c1 be the addresses of segment S, pointer p and object X in application 1 and similarly a2, b2, and c2
/// in application 2. If application 2 maps the memory differently they will be shifted by some common offset d
/// depending on the individual memory mapping: a2=a1+d, b2=b1+d, c2=c1+d
/// This is why storing a raw pointer to X will not be sufficient, the value of c1 will not point to X in application 2.
/// However, storing the difference between the location of p and X will work since it is an invariant in both address
/// spaces.
class BaseRelocatablePointer
{
    template <typename T>
    friend class RelocatablePointer;

  public:
    using offset_t = std::ptrdiff_t;

    /// @brief default constructs a logical nullptr
    BaseRelocatablePointer() noexcept;

    /// @brief creates a relocatable pointer pointing to the same pointee as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    explicit BaseRelocatablePointer(const void* ptr) noexcept;

    /// @brief copy constructor
    /// @param[in] other is the copy origin
    BaseRelocatablePointer(const BaseRelocatablePointer& other) noexcept;

    /// @brief move constructor
    /// @param[in] other is the move origin
    BaseRelocatablePointer(BaseRelocatablePointer&& other) noexcept;

    /// @brief copy assignment
    /// @param[in] other is the copy origin
    /// @return reference to self
    BaseRelocatablePointer& operator=(const BaseRelocatablePointer& other) noexcept;

    /// @brief assign BaseRelocatablePointer to point to the same pointee as rawPtr
    /// @param[in] rawPtr the pointer whose pointee shall be the same for this
    /// @return reference to self
    BaseRelocatablePointer& operator=(const void* rawPtr) noexcept;

    /// @brief move assignment
    /// @param[in] other is the move origin
    /// @return reference to self
    BaseRelocatablePointer& operator=(BaseRelocatablePointer&& other) noexcept;

    /// @brief read-only access to the underlying object in shared memory
    /// @return a const pointer to the underlying object
    const void* operator*() const noexcept;

    /// @brief checks if this is not a logical nullptr
    /// @return true if this is not a logical nullptr, otherwise false
    operator bool() const noexcept;

    /// @brief checks if this is a logical nullptr
    /// @return true if this is a logical nullptr, otherwise false
    bool operator!() const noexcept;

    /// @brief access to the underlying object in shared memory
    /// @return a pointer to the underlying object
    const void* get() const noexcept;

    /// @brief returns the offset
    /// @return offset
    offset_t getOffset() const noexcept;

    static constexpr offset_t NULL_POINTER_OFFSET = std::numeric_limits<offset_t>::max();

  protected:
    offset_t m_offset{NULL_POINTER_OFFSET};

    offset_t computeOffset(const void* ptr) const noexcept;

    void* computeRawPtr() const noexcept;
};

} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_BASE_RELOCATABLE_POINTER_HPP

