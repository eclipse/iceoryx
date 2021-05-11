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

#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "test.hpp"

using namespace ::testing;
using namespace iox::mepoo;

class MemPool_test : public Test
{
  public:
    static constexpr uint32_t NumberOfChunks{100};
    static constexpr uint32_t ChunkSize{64};

    using FreeListIndex_t = iox::mepoo::MemPool::freeList_t::Index_t;
    static constexpr FreeListIndex_t LoFFLiMemoryRequirement{
        iox::mepoo::MemPool::freeList_t::requiredIndexMemorySize(NumberOfChunks) + 10000};

    MemPool_test()
        : allocator(m_rawMemory, NumberOfChunks * ChunkSize + LoFFLiMemoryRequirement)
        , sut(ChunkSize, NumberOfChunks, allocator, allocator)
    {
    }

    void SetUp(){};
    void TearDown(){};

    alignas(MemPool::CHUNK_MEMORY_ALIGNMENT) uint8_t m_rawMemory[NumberOfChunks * ChunkSize + LoFFLiMemoryRequirement];
    iox::posix::Allocator allocator;

    MemPool sut;
};

TEST_F(MemPool_test, CTor)
{
}

TEST_F(MemPool_test, WriteChunks)
{
    uint8_t* chunk = reinterpret_cast<uint8_t*>(sut.getChunk());
    uint8_t* chunk2 = reinterpret_cast<uint8_t*>(sut.getChunk());

    *chunk = 123;
    *chunk2 = 45;

    EXPECT_THAT(*chunk, Eq(123));
    EXPECT_THAT(*chunk2, Eq(45));
}


TEST_F(MemPool_test, WriteAllChunks)
{
    std::vector<uint8_t*> chunks;
    for (uint8_t i = 0; i < NumberOfChunks; i++)
    {
        auto chunk = sut.getChunk();
        ASSERT_THAT(chunk, Ne(nullptr));
        chunks.push_back(reinterpret_cast<uint8_t*>(chunk));
        *chunks.back() = i;
    }

    for (uint8_t i = 0; i < NumberOfChunks; i++)
    {
        EXPECT_THAT(*chunks[i], Eq(i));
    }
}

TEST_F(MemPool_test, GetChunkWhenFull)
{
    for (uint8_t i = 0; i < NumberOfChunks; i++)
    {
        sut.getChunk();
    }

    EXPECT_THAT(sut.getChunk(), Eq(nullptr));
}

TEST_F(MemPool_test, getChunkSize)
{
    EXPECT_THAT(sut.getChunkSize(), Eq(ChunkSize));
}

TEST_F(MemPool_test, getChunkCount)
{
    EXPECT_THAT(sut.getChunkCount(), Eq(NumberOfChunks));
}

TEST_F(MemPool_test, getUsedChunks)
{
    for (uint32_t i = 0; i < NumberOfChunks; ++i)
    {
        EXPECT_THAT(sut.getUsedChunks(), Eq(i));
        sut.getChunk();
    }
}

TEST_F(MemPool_test, freeChunk)
{
    std::vector<uint8_t*> chunks;
    for (uint32_t i = 0; i < NumberOfChunks; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }

    for (uint32_t i = 0; i < NumberOfChunks; ++i)
    {
        EXPECT_THAT(sut.getUsedChunks(), Eq(NumberOfChunks - i));
        sut.freeChunk(chunks[i]);
    }
}

TEST_F(MemPool_test, freeChunkAndGetChunkFull)
{
    std::vector<uint8_t*> chunks;
    for (uint32_t i = 0; i < NumberOfChunks; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }

    for (uint32_t i = 0; i < NumberOfChunks; ++i)
    {
        sut.freeChunk(chunks[i]);
    }

    chunks.clear();


    for (uint8_t i = 0; i < NumberOfChunks; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
        *chunks.back() = i;
    }


    for (uint8_t i = 0; i < NumberOfChunks; ++i)
    {
        EXPECT_THAT(*chunks[i], Eq(i));
    }
}

TEST_F(MemPool_test, getMinFreeSimpleGetChunk)
{
    std::vector<uint8_t*> chunks;
    for (uint32_t i = 0; i < NumberOfChunks / 2; ++i)
    {
        EXPECT_THAT(sut.getMinFree(), Eq(NumberOfChunks - i));
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }
}

TEST_F(MemPool_test, getMinFreeAfterFree)
{
    std::vector<uint8_t*> chunks;
    for (uint32_t i = 0; i < NumberOfChunks / 2; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }

    for (auto chunk : chunks)
    {
        sut.freeChunk(chunk);
        EXPECT_THAT(sut.getMinFree(), Eq(50u));
    }
}

TEST_F(MemPool_test, getMinFreeWithSecondGetChunk)
{
    std::vector<uint8_t*> chunks;
    for (uint32_t i = 0; i < NumberOfChunks / 2; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }

    for (auto chunk : chunks)
    {
        sut.freeChunk(chunk);
    }

    chunks.clear();


    for (uint32_t i = 0; i < NumberOfChunks / 2; ++i)
    {
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }


    for (uint32_t i = 0; i < NumberOfChunks / 2; ++i)
    {
        EXPECT_THAT(sut.getMinFree(), Eq(NumberOfChunks / 2 - i));
        chunks.push_back(reinterpret_cast<uint8_t*>(sut.getChunk()));
    }
}

TEST_F(MemPool_test, dieWhenMempoolChunkSizeIsSmallerThan32Bytes)
{
    EXPECT_DEATH({ iox::mepoo::MemPool sut(12, 10, allocator, allocator); }, ".*");
}

TEST_F(MemPool_test, dieWhenMempoolChunkSizeIsNotPowerOf32)
{
    EXPECT_DEATH({ iox::mepoo::MemPool sut(333, 10, allocator, allocator); }, ".*");
}
