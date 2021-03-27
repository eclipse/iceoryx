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

#ifndef IOX_POSH_POPO_USED_CHUNK_LIST_INL
#define IOX_POSH_POPO_USED_CHUNK_LIST_INL

namespace iox
{
namespace popo
{
template <uint32_t Capacity>
constexpr typename UsedChunkList<Capacity>::DataElement_t UsedChunkList<Capacity>::DATA_ELEMENT_LOGICAL_NULLPTR;

template <uint32_t Capacity>
UsedChunkList<Capacity>::UsedChunkList() noexcept
{
    static_assert(sizeof(DataElement_t) <= 8U, "The size of the data element type must not exceed 64 bit!");
    static_assert(std::is_trivially_copyable<DataElement_t>::value,
                  "The data element type must be trivially copyable!");

    init();
}

template <uint32_t Capacity>
bool UsedChunkList<Capacity>::insert(mepoo::SharedChunk chunk) noexcept
{
    auto hasFreeSpace = m_freeListHead != INVALID_INDEX;
    if (hasFreeSpace)
    {
        // get next free entry after freelistHead
        auto nextFree = m_listNodes[m_freeListHead];

        // freeListHead is getting new usedListHead, next of this entry is updated to next in usedList
        m_listNodes[m_freeListHead] = m_usedListHead;
        m_usedListHead = m_freeListHead;

        // store chunk mgmt ptr
        rp::RelativePointer<mepoo::ChunkManagement> ptr = chunk.release();
        auto id = ptr.getId();
        auto offset = ptr.getOffset();
        cxx::Ensures(id < DataElement_t::MAX_ID && "RelativePointer id must fit into id type!");
        cxx::Ensures(offset < DataElement_t::MAX_OFFSET && "RelativePointer offset must fit into offset type!");
        m_listData[m_usedListHead] = DataElement_t(static_cast<uint16_t>(id), offset);

        // set freeListHead to the next free entry
        m_freeListHead = nextFree;

        /// @todo can we do this cheaper with a global fence in cleanup?
        m_synchronizer.clear(std::memory_order_release);
        return true;
    }
    else
    {
        return false;
    }
}

template <uint32_t Capacity>
bool UsedChunkList<Capacity>::remove(const mepoo::ChunkHeader* chunkHeader, mepoo::SharedChunk& chunk) noexcept
{
    auto previous = INVALID_INDEX;

    // go through usedList with stored chunks
    for (auto current = m_usedListHead; current != INVALID_INDEX; current = m_listNodes[current])
    {
        if (!m_listData[current].isLogicalNullptr())
        {
            auto chunkMgmt =
                rp::RelativePointer<mepoo::ChunkManagement>(m_listData[current].offset(), m_listData[current].id());
            // does the entry match the one we want to remove?
            if (chunkMgmt->m_chunkHeader == chunkHeader)
            {
                // return the chunk mgmt entry as SharedChunk object
                chunk = mepoo::SharedChunk(chunkMgmt);
                m_listData[current] = DATA_ELEMENT_LOGICAL_NULLPTR;

                // remove index from used list
                if (current == m_usedListHead)
                {
                    m_usedListHead = m_listNodes[current];
                }
                else
                {
                    m_listNodes[previous] = m_listNodes[current];
                }

                // insert index to free list
                m_listNodes[current] = m_freeListHead;
                m_freeListHead = current;

                /// @todo can we do this cheaper with a global fence in cleanup?
                m_synchronizer.clear(std::memory_order_release);
                return true;
            }
        }
        previous = current;
    }
    return false;
}

template <uint32_t Capacity>
void UsedChunkList<Capacity>::cleanup() noexcept
{
    m_synchronizer.test_and_set(std::memory_order_acquire);

    for (auto& data : m_listData)
    {
        if (!data.isLogicalNullptr())
        {
            mepoo::SharedChunk{rp::RelativePointer<mepoo::ChunkManagement>(data.offset(), data.id())};
        }
    }

    init(); // just to save us from the future self
}

template <uint32_t Capacity>
void UsedChunkList<Capacity>::init() noexcept
{
    // build list
    for (uint32_t i = 0U; i < Capacity; ++i)
    {
        m_listNodes[i] = i + 1u;
    }

    if (Capacity > 0U)
    {
        m_listNodes[Capacity - 1U] = INVALID_INDEX; // just to save us from the future self
    }
    else
    {
        m_listNodes[0U] = INVALID_INDEX;
    }


    m_usedListHead = INVALID_INDEX;
    m_freeListHead = 0U;

    // clear data
    for (auto& data : m_listData)
    {
        data = DATA_ELEMENT_LOGICAL_NULLPTR;
    }

    m_synchronizer.clear(std::memory_order_release);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_USED_CHUNK_LIST_INL