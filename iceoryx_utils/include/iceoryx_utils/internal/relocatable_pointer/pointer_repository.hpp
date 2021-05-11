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

#ifndef IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_HPP
#define IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_HPP

#include "iceoryx_utils/cxx/vector.hpp"
#include <iostream>

#include <assert.h>

namespace iox
{
namespace rp
{
/// @brief Allows registration of memory segments with their start pointers and size.
/// This class is used to resolve relative pointers in the corresponding address space of the application.
/// Up to CAPACITY segments can be registered with MIN_ID = 1 to MAX_ID = CAPACITY - 1
/// id 0 is reserved and allows relative pointers to behave like normal pointers
/// (which is equivalent to measure the offset relative to 0).
template <typename id_t, typename ptr_t, uint64_t CAPACITY = 10000U>
class PointerRepository
{
  private:
    struct Info
    {
        ptr_t basePtr{nullptr};
        ptr_t endPtr{nullptr};
    };

    /// @note 0 is a special purpose id and reserved
    /// id 0 is reserved to interpret the offset just as a raw pointer,
    /// i.e. its corresponding base ptr is 0
    static constexpr size_t MIN_ID = 1U;
    static constexpr size_t MAX_ID = CAPACITY - 1U;

  public:
    static constexpr id_t INVALID_ID = std::numeric_limits<id_t>::max();

    /// @brief default constructor
    PointerRepository() noexcept;

    /// @brief registers the start pointer of the segment in another application with a specific id
    /// @param[in] id identifies the segment
    /// @param[in] ptr is the start pointer of the segment
    /// @param[in] size is the size of the segment
    /// @return true if the registration was successful, otherwise false
    bool registerPtr(id_t id, ptr_t ptr, uint64_t size) noexcept;

    /// @brief registers the start pointer of a segment with a specific size
    /// @param[in] ptr is the start pointer of the segment
    /// @param[in] size is the size of the segment
    /// @return the id that identifies the segment
    id_t registerPtr(const ptr_t ptr, uint64_t size = 0U) noexcept;

    /// @brief unregisters the id
    /// @param[in] id is the id to be unregistered
    /// @return true if successful, otherwise false
    /// @attention the relative pointers corresponding to this id become unsafe to use
    bool unregisterPtr(id_t id) noexcept;

    /// @brief unregisters all ids
    /// @attention the relative pointers corresponding to this id become unsafe to use
    void unregisterAll() noexcept;

    /// @brief gets the base pointer, i.e. the starting address, associated with id
    /// @param[in] id is the segment id
    /// @return the base pointer associated with the id
    ptr_t getBasePtr(id_t id) const noexcept;

    /// @brief returns the id for a given pointer ptr
    /// @param[in] ptr is the pointer whose corresponding id is searched for
    /// @return the id the pointer was registered to
    id_t searchId(ptr_t ptr) const noexcept;

    /// @brief checks if given id is valid
    /// @param[in] id is the id to be checked
    /// @return true if id is valid, otherwise false
    bool isValid(id_t id) const noexcept;

    /// @brief prints the ids and their associated base pointers
    void print() const noexcept;

  private:
    /// @todo: if required protect vector against concurrent modification
    /// whether this is required depends on the use case, we currently do not need it
    /// we control the ids, so if they are consecutive we only need a vector/array to get the address
    /// this variable exists once per application using relative pointers,
    /// and each needs to initialize it via register calls above

    iox::cxx::vector<Info, CAPACITY> m_info;
    uint64_t m_maxRegistered{0U};
};

} // namespace rp
} // namespace iox

#include "iceoryx_utils/internal/relocatable_pointer/pointer_repository.inl"

#endif // IOX_UTILS_RELOCATABLE_POINTER_POINTER_REPOSITORY_HPP
