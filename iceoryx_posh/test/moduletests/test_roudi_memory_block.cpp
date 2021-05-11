// Copyright (c) 2019, 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/roudi/memory/memory_block.hpp"

#include "mocks/roudi_memory_block_mock.hpp"
#include "mocks/roudi_memory_provider_mock.hpp"

#include "test.hpp"

using namespace ::testing;

using namespace iox::roudi;

class MemoryBlock_Test : public Test
{
  public:
    void SetUp() override
    {
        EXPECT_CALL(sut, sizeMock()).WillRepeatedly(Return(MEMORY_SIZE));
        EXPECT_CALL(sut, alignmentMock()).WillRepeatedly(Return(MEMORY_ALIGNMENT));
    }

    void TearDown() override
    {
    }

    static constexpr uint64_t MEMORY_SIZE = 1U;
    static constexpr uint64_t MEMORY_ALIGNMENT = 1U;

    MemoryBlockMock sut;
    MemoryProviderTestImpl memoryProvider;
};

TEST_F(MemoryBlock_Test, Initial)
{
    EXPECT_THAT(sut.memory().has_value(), Eq(false));
}

TEST_F(MemoryBlock_Test, MemoryAvailableAfterCreation)
{
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());
    EXPECT_THAT(memoryProvider.dummyMemory, Ne(nullptr));
    ASSERT_THAT(sut.memory().has_value(), Eq(true));
    EXPECT_THAT(sut.memory().value(), Eq(memoryProvider.dummyMemory));
}
