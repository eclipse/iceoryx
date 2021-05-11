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

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object.hpp"
#include "test.hpp"

namespace
{
using namespace testing;
using namespace iox;

class SharedMemoryObject_Test : public Test
{
  public:
    void SetUp() override
    {
        internal::CaptureStderr();
    }

    void TearDown() override
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    void PerformDeathTest(const std::function<void()>& deathTest)
    {
        std::set_terminate([]() { std::cout << "", std::abort(); });

        internal::GetCapturedStderr();
        EXPECT_DEATH({ deathTest(); }, ".*");
        internal::CaptureStderr();
    }
};

TEST_F(SharedMemoryObject_Test, CTorWithValidArguments)
{
    auto sut = iox::posix::SharedMemoryObject::create("/validShmMem",
                                                      100,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    EXPECT_THAT(sut.has_error(), Eq(false));
}

TEST_F(SharedMemoryObject_Test, CTorOpenNonExistingSharedMemoryObject)
{
    auto sut = iox::posix::SharedMemoryObject::create("/pummeluff",
                                                      100,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::OPEN_EXISTING,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    EXPECT_THAT(sut.has_error(), Eq(true));
}

TEST_F(SharedMemoryObject_Test, AllocateMemoryInSharedMemoryAndReadIt)
{
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      16,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    int* test = static_cast<int*>(sut->allocate(sizeof(int), 1));
    ASSERT_THAT(test, Ne(nullptr));
    *test = 123;
    EXPECT_THAT(*test, Eq(123));
}

TEST_F(SharedMemoryObject_Test, AllocateWholeSharedMemoryWithOneChunk)
{
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      8,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);

    void* test = sut->allocate(8, 1);
    ASSERT_THAT(test, Ne(nullptr));
}

TEST_F(SharedMemoryObject_Test, AllocateWholeSharedMemoryWithMultipleChunks)
{
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      8,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);

    for (uint64_t i = 0; i < 8; ++i)
    {
        void* test = sut->allocate(1, 1);
        ASSERT_THAT(test, Ne(nullptr));
    }
}

TEST_F(SharedMemoryObject_Test, AllocateTooMuchMemoryInSharedMemoryWithOneChunk)
{
    uint64_t memorySize{8u};
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      memorySize,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);

    PerformDeathTest([&] { sut->allocate(cxx::align(memorySize, posix::Allocator::MEMORY_ALIGNMENT) + 1, 1); });
}

TEST_F(SharedMemoryObject_Test, AllocateTooMuchSharedMemoryWithMultipleChunks)
{
    uint64_t memorySize{8u};
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      memorySize,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);

    for (uint64_t i = 0; i < cxx::align(memorySize, posix::Allocator::MEMORY_ALIGNMENT); ++i)
    {
        void* test = sut->allocate(1, 1);
        ASSERT_THAT(test, Ne(nullptr));
    }

    PerformDeathTest([&] { sut->allocate(1, 1); });
}

TEST_F(SharedMemoryObject_Test, AllocateAfterFinalizeAllocation)
{
    auto sut = iox::posix::SharedMemoryObject::create("/shmAllocate",
                                                      8,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::MINE,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    sut->finalizeAllocation();

    PerformDeathTest([&] { sut->allocate(2, 1); });
}

TEST_F(SharedMemoryObject_Test, OpeningSharedMemoryAndReadMultipleContents)
{
    uint64_t memorySize = 128;
    auto shmMemory = iox::posix::SharedMemoryObject::create("/shmSut",
                                                            memorySize,
                                                            iox::posix::AccessMode::READ_WRITE,
                                                            iox::posix::OwnerShip::MINE,
                                                            iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    int* test = static_cast<int*>(shmMemory->allocate(sizeof(int), 1));
    *test = 4557;
    int* test2 = static_cast<int*>(shmMemory->allocate(sizeof(int), 1));
    *test2 = 8912;

    auto sut = iox::posix::SharedMemoryObject::create("/shmSut",
                                                      memorySize,
                                                      iox::posix::AccessMode::READ_WRITE,
                                                      iox::posix::OwnerShip::OPEN_EXISTING,
                                                      iox::posix::SharedMemoryObject::NO_ADDRESS_HINT);
    int* sutValue1 = static_cast<int*>(sut->allocate(sizeof(int), 1));
    int* sutValue2 = static_cast<int*>(sut->allocate(sizeof(int), 1));

    EXPECT_THAT(*sutValue1, Eq(4557));
    EXPECT_THAT(*sutValue2, Eq(8912));
}
} // namespace
