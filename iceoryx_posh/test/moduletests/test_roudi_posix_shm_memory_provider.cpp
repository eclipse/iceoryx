// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/roudi/memory/posix_shm_memory_provider.hpp"

#include "iceoryx_utils/internal/posix_wrapper/system_configuration.hpp"

#include "mocks/roudi_memory_block_mock.hpp"

#include "test.hpp"

using namespace ::testing;

using namespace iox::roudi;

using iox::ShmName_t;
static const ShmName_t TEST_SHM_NAME = ShmName_t("/FuManchu");

class PosixShmMemoryProvider_Test : public Test
{
  public:
    void SetUp() override
    {
        /// @note just is case a test left something behind, cleanup the shm by creating a new one with "mine" ownership
        IOX_DISCARD_RESULT(iox::posix::SharedMemoryObject::create(TEST_SHM_NAME,
                                                                  1024,
                                                                  iox::posix::AccessMode::READ_WRITE,
                                                                  iox::posix::OwnerShip::MINE,
                                                                  iox::posix::SharedMemoryObject::NO_ADDRESS_HINT));
    }

    void TearDown() override
    {
    }

    bool shmExists()
    {
        return !iox::posix::SharedMemoryObject::create(TEST_SHM_NAME,
                                                       8,
                                                       iox::posix::AccessMode::READ_ONLY,
                                                       iox::posix::OwnerShip::OPEN_EXISTING,
                                                       iox::posix::SharedMemoryObject::NO_ADDRESS_HINT)
                    .has_error();
    }

    MemoryBlockMock memoryBlock1;
    MemoryBlockMock memoryBlock2;
};

TEST_F(PosixShmMemoryProvider_Test, CreateMemory)
{
    PosixShmMemoryProvider sut(TEST_SHM_NAME, iox::posix::AccessMode::READ_WRITE, iox::posix::OwnerShip::MINE);
    ASSERT_FALSE(sut.addMemoryBlock(&memoryBlock1).has_error());
    uint64_t MEMORY_SIZE{16};
    uint64_t MEMORY_ALIGNMENT{8};
    EXPECT_CALL(memoryBlock1, sizeMock()).WillRepeatedly(Return(MEMORY_SIZE));
    EXPECT_CALL(memoryBlock1, alignmentMock()).WillRepeatedly(Return(MEMORY_ALIGNMENT));

    EXPECT_THAT(sut.create().has_error(), Eq(false));

    EXPECT_THAT(shmExists(), Eq(true));

    EXPECT_CALL(memoryBlock1, destroyMock());
}

TEST_F(PosixShmMemoryProvider_Test, DestroyMemory)
{
    PosixShmMemoryProvider sut(TEST_SHM_NAME, iox::posix::AccessMode::READ_WRITE, iox::posix::OwnerShip::MINE);
    ASSERT_FALSE(sut.addMemoryBlock(&memoryBlock1).has_error());
    uint64_t MEMORY_SIZE{16};
    uint64_t MEMORY_ALIGNMENT{8};
    EXPECT_CALL(memoryBlock1, sizeMock()).WillRepeatedly(Return(MEMORY_SIZE));
    EXPECT_CALL(memoryBlock1, alignmentMock()).WillRepeatedly(Return(MEMORY_ALIGNMENT));

    ASSERT_FALSE(sut.create().has_error());

    EXPECT_CALL(memoryBlock1, destroyMock());

    ASSERT_FALSE(sut.destroy().has_error());

    EXPECT_THAT(shmExists(), Eq(false));
}

TEST_F(PosixShmMemoryProvider_Test, CreationFailedWithAlignmentExceedingPageSize)
{
    PosixShmMemoryProvider sut(TEST_SHM_NAME, iox::posix::AccessMode::READ_WRITE, iox::posix::OwnerShip::MINE);
    ASSERT_FALSE(sut.addMemoryBlock(&memoryBlock1).has_error());
    uint64_t MEMORY_SIZE{16};
    uint64_t MEMORY_ALIGNMENT{iox::posix::pageSize().value_or(iox::posix::MaxPageSize) + 8};
    EXPECT_CALL(memoryBlock1, sizeMock()).WillRepeatedly(Return(MEMORY_SIZE));
    EXPECT_CALL(memoryBlock1, alignmentMock()).WillRepeatedly(Return(MEMORY_ALIGNMENT));

    auto expectFailed = sut.create();
    ASSERT_THAT(expectFailed.has_error(), Eq(true));
    ASSERT_THAT(expectFailed.get_error(), Eq(MemoryProviderError::MEMORY_ALIGNMENT_EXCEEDS_PAGE_SIZE));

    EXPECT_THAT(shmExists(), Eq(false));
}
