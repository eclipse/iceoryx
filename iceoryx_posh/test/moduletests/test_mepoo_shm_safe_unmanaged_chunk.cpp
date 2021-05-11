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

#include "iceoryx_posh/internal/mepoo/shm_safe_unmanaged_chunk.hpp"

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;

using namespace iox::mepoo;

class ShmSafeUnmanagedChunk_test : public Test
{
  public:
    void SetUp() override
    {
        MePooConfig mempoolconf;
        mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS_IN_POOL});
        memoryManager.configureMemoryManager(mempoolconf, m_memoryAllocator, m_memoryAllocator);
    }

    SharedChunk getChunkFromMemoryManager()
    {
        auto chunkSettingsResult = iox::mepoo::ChunkSettings::create(sizeof(bool), alignof(bool));
        EXPECT_FALSE(chunkSettingsResult.has_error());
        if (chunkSettingsResult.has_error())
        {
            return nullptr;
        }
        auto& chunkSettings = chunkSettingsResult.value();

        return memoryManager.getChunk(chunkSettings);
    }

    iox::mepoo::MemoryManager memoryManager;

  private:
    static constexpr size_t KILOBYTE = 1 << 10;
    static constexpr size_t MEMORY_SIZE = 100 * KILOBYTE;
    std::unique_ptr<char[]> m_memory{new char[MEMORY_SIZE]};
    static constexpr uint32_t NUM_CHUNKS_IN_POOL = 100;
    static constexpr uint32_t CHUNK_SIZE = 128;

    iox::posix::Allocator m_memoryAllocator{m_memory.get(), MEMORY_SIZE};
};

TEST_F(ShmSafeUnmanagedChunk_test, DefaultConstructedResultsInLogicalNullptr)
{
    ShmSafeUnmanagedChunk sut;

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST_F(ShmSafeUnmanagedChunk_test, ConstructedWithEmptySharedChunkResultsInLogicalNullptr)
{
    SharedChunk sharedChunk;
    ShmSafeUnmanagedChunk sut(sharedChunk);

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST_F(ShmSafeUnmanagedChunk_test, CallIsLogicalNullptrOnSutConstructedWithSharedChunkResultsNotInLogicalNullptr)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    EXPECT_FALSE(sut.isLogicalNullptr());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test, CallIsLogicalNullptrOnSutPreviouslyCalledReleaseToSharedChunkResultsInLogicalNullptr)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);
    sut.releaseToSharedChunk();

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallIsLogicalNullptrOnSutPreviouslyCalledDuplicateToSharedChunkResultsNotInLogicalNullptr)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);
    sut.cloneToSharedChunk();

    EXPECT_FALSE(sut.isLogicalNullptr());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test, CallReleaseToSharedChunkOnDefaultConstructedSutResultsInEmptySharedChunk)
{
    ShmSafeUnmanagedChunk sut;

    EXPECT_FALSE(sut.releaseToSharedChunk());
}

TEST_F(ShmSafeUnmanagedChunk_test, CallReleaseToSharedChunkOnSutConstructedWithSharedChunkResultsInNotEmptySharedChunk)
{
    ShmSafeUnmanagedChunk sut(getChunkFromMemoryManager());

    EXPECT_EQ(memoryManager.getMemPoolInfo(0).m_usedChunks, 1U);
    EXPECT_TRUE(sut.releaseToSharedChunk());
    EXPECT_EQ(memoryManager.getMemPoolInfo(0).m_usedChunks, 0U);
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallReleaseToSharedChunkTwiceOnSutConstructedWithSharedChunkResultsInEmptySharedChunk)
{
    ShmSafeUnmanagedChunk sut(getChunkFromMemoryManager());
    sut.releaseToSharedChunk();

    EXPECT_FALSE(sut.releaseToSharedChunk());
}

TEST_F(ShmSafeUnmanagedChunk_test, CallDuplicateToSharedChunkOnDefaultConstructedSutResultsEmptySharedChunk)
{
    ShmSafeUnmanagedChunk sut;

    EXPECT_FALSE(sut.cloneToSharedChunk());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallDuplicateToSharedChunkOnSutConstructedWithSharedChunkResultsInNotEmptySharedChunk)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    auto duplicatedSharedChunk = sut.cloneToSharedChunk();
    EXPECT_TRUE(duplicatedSharedChunk);

    sut.releaseToSharedChunk();
    EXPECT_EQ(memoryManager.getMemPoolInfo(0).m_usedChunks, 1U);
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallDuplicateToSharedChunkOnSutPreviouslyCalledReleaseToSharedChunkResultsInEmptySharedChunk)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);
    sut.releaseToSharedChunk();

    EXPECT_FALSE(sut.cloneToSharedChunk());
}

