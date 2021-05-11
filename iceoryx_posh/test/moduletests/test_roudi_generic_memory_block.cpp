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

#include "iceoryx_posh/roudi/memory/generic_memory_block.hpp"

#include "mocks/roudi_memory_provider_mock.hpp"

#include "test.hpp"

using namespace ::testing;

using namespace iox::roudi;

class GenericMemoryBlock_POD_Test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    using PodType = uint32_t;
    GenericMemoryBlock<PodType> sutPOD;
    MemoryProviderTestImpl memoryProvider;
};

TEST_F(GenericMemoryBlock_POD_Test, Initial)
{
    EXPECT_THAT(sutPOD.value().has_value(), Eq(false));
}

TEST_F(GenericMemoryBlock_POD_Test, Size)
{
    EXPECT_THAT(sutPOD.size(), Eq(sizeof(PodType)));
}

TEST_F(GenericMemoryBlock_POD_Test, Alignment)
{
    EXPECT_THAT(sutPOD.size(), Eq(alignof(PodType)));
}

TEST_F(GenericMemoryBlock_POD_Test, EmplaceWithoutCreate)
{
    constexpr uint32_t EXPECTED_VALUE{37};
    EXPECT_THAT(sutPOD.emplace(EXPECTED_VALUE).has_value(), Eq(false));
}

TEST_F(GenericMemoryBlock_POD_Test, EmplaceValue)
{
    constexpr uint32_t EXPECTED_VALUE{42};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sutPOD));
    IOX_DISCARD_RESULT(memoryProvider.create());
    ASSERT_THAT(sutPOD.memory().has_value(), Eq(true));

    auto emplaceResult = sutPOD.emplace(EXPECTED_VALUE);
    ASSERT_THAT(emplaceResult.has_value(), Eq(true));
    EXPECT_THAT(*emplaceResult.value(), Eq(EXPECTED_VALUE));
}

TEST_F(GenericMemoryBlock_POD_Test, MultipleEmplaceValue)
{
    constexpr uint32_t FIRST_VALUE{13};
    constexpr uint32_t EXPECTED_VALUE{73};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sutPOD));
    IOX_DISCARD_RESULT(memoryProvider.create());

    sutPOD.emplace(FIRST_VALUE);

    auto emplaceResult = sutPOD.emplace(EXPECTED_VALUE);
    ASSERT_THAT(emplaceResult.has_value(), Eq(true));
    EXPECT_THAT(*emplaceResult.value(), Eq(EXPECTED_VALUE));
}

TEST_F(GenericMemoryBlock_POD_Test, GetValue)
{
    constexpr uint32_t EXPECTED_VALUE{42};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sutPOD));
    IOX_DISCARD_RESULT(memoryProvider.create());

    sutPOD.emplace(EXPECTED_VALUE);

    ASSERT_THAT(sutPOD.value().has_value(), Eq(true));
    EXPECT_THAT(*(*sutPOD.value()), Eq(EXPECTED_VALUE));
}

class NonTrivialClass
{
  public:
    NonTrivialClass(uint32_t data)
        : m_data(data)
    {
        ++s_constructorCounter;
    }

    ~NonTrivialClass()
    {
        ++s_destructorCounter;
    }

    uint32_t m_data{0};

    static uint64_t s_constructorCounter;
    static uint64_t s_destructorCounter;

    static void resetCounter()
    {
        s_constructorCounter = 0;
        s_destructorCounter = 0;
    }
};

uint64_t NonTrivialClass::s_constructorCounter = 0;
uint64_t NonTrivialClass::s_destructorCounter = 0;

class GenericMemoryBlock_NonTrivial_Test : public Test
{
  public:
    void SetUp() override
    {
        NonTrivialClass::resetCounter();
        EXPECT_THAT(NonTrivialClass::s_constructorCounter, Eq(0u));
        EXPECT_THAT(NonTrivialClass::s_destructorCounter, Eq(0u));
    }

    void TearDown() override
    {
    }

    GenericMemoryBlock<NonTrivialClass> sut;
    MemoryProviderTestImpl memoryProvider;
};

TEST_F(GenericMemoryBlock_NonTrivial_Test, EmplaceValue)
{
    constexpr uint32_t EXPECTED_VALUE{142};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());
    ASSERT_THAT(sut.memory().has_value(), Eq(true));

    auto emplaceResult = sut.emplace(EXPECTED_VALUE);
    EXPECT_THAT(NonTrivialClass::s_constructorCounter, Eq(1u));
    ASSERT_THAT(emplaceResult.has_value(), Eq(true));
    EXPECT_THAT(emplaceResult.value()->m_data, Eq(EXPECTED_VALUE));
}

TEST_F(GenericMemoryBlock_NonTrivial_Test, MultipleEmplaceValue)
{
    constexpr uint32_t FIRST_VALUE{113};
    constexpr uint32_t EXPECTED_VALUE{173};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());

    sut.emplace(FIRST_VALUE);

    auto emplaceResult = sut.emplace(EXPECTED_VALUE);
    EXPECT_THAT(NonTrivialClass::s_constructorCounter, Eq(2u));
    ASSERT_THAT(emplaceResult.has_value(), Eq(true));
    EXPECT_THAT(emplaceResult.value()->m_data, Eq(EXPECTED_VALUE));
}

TEST_F(GenericMemoryBlock_NonTrivial_Test, DestroyWithoutCreate)
{
    sut.destroy();
    /// @note we just expect to not terminate
}

TEST_F(GenericMemoryBlock_NonTrivial_Test, DestroyWithoutEmplace)
{
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());
    sut.destroy();
    /// @note we just expect to not terminate
}

TEST_F(GenericMemoryBlock_NonTrivial_Test, DestroyWithEmplace)
{
    constexpr uint32_t EXPECTED_VALUE{111};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());
    EXPECT_THAT(sut.emplace(EXPECTED_VALUE).value()->m_data, EXPECTED_VALUE);
    EXPECT_THAT(NonTrivialClass::s_constructorCounter, Eq(1u));

    sut.destroy();

    EXPECT_THAT(sut.value().has_value(), Eq(false));
    EXPECT_THAT(NonTrivialClass::s_destructorCounter, Eq(1u));
}

TEST_F(GenericMemoryBlock_NonTrivial_Test, RepetitiveDestroyWithEmplace)
{
    constexpr uint32_t EXPECTED_VALUE{42};
    IOX_DISCARD_RESULT(memoryProvider.addMemoryBlock(&sut));
    IOX_DISCARD_RESULT(memoryProvider.create());
    sut.emplace(EXPECTED_VALUE);

    sut.destroy();

    EXPECT_THAT(sut.value().has_value(), Eq(false));

    sut.destroy();
    sut.destroy();

    EXPECT_THAT(NonTrivialClass::s_destructorCounter, Eq(1u));
}
