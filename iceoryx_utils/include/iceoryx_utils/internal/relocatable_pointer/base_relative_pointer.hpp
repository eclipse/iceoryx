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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_BASE_RELATIVE_POINTER_HPP
#define IOX_UTILS_RELOCATABLE_POINTER_BASE_RELATIVE_POINTER_HPP

#include "pointer_repository.hpp"

#include <cstdint>

namespace iox
{
namespace rp
{
/// @brief pointer class to use when pointer and pointee are located in different shared memory segments
/// We can have the following scenario:
/// Pointer p is stored in segment S1 and points to object X of type T in segment S2.
///
/// Shared Memory   S1:  p              S2:  X
///                      |___________________^
/// App1            a1   b1             c1   d1
/// App2            a2   b2             c2   d2
///
/// Now it is no longer true in general that both segments will be offset by the same difference in App2 and therefore
/// relocatable pointers are no longer sufficient.
/// Relative pointers solve this problem by incorporating the information from where they need to measure differences
/// (i.e. relative to the given address). This requires an additional registration mechanism to be used by all
/// applications where the start addresses and the size of all segments to be used are registered. Since these start
/// address may differ between applications, each segment is identified by a unique id, which can be provided upon
/// registration by the first application. In the figure, this means that the starting addresses of both segments(a1, a2
/// and c1, c2) would have to be registered in both applications.
/// Once this registration is done, relative pointers can be constructed from raw pointers similar to relocatable
/// pointers.
/// @note It should be noted that relocating a memory segment will invalidate relative pointers, i.e. relative pointers
/// are NOT relocatable. This is because the registration mechanism cannot be automatically informed about the copy of a
/// whole segment, such a segment would have to be registered on its own (and the original segment deregistered).
class BaseRelativePointer
{
  public:
    using id_t = uint64_t;
    using ptr_t = void*;
    using const_ptr_t = const void* const;
    using offset_t = std::uintptr_t;

    /// @brief constructs a BaseRelativePointer pointing to the same pointee as ptr in a segment identified by id
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    /// @param[in] id is the unique id of the segment
    BaseRelativePointer(ptr_t ptr, id_t id) noexcept;

    /// @brief constructs a BaseRelativePointer from a given offset and segment id
    /// @param[in] offset is the offset
    /// @param[in] id is the unique id of the segment
    BaseRelativePointer(offset_t offset, id_t id) noexcept;

    /// @brief constructs a BaseRelativePointer pointing to the same pointer as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    BaseRelativePointer(ptr_t ptr = nullptr) noexcept;

    /// @brief copy constructor
    /// @param[in] other is the copy origin
    BaseRelativePointer(const BaseRelativePointer& other) noexcept;

    /// @brief move constructor
    /// @param[in] other is the move origin
    BaseRelativePointer(BaseRelativePointer&& other) noexcept;

    /// @brief copy assignment
    /// @param[in] other is the copy origin
    /// @return a reference to self
    BaseRelativePointer& operator=(const BaseRelativePointer& other) noexcept;

    /// @brief assigns the BaseRelativePointer to point to the same pointee as ptr
    /// @param[in] ptr the pointer whose pointee shall be the same for this
    /// @return reference to self
    BaseRelativePointer& operator=(void* ptr) noexcept;

    /// @brief move assignment
    /// @param[in] other is the move origin
    /// @return a reference to self
    BaseRelativePointer& operator=(BaseRelativePointer&& other) noexcept;

    /// @brief access to the underlying object
    /// @return a pointer to the underlying object
    ptr_t get() const noexcept;

    /// @brief returns the id which identifies the segment
    /// @return the id which identifies the segment
    id_t getId() const noexcept;

    /// @brief returns the offset
    /// @return the offset
    offset_t getOffset() const noexcept;

    /// @brief get the base pointer associated with this' id
    /// @return the registered base pointer
    ptr_t getBasePtr() const noexcept;

    /// @brief registers a memory segment at ptr with size of a new id
    /// @param[in] ptr starting address of the segment to be registered
    /// @param[in] size is the size of the segment
    /// @return id it was registered to
    static id_t registerPtr(const ptr_t ptr, uint64_t size = 0U) noexcept;

    /// @brief tries to register a memory segment with a given size starting at ptr to a given id
    /// @param[in] id is the id of the segment
    /// @param[in] ptr starting address of the segment to be registered
    /// @param[in] size is the size of the segment
    /// @return true if successful (id not occupied), false otherwise
    static bool registerPtr(const id_t id, const ptr_t ptr, uint64_t size = 0U) noexcept;

    /// @brief unregisters ptr with given id
    /// @param[in] id is the id of the segment
    /// @return true if successful (ptr was registered with this id before), false otherwise
    static bool unregisterPtr(const id_t id) noexcept;

    /// @brief get the base ptr associated with the given id
    /// @param[in] id is the id of the segment
    /// @return ptr registered at the given id, nullptr if none was registered
    static ptr_t getBasePtr(const id_t id) noexcept;

    /// @brief unregisters all ptr id pairs (leads to initial state)
    static void unregisterAll() noexcept;

    /// @brief get the offset from id and ptr
    /// @param[in] id is the id of the segment and is used to get the base pointer
    /// @param[in] ptr is the pointer whose offset should be calculated
    /// @return offset
    static offset_t getOffset(const id_t id, const_ptr_t ptr) noexcept;

    /// @brief get the pointer from id and offset ("inverse" to getOffset)
    /// @param[in] id is the id of the segment and is used to get the base pointer
    /// @param[in] offset is the offset for which the pointer should be calculated
    /// @return the pointer from id and offset
    static ptr_t getPtr(const id_t id, const offset_t offset) noexcept;

    /// @brief get the id for a given ptr
    /// @param[in] ptr the pointer whose corresponding id is searched for
    /// @return id the pointer was registered to
    static id_t searchId(ptr_t ptr) noexcept;

    /// @brief checks if given id is valid
    /// @param[in] id is the id to be checked
    /// @return true if the given id is valid, otherwise false
    static bool isValid(id_t id) noexcept;

    /// @brief returns the pointer repository
    /// @return the pointer repository
    static PointerRepository<id_t, ptr_t>& getRepository() noexcept;

    /// @brief get the offset from the start address of the segment and ptr
    /// @param[in] ptr is the pointer whose offset should be calculated
    /// @return offset
    offset_t computeOffset(ptr_t ptr) const noexcept;

    /// @brief get the pointer from stored id and offset
    /// @return the pointer for stored id and offset
    ptr_t computeRawPtr() const noexcept;

    static constexpr id_t NULL_POINTER_ID = std::numeric_limits<id_t>::max();
    static constexpr offset_t NULL_POINTER_OFFSET = std::numeric_limits<offset_t>::max();

  protected:
    id_t m_id{NULL_POINTER_ID};
    offset_t m_offset{NULL_POINTER_OFFSET};
};
} // namespace rp
} // namespace iox

#endif // IOX_UTILS_RELOCATABLE_POINTER_BASE_RELATIVE_POINTER_HPP

