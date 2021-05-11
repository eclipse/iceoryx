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

#include "iceoryx_utils/internal/concurrent/loffli.hpp"
#include "iceoryx_utils/platform/platform_correction.hpp"

#include <cassert>

namespace iox
{
namespace concurrent
{
void LoFFLi::init(cxx::not_null<Index_t*> freeIndicesMemory, const uint32_t capacity) noexcept
{
    cxx::Expects(capacity > 0 && "A capacity of 0 is not supported!");
    constexpr uint32_t INTERNALLY_RESERVED_INDICES{1U};
    cxx::Expects(capacity < (std::numeric_limits<Index_t>::max() - INTERNALLY_RESERVED_INDICES)
                 && "Requested capacityexceeds limits!");
    cxx::Expects(m_head.is_lock_free() && "std::atomic<LoFFLi::Node> must be lock-free!");

    m_nextFreeIndex = freeIndicesMemory;
    m_size = capacity;
    m_invalidIndex = m_size + 1;

    if (m_nextFreeIndex != nullptr)
    {
        for (uint32_t i = 0; i < m_size + 1; i++)
        {
            m_nextFreeIndex[i] = i + 1;
        }
    }
}

bool LoFFLi::pop(Index_t& index) noexcept
{
    Node oldHead = m_head.load(std::memory_order_acquire);
    Node newHead = oldHead;

    do
    {
        // we are empty if next points to an element with index of Size
        if (oldHead.indexToNextFreeIndex >= m_size)
        {
            return false;
        }

        newHead.indexToNextFreeIndex = m_nextFreeIndex[oldHead.indexToNextFreeIndex];
        newHead.abaCounter += 1;
    } while (!m_head.compare_exchange_weak(oldHead, newHead, std::memory_order_acq_rel, std::memory_order_acquire));

    /// comes from outside, is not shared and therefore no synchronization is needed
    index = oldHead.indexToNextFreeIndex;
    /// @todo what if interrupted here an another thread guesses the index and
    ///         calls push
    /// @brief murphy case: m_nextFreeIndex does not require any synchronization since it
    ///         either is used by the same thread in push or it is given to another
    ///         thread which performs the cleanup and during this process a synchronization
    ///         is required
    m_nextFreeIndex[index] = m_invalidIndex;

    /// we need to synchronize m_nextFreeIndex with push so that we can perform a validation
    /// check right before push to avoid double free's
    std::atomic_thread_fence(std::memory_order_release);

    return true;
}

bool LoFFLi::push(const Index_t index) noexcept
{
    /// we synchronize with m_nextFreeIndex in pop to perform the validity check
    std::atomic_thread_fence(std::memory_order_release);

    /// we want to avoid double free's therefore we check if the index was acquired
    /// in pop and the push argument "index" is valid
    if (index >= m_size || m_nextFreeIndex[index] != m_invalidIndex)
    {
        return false;
    }

    Node oldHead = m_head.load(std::memory_order_acquire);
    Node newHead = oldHead;

    do
    {
        m_nextFreeIndex[index] = oldHead.indexToNextFreeIndex;
        newHead.indexToNextFreeIndex = index;
        newHead.abaCounter += 1;
    } while (!m_head.compare_exchange_weak(oldHead, newHead, std::memory_order_acq_rel, std::memory_order_acquire));

    return true;
}

} // namespace concurrent
} // namespace iox
