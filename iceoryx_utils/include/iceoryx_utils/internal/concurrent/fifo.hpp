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
#ifndef IOX_UTILS_CONCURRENT_FIFO_HPP
#define IOX_UTILS_CONCURRENT_FIFO_HPP

#include "iceoryx_utils/cxx/optional.hpp"

#include <atomic>

namespace iox
{
namespace concurrent
{
/// @brief single pusher single pop'er thread safe fifo
template <typename ValueType, uint64_t Capacity>
class FiFo
{
  public:
    /// @brief pushes a value into the fifo
    /// @return if the values was pushed successfully into the fifo it returns
    ///         true, otherwise false
    bool push(const ValueType& f_value) noexcept;

    /// @brief returns the oldest value from the fifo and removes it
    /// @return if the fifo was not empty the optional contains the value,
    ///         otherwise it contains a nullopt
    cxx::optional<ValueType> pop() noexcept;

    /// @brief returns true when the fifo is empty, otherwise false
    bool empty() const noexcept;

    /// @brief returns the size of the fifo
    uint64_t size() const noexcept;

    /// @brief returns the capacity of the fifo
    static constexpr uint64_t capacity() noexcept;

  private:
    bool is_full() const noexcept;

  private:
    ValueType m_data[Capacity];
    std::atomic<uint64_t> m_write_pos{0};
    std::atomic<uint64_t> m_read_pos{0};
};

} // namespace concurrent
} // namespace iox

#include "iceoryx_utils/internal/concurrent/fifo.inl"

#endif // IOX_UTILS_CONCURRENT_FIFO_HPP
