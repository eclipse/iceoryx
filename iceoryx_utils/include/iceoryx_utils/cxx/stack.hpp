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
#ifndef IOX_UTILS_CXX_STACK_HPP
#define IOX_UTILS_CXX_STACK_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include <cstdint>

namespace iox
{
namespace cxx
{
/// @brief stack implementation with a simple push pop interface
/// @tparam T type which the stack contains
/// @tparam Capacity the capacity of the stack
template <typename T, uint64_t Capacity>
class stack
{
  public:
    /// @brief returns the last pushed element when the stack contains elements
    ///         otherwise a cxx::nullopt
    cxx::optional<T> pop() noexcept;

    /// @brief pushed an element into the stack by forwarding all arguments
    ///        to the constructor of T
    /// @param[in] args arguments which will be perfectly forwarded to the constructor of T
    /// @return true if the push was successful, otherwise false
    template <typename... Targs>
    bool push(Targs&&... args) noexcept;

    /// @brief returns the stack size
    uint64_t size() const noexcept;

    /// @brief returns the stack capacity
    static constexpr uint64_t capacity() noexcept;

  private:
    using element_t = uint8_t[sizeof(T)];
    alignas(T) element_t m_data[Capacity];
    uint64_t m_size = 0U;
};
} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/stack.inl"

#endif
