// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/internal/popo/used_chunk_list.hpp"

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::mepoo;
using namespace iox::popo;

class UsedChunkList_test : public Test
{
  public:
    void SetUp() override
    {
        static constexpr uint32_t NUM_CHUNKS_IN_POOL = 100U;
        static constexpr uint32_t CHUNK_SIZE = 128U;
        MePooConfig mempoolconf;
        mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS_IN_POOL});

        iox::posix::Allocator memoryAllocator{m_memory.get(), MEMORY_SIZE};
        memoryManager.configureMemoryManager(mempoolconf, memoryAllocator, memoryAllocator);
    };

    void TearDown() override{};

    SharedChunk getChunkFromMemoryManager()
    {
        constexpr uint32_t USER_PAYLOAD_SIZE{32U};
        auto chunkSettingsResult =
            iox::mepoo::ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
        EXPECT_FALSE(chunkSettingsResult.has_error());
        if (chunkSettingsResult.has_error())
        {
            return nullptr;
        }
        auto& chunkSettings = chunkSettingsResult.value();
        return memoryManager.getChunk(chunkSettings);
    }

    void createMultipleChunks(uint32_t numberOfChunks, std::function<void(SharedChunk&&)> testHook)
    {
        ASSERT_TRUE(testHook);
        for (uint32_t i = 0; i < numberOfChunks; ++i)
        {
            testHook(getChunkFromMemoryManager());
        }
    }

    void checkIfEmpty()
    {
        SCOPED_TRACE(std::string("Empty check"));
        for (uint32_t i = 0; i < USED_CHUNK_LIST_CAPACITY; ++i)
        {
            EXPECT_TRUE(sut.insert(getChunkFromMemoryManager()));
        }
    }

    MemoryManager memoryManager;

    static constexpr uint32_t USED_CHUNK_LIST_CAPACITY{10U};
    UsedChunkList<USED_CHUNK_LIST_CAPACITY> sut;

  private:
    static constexpr size_t MEGABYTE = 1U << 20U;
    static constexpr size_t MEMORY_SIZE = 4U * MEGABYTE;
    std::unique_ptr<char[]> m_memory{new char[MEMORY_SIZE]};
};

TEST_F(UsedChunkList_test, OneChunkCanBeAdded)
{
    EXPECT_TRUE(sut.insert(getChunkFromMemoryManager()));
}

TEST_F(UsedChunkList_test, AddSameChunkTwiceWorks)
{
    auto chunk = getChunkFromMemoryManager();
    sut.insert(chunk);

    EXPECT_TRUE(sut.insert(chunk));
}

TEST_F(UsedChunkList_test, MultipleChunksCanBeAdded)
{
    EXPECT_TRUE(sut.insert(getChunkFromMemoryManager()));
    EXPECT_TRUE(sut.insert(getChunkFromMemoryManager()));
    EXPECT_TRUE(sut.insert(getChunkFromMemoryManager()));
}

TEST_F(UsedChunkList_test, AddChunksUpToCapacityWorks)
{
    createMultipleChunks(USED_CHUNK_LIST_CAPACITY, [this](SharedChunk&& chunk) { EXPECT_TRUE(sut.insert(chunk)); });
}

TEST_F(UsedChunkList_test, AddChunksUntilOverflowIsHandledGracefully)
{
    createMultipleChunks(USED_CHUNK_LIST_CAPACITY, [this](SharedChunk&& chunk) { EXPECT_TRUE(sut.insert(chunk)); });

    EXPECT_FALSE(sut.insert(getChunkFromMemoryManager()));
}

