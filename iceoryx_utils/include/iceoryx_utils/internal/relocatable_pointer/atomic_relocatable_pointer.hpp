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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_HPP
#define IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_HPP

#include <atomic>
#include <limits>

namespace iox
{
namespace rp
{
/// @brief minimalistic relocatable pointer that can be written and read atomically and can be stored safely in shared
/// memory.
/// As the basic RelocatablePointer, it must point to something in the same shared memory segment as itself since the
/// internally used offset must be an invariant different across adress spaces. Rationale: the default
/// RelocatablePointer cannot be used in an atomic since the copy ctor is nontrivial.
template <typename T>
class AtomicRelocatablePointer
{
  public:
    using offset_t = std::ptrdiff_t;
    static constexpr offset_t NULL_POINTER_OFFSET = std::numeric_limits<offset_t>::max();

    /// @brief creates an AtomicRelocatablePointer pointing to the same pointee as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    AtomicRelocatablePointer(const T* ptr = nullptr) noexcept;

    /// @todo: can be implemented when needed, note that the offset must be recomputed during the move/copy
    AtomicRelocatablePointer(const AtomicRelocatablePointer&) = delete;
    AtomicRelocatablePointer& operator=(const AtomicRelocatablePointer& other) = delete;
    AtomicRelocatablePointer(AtomicRelocatablePointer&& other) = delete;
    AtomicRelocatablePointer& operator=(AtomicRelocatablePointer&& other) = delete;

    /// @note minimal set of required operators, can be extended later

    /// @brief assign AtomicRelocatablePointer to point to the same pointee as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    /// @return reference to self
    AtomicRelocatablePointer& operator=(const T* ptr) noexcept;

    /// @brief access to the underlying object in shared memory
    /// @return a pointer to the underlying object
    T* operator->() const noexcept;

    /// @brief dereferencing operator which returns a reference to the pointee
    /// @return a reference to the pointee
    T& operator*() const noexcept;

    /// @brief converts the AtomicRelocatablePointer to a pointer of type of the underlying object
    /// @return a pointer of type T pointing to the underlying object
    operator T*() const noexcept;

  private:
    std::atomic<offset_t> m_offset{NULL_POINTER_OFFSET};

    inline T* computeRawPtr() const noexcept;

    inline offset_t computeOffset(const void* ptr) const noexcept;
};
} // namespace rp
} // namespace iox

#include "iceoryx_utils/internal/relocatable_pointer/atomic_relocatable_pointer.inl"

#endif // IOX_UTILS_RELOCATABLE_POINTER_ATOMIC_RELOCATABLE_POINTER_HPP
