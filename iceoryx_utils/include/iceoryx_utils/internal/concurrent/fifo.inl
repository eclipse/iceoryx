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
#ifndef IOX_UTILS_CONCURRENT_FIFO_INL
#define IOX_UTILS_CONCURRENT_FIFO_INL

#include "iceoryx_utils/internal/concurrent/fifo.hpp"

namespace iox
{
namespace concurrent
{
template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::push(const ValueType& f_param_r) noexcept
{
    if (is_full())
    {
        return false;
    }
    else
    {
        auto currentWritePos = m_write_pos.load(std::memory_order_relaxed);
        m_data[currentWritePos % Capacity] = f_param_r;

        // m_write_pos must be increased after writing the new value otherwise
        // it is possible that the value is read by pop while it is written.
        // this fifo is a single producer, single consumer fifo therefore
        // store is allowed.
        m_write_pos.store(currentWritePos + 1, std::memory_order_release);
        return true;
    }
}

template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::is_full() const noexcept
{
    return m_write_pos.load(std::memory_order_relaxed) == m_read_pos.load(std::memory_order_relaxed) + Capacity;
}

template <class ValueType, uint64_t Capacity>
inline uint64_t FiFo<ValueType, Capacity>::size() const noexcept
{
    return m_write_pos.load(std::memory_order_relaxed) - m_read_pos.load(std::memory_order_relaxed);
}

template <class ValueType, uint64_t Capacity>
inline constexpr uint64_t FiFo<ValueType, Capacity>::capacity() noexcept
{
    return Capacity;
}

template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::empty() const noexcept
{
    return m_read_pos.load(std::memory_order_relaxed) == m_write_pos.load(std::memory_order_relaxed);
}

template <class ValueType, uint64_t Capacity>
inline cxx::optional<ValueType> FiFo<ValueType, Capacity>::pop() noexcept
{
    auto currentReadPos = m_read_pos.load(std::memory_order_relaxed);
    bool isEmpty = (currentReadPos ==
                    // we are not allowed to use the empty method since we have to sync with
                    // the producer pop - this is done here
                    m_write_pos.load(std::memory_order_acquire));
    if (isEmpty)
    {
        return cxx::nullopt_t();
    }
    else
    {
        ValueType out = m_data[currentReadPos % Capacity];

        // m_read_pos must be increased after reading the pop'ed value otherwise
        // it is possible that the pop'ed value is overwritten by push while it is read.
        // Implementing a single consumer fifo here allows us to use store.
        m_read_pos.store(currentReadPos + 1, std::memory_order_relaxed);
        return out;
    }
}
} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_FIFO_INL
