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
#ifndef IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP
#define IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP

#include "iceoryx_utils/cxx/optional.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

namespace iox
{
namespace concurrent
{
template <typename ElementType, uint64_t Capacity>
class LockFreeQueue;
template <typename ElementType, uint64_t Capacity>
class ResizeableLockFreeQueue;

template <typename T, uint64_t Capacity, template <typename, uint64_t> class Queue>
struct QueueAdapter
{
    static bool push(Queue<T, Capacity>& queue, const T& in) noexcept;
};

template <typename T, uint64_t Capacity>
struct QueueAdapter<T, Capacity, LockFreeQueue>
{
    static bool push(LockFreeQueue<T, Capacity>& queue, const T& in) noexcept;
};

template <typename T, uint64_t Capacity>
struct QueueAdapter<T, Capacity, ResizeableLockFreeQueue>
{
    static bool push(ResizeableLockFreeQueue<T, Capacity>& queue, const T& in) noexcept;
};


/// @brief TriggerQueue is behaves exactly like a normal queue (fifo) except that
///         this queue is threadsafe and offers a blocking push which blocks the
///         the caller until the queue has space for at least one element which can
///         be pushed
template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
class TriggerQueue
{
  public:
    using ValueType = T;
    static constexpr uint64_t CAPACITY = Capacity;

    /// @brief Pushs an element into the trigger queue.
    ///         If the queue is full it blocks until there is space again.
    ///         If in the meantime destroy() was called the block is released and
    ///         push returns false.
    bool push(const T& in) noexcept;

    /// @brief  If the queue already contains an element it writes the contents
    ///         of that element in out and returns true, otherwise false.
    /// @return if an element could be removed the optional contains it, otherwise when the queue is empty
    ///         the optional is empty
    cxx::optional<T> pop() noexcept;

    /// @brief  Returns true if the queue is empty, otherwise false.
    bool empty() const noexcept;

    /// @brief  Returns the number of elements which are currently in the queue.
    uint64_t size() const noexcept;

    /// @brief  Returns the capacity of the trigger queue.
    static constexpr uint64_t capacity() noexcept;

    /// @brief when someone is waiting in push since the queue is full it
    ///        unblocks push. after that call it is impossible to push
    ///        elements.
    void destroy() noexcept;

    /// @brief resizes the queue.
    /// @return true if resize was successful otherwise false
    bool setCapacity(const uint64_t capacity) noexcept;

  private:
    QueueType<T, Capacity> m_queue;
    std::atomic_bool m_toBeDestroyed{false};
};
} // namespace concurrent
} // namespace iox

#include "iceoryx_utils/internal/concurrent/trigger_queue.inl"

#endif // IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP
