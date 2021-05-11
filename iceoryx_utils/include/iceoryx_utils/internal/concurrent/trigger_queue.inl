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
#ifndef IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_INL
#define IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_INL

#include "iceoryx_utils/internal/concurrent/trigger_queue.hpp"

namespace iox
{
namespace concurrent
{
template <typename T, uint64_t Capacity, template <typename, uint64_t> class Queue>
inline bool QueueAdapter<T, Capacity, Queue>::push(Queue<T, Capacity>& queue, const T& in) noexcept
{
    return queue.push(in);
}

template <typename T, uint64_t Capacity>
inline bool QueueAdapter<T, Capacity, LockFreeQueue>::push(LockFreeQueue<T, Capacity>& queue, const T& in) noexcept
{
    return queue.tryPush(in);
}

template <typename T, uint64_t Capacity>
inline bool QueueAdapter<T, Capacity, ResizeableLockFreeQueue>::push(ResizeableLockFreeQueue<T, Capacity>& queue,
                                                                     const T& in) noexcept
{
    return queue.tryPush(in);
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline bool TriggerQueue<T, Capacity, QueueType>::push(const T& in) noexcept
{
    while (!m_toBeDestroyed.load(std::memory_order_relaxed) && !QueueAdapter<T, Capacity, QueueType>::push(m_queue, in))
    {
        std::this_thread::yield();
    }

    return !m_toBeDestroyed.load(std::memory_order_relaxed);
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline cxx::optional<T> TriggerQueue<T, Capacity, QueueType>::pop() noexcept
{
    return m_queue.pop();
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline bool TriggerQueue<T, Capacity, QueueType>::empty() const noexcept
{
    return m_queue.empty();
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline uint64_t TriggerQueue<T, Capacity, QueueType>::size() const noexcept
{
    return m_queue.size();
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline constexpr uint64_t TriggerQueue<T, Capacity, QueueType>::capacity() noexcept
{
    return Capacity;
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline void TriggerQueue<T, Capacity, QueueType>::destroy() noexcept
{
    m_toBeDestroyed.store(true, std::memory_order_relaxed);
}

template <typename T, uint64_t Capacity, template <typename, uint64_t> class QueueType>
inline bool TriggerQueue<T, Capacity, QueueType>::setCapacity(const uint64_t capacity) noexcept
{
    return m_queue.setCapacity(capacity);
}


} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_INL