TEST_F(UsedChunkList_test, OneChunkCanBeRemoved)
{
    auto chunk = getChunkFromMemoryManager();
    auto chunkHeader = chunk.getChunkHeader();
    sut.insert(chunk);

    SharedChunk removedChunk;
    EXPECT_TRUE(sut.remove(chunkHeader, removedChunk));
    EXPECT_TRUE(removedChunk);

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, RemoveSameChunkAddedTwiceWorks)
{
    auto chunk = getChunkFromMemoryManager();
    auto chunkHeader = chunk.getChunkHeader();
    sut.insert(chunk);
    sut.insert(chunk);

    for (auto i = 0; i < 2; ++i)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeader, removedChunk));
        EXPECT_TRUE(removedChunk);
    }

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, MultipleChunksCanBeRemoved)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(3U, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        sut.insert(chunk);
    });

    for (auto chunkHeader : chunkHeaderInUse)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeader, removedChunk));
        EXPECT_TRUE(removedChunk);
    }

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, MultipleChunksCanBeRemovedInReverseOrder)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(3U, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        sut.insert(chunk);
    });

    constexpr uint32_t removeOrderIndices[]{2U, 1U, 0U};
    for (auto index : removeOrderIndices)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeaderInUse[index], removedChunk));
        EXPECT_TRUE(removedChunk);
    }

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, MultipleChunksCanBeRemovedInArbitraryOrder)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(3U, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        sut.insert(chunk);
    });

    constexpr uint32_t removeOrderIndices[]{0U, 2U, 1U};
    for (auto index : removeOrderIndices)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeaderInUse[index], removedChunk));
        EXPECT_TRUE(removedChunk);
    }

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, UsedChunkListCanBeFilledToCapacityAndFullyEmptied)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(USED_CHUNK_LIST_CAPACITY, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        EXPECT_TRUE(sut.insert(chunk));
    });

    for (auto chunkHeader : chunkHeaderInUse)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeader, removedChunk));
        EXPECT_TRUE(removedChunk);
    }

    checkIfEmpty();
}

TEST_F(UsedChunkList_test, RemoveChunkFromEmptyListIsHandledGracefully)
{
    auto chunk = getChunkFromMemoryManager();
    auto chunkHeader = chunk.getChunkHeader();

    SharedChunk chunkNotInList;
    EXPECT_FALSE(sut.remove(chunkHeader, chunkNotInList));
    EXPECT_FALSE(chunkNotInList);
}

TEST_F(UsedChunkList_test, RemoveChunkNotInListIsHandledGracefully)
{
    createMultipleChunks(3U, [&](SharedChunk&& chunk) { sut.insert(chunk); });

    auto chunk = getChunkFromMemoryManager();
    auto chunkHeader = chunk.getChunkHeader();

    SharedChunk chunkNotInList;
    EXPECT_FALSE(sut.remove(chunkHeader, chunkNotInList));
    EXPECT_FALSE(chunkNotInList);
}

TEST_F(UsedChunkList_test, RemoveChunkNotInListDoesNotRemoveOtherChunk)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(3U, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        sut.insert(chunk);
    });

    auto chunk = getChunkFromMemoryManager();
    auto chunkHeader = chunk.getChunkHeader();
    SharedChunk chunkNotInList;
    sut.remove(chunkHeader, chunkNotInList);

    for (auto chunkHeader : chunkHeaderInUse)
    {
        SharedChunk removedChunk;
        EXPECT_TRUE(sut.remove(chunkHeader, removedChunk));
        EXPECT_TRUE(removedChunk);
    }
}

TEST_F(UsedChunkList_test, ChunksAddedToTheUsedChunkKeepsTheChunkAlive)
{
    EXPECT_THAT(memoryManager.getMemPoolInfo(0U).m_usedChunks, Eq(0U));

    sut.insert(getChunkFromMemoryManager());

    EXPECT_THAT(memoryManager.getMemPoolInfo(0U).m_usedChunks, Eq(1U));
}

TEST_F(UsedChunkList_test, RemovingChunkFromListLetsTheSharedChunkReturnOwnershipToTheMempool)
{
    {
        auto chunk = getChunkFromMemoryManager();
        auto chunkHeader = chunk.getChunkHeader();
        sut.insert(chunk);

        SharedChunk removedChunk;
        sut.remove(chunkHeader, removedChunk);
    }

    EXPECT_THAT(memoryManager.getMemPoolInfo(0U).m_usedChunks, Eq(0U));
}

TEST_F(UsedChunkList_test, CallingCleanupReleasesAllChunks)
{
    std::vector<ChunkHeader*> chunkHeaderInUse;
    createMultipleChunks(USED_CHUNK_LIST_CAPACITY, [&](SharedChunk&& chunk) {
        chunkHeaderInUse.push_back(chunk.getChunkHeader());
        sut.insert(chunk);
    });

    sut.cleanup();

    EXPECT_THAT(memoryManager.getMemPoolInfo(0U).m_usedChunks, Eq(0U));
    checkIfEmpty();
}
} // namespace