TEST_F(ShmSafeUnmanagedChunk_test, CallGetChunkHeaderOnNonConstDefaultConstructedSutResultsInNullptr)
{
    ShmSafeUnmanagedChunk sut;

    EXPECT_EQ(sut.getChunkHeader(), nullptr);
}

TEST_F(ShmSafeUnmanagedChunk_test, CallGetChunkHeaderOnNonConstSutConstructedWithSharedChunkResultsInValidHeader)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    auto chunkHeader = sut.getChunkHeader();
    EXPECT_NE(chunkHeader, nullptr);
    EXPECT_EQ(chunkHeader, sharedChunk.getChunkHeader());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test, CallGetChunkHeaderOnConstDefaultConstructedSutResultsInNullptr)
{
    const ShmSafeUnmanagedChunk sut;

    EXPECT_EQ(sut.getChunkHeader(), nullptr);
}

TEST_F(ShmSafeUnmanagedChunk_test, CallGetChunkHeaderOnConstSutConstructedWithSharedChunkResultsInValidHeader)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    [&](const ShmSafeUnmanagedChunk& sut) {
        auto chunkHeader = sut.getChunkHeader();
        EXPECT_NE(chunkHeader, nullptr);
        EXPECT_EQ(chunkHeader, sharedChunk.getChunkHeader());
    }(sut);

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test, CallNonConstGetChunkHeaderResultsInNonConstChunkHeader)
{
    auto isNonConstReturn =
        std::is_same<decltype(std::declval<ShmSafeUnmanagedChunk>().getChunkHeader()), ChunkHeader*>::value;
    EXPECT_TRUE(isNonConstReturn);
}

TEST_F(ShmSafeUnmanagedChunk_test, CallConstGetChunkHeaderResultsInConstChunkHeader)
{
    auto isConstReturn =
        std::is_same<decltype(std::declval<const ShmSafeUnmanagedChunk>().getChunkHeader()), const ChunkHeader*>::value;
    EXPECT_TRUE(isConstReturn);
}

TEST_F(ShmSafeUnmanagedChunk_test, CallIsNotLogicalNullptrAndHasNoOtherOwnersOnDefaultConstructedResultsInFalse)
{
    ShmSafeUnmanagedChunk sut;

    EXPECT_FALSE(sut.isNotLogicalNullptrAndHasNoOtherOwners());
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallIsNotLogicalNullptrAndHasNoOtherOwnersOnOnSutConstructedWithSharedChunkResultsInTrue)
{
    ShmSafeUnmanagedChunk sut(getChunkFromMemoryManager());

    EXPECT_TRUE(sut.isNotLogicalNullptrAndHasNoOtherOwners());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallIsNotLogicalNullptrAndHasNoOtherOwnersOnOnSutConstructedWithSharedChunkAndOtherOwnerResultsInFalse)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    EXPECT_FALSE(sut.isNotLogicalNullptrAndHasNoOtherOwners());

    sut.releaseToSharedChunk();
}

TEST_F(
    ShmSafeUnmanagedChunk_test,
    CallIsNotLogicalNullptrAndHasNoOtherOwnersOnOnSutConstructedWithSharedChunkAndOtherOwnerReleasedOwnershipResultsInTrue)
{
    auto sharedChunk = getChunkFromMemoryManager();

    ShmSafeUnmanagedChunk sut(sharedChunk);

    // release ownership by assigning an empty SharedChunk
    sharedChunk = SharedChunk();

    EXPECT_TRUE(sut.isNotLogicalNullptrAndHasNoOtherOwners());

    sut.releaseToSharedChunk();
}

TEST_F(ShmSafeUnmanagedChunk_test,
       CallIsNotLogicalNullptrAndHasNoOtherOwnersOnOnSutPreviouslyCalledReleaseToSharedChunkResultsInFalse)
{
    ShmSafeUnmanagedChunk sut(getChunkFromMemoryManager());
    sut.releaseToSharedChunk();

    EXPECT_FALSE(sut.isNotLogicalNullptrAndHasNoOtherOwners());
}

} // namespace
