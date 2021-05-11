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

#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "test.hpp"

namespace
{
using namespace testing;

class Allocator_Test : public Test
{
  public:
    void SetUp() override
    {
        memory = malloc(memorySize);
    }

    void TearDown() override
    {
        free(memory);
    }

    static constexpr uint64_t MEMORY_ALIGNMENT{iox::posix::Allocator::MEMORY_ALIGNMENT};

    void* memory;
    size_t memorySize = 10016;
};

TEST_F(Allocator_Test, allocateOneSmallElement)
{
    iox::posix::Allocator sut(memory, memorySize);
    int* bla = static_cast<int*>(sut.allocate(sizeof(int), MEMORY_ALIGNMENT));
    *bla = 123;
    EXPECT_THAT(*bla, Eq(123));
}

TEST_F(Allocator_Test, allocateEverythingWithSingleElement)
{
    iox::posix::Allocator sut(memory, memorySize);
    int* bla = static_cast<int*>(sut.allocate(memorySize, 1));
    *bla = 123;
    EXPECT_THAT(*bla, Eq(123));
}

TEST_F(Allocator_Test, allocateEverythingWithMultipleElements)
{
    iox::posix::Allocator sut(memory, memorySize);
    for (size_t i = 0; i < memorySize; i += 32)
    {
        size_t* bla = static_cast<size_t*>(sut.allocate(32, 1));
        *bla = i;
        EXPECT_THAT(*bla, Eq(i));
    }
}

TEST_F(Allocator_Test, allocateTooMuchSingleElement)
{
    iox::posix::Allocator sut(memory, memorySize);
    std::set_terminate([]() { std::cout << "", std::abort(); });
    EXPECT_DEATH({ sut.allocate(memorySize + 1, MEMORY_ALIGNMENT); }, ".*");
}

TEST_F(Allocator_Test, allocateTooMuchMultipleElement)
{
    iox::posix::Allocator sut(memory, memorySize);
    for (size_t i = 0; i < memorySize; i += 32)
    {
        sut.allocate(32, 1);
    }

    std::set_terminate([]() { std::cout << "", std::abort(); });
    EXPECT_DEATH({ sut.allocate(1, MEMORY_ALIGNMENT); }, ".*");
}

TEST_F(Allocator_Test, allocateAndAlignment)
{
    iox::posix::Allocator sut(memory, memorySize);
    auto bla = static_cast<uint8_t*>(sut.allocate(5, MEMORY_ALIGNMENT));
    auto bla2 = static_cast<uint8_t*>(sut.allocate(5, MEMORY_ALIGNMENT));
    EXPECT_THAT(bla2 - bla, Eq(8U));
}

TEST_F(Allocator_Test, allocateElementOfSizeZero)
{
    iox::posix::Allocator sut(memory, memorySize);
    EXPECT_DEATH(sut.allocate(0, MEMORY_ALIGNMENT), ".*");
}

TEST_F(Allocator_Test, allocateAfterFinalizeAllocation)
{
    class AllocatorAccess : iox::posix::Allocator
    {
      public:
        AllocatorAccess(void* const f_startAddress, const uint64_t f_length)
            : iox::posix::Allocator(f_startAddress, f_length)
        {
        }
        using iox::posix::Allocator::allocate;
        using iox::posix::Allocator::finalizeAllocation;
    };
    AllocatorAccess sut(memory, memorySize);
    sut.allocate(5, MEMORY_ALIGNMENT);
    sut.finalizeAllocation();

    std::set_terminate([]() { std::cout << "", std::abort(); });
    EXPECT_DEATH({ sut.allocate(5, MEMORY_ALIGNMENT); }, ".*");
}
} // namespace
