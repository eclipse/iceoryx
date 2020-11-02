// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_CONCURRENT_LOCKED_LOFFLI_HPP
#define IOX_UTILS_CONCURRENT_LOCKED_LOFFLI_HPP

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>

namespace iox
{
namespace concurrent
{
class LockedLoFFLi
{
  private:
    uint32_t m_size{0};
    uint32_t m_head{0};
    uint32_t* m_freeIndices{nullptr};

    using mutex_t = posix::mutex;
    mutable mutex_t m_accessMutex{mutex_t::Recursive::OFF, mutex_t::Robust::OFF};

    uint32_t m_invalidIndex{0};

  public:
    LockedLoFFLi() = default;
    /// Initializes the lock-free free-list
    /// @param [in] f_freeIndicesMemory pointer to a memory with the size calculated by requiredMemorySize()
    /// @param [in] f_size is the number of elements of the free-list; must be the same used at requiredMemorySize()
    void init(cxx::not_null<uint32_t*> f_freeIndicesMemory, const uint32_t f_size);

    /// Pop a value from the free-list
    /// @param [out] index for an element to use
    /// @return true if index is valid, false otherwise
    bool pop(uint32_t& index);

    /// Push previously poped element
    /// @param [in] index to previously poped element
    /// @return true if index is valid or not yet pushed, false otherwise
    bool push(const uint32_t index);

    /// Calculates the required memory size for a free-list
    /// @param [in] f_size is the number of elements of the free-list
    /// @return the required memory size for a free-list with f_size elements
    static inline constexpr std::size_t requiredMemorySize(const uint32_t f_size)
    {
        return (static_cast<size_t>(f_size) + 1u) * sizeof(uint32_t);
    }
};

} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_LOCKED_LOFFLI_HPP
