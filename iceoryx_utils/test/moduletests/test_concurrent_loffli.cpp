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
#include "test.hpp"

#include <algorithm>
#include <random>
#include <vector>

namespace
{
using namespace ::testing;

constexpr uint32_t Size{4};
using LoFFLiTestSubjects = Types<iox::concurrent::LoFFLi>;
/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(LoFFLi_test, LoFFLiTestSubjects);
#pragma GCC diagnostic pop

template <typename LoFFLiType>
class LoFFLi_test : public Test
{
  public:
    void SetUp() override
    {
        m_loffli.init(m_memoryLoFFLi, Size);
    }

    void TearDown() override
    {
    }

    using LoFFLiIndex_t = typename LoFFLiType::Index_t;
    LoFFLiIndex_t m_memoryLoFFLi[LoFFLiType::requiredIndexMemorySize(Size)];
    LoFFLiType m_loffli;
};

TYPED_TEST(LoFFLi_test, Misuse_NullptrMemory)
{
    decltype(this->m_loffli) loFFLi;
    EXPECT_DEATH(loFFLi.init(nullptr, 1), ".*");
}

TYPED_TEST(LoFFLi_test, Misuse_ZeroSize)
{
    uint32_t memoryLoFFLi[4];
    decltype(this->m_loffli) loFFLi;
    EXPECT_DEATH(loFFLi.init(memoryLoFFLi, 0), ".*");
}

TYPED_TEST(LoFFLi_test, Misuse_SizeToLarge)
{
    uint32_t memoryLoFFLi[4];
    decltype(this->m_loffli) loFFLi;
    EXPECT_DEATH(loFFLi.init(memoryLoFFLi, UINT32_MAX - 1), ".*");
}


TYPED_TEST(LoFFLi_test, Initialized)
{
    // loffli must be full when initialized
    EXPECT_THAT(this->m_loffli.push(0), Eq(false));
}

TYPED_TEST(LoFFLi_test, SinglePop)
{
    constexpr uint32_t AFFE = 0xAFFE;
    uint32_t index = AFFE;
    EXPECT_THAT(this->m_loffli.pop(index), Eq(true));
    EXPECT_THAT(index, Ne(AFFE));
}

TYPED_TEST(LoFFLi_test, PopEmpty)
{
    constexpr uint32_t AFFE = 0xAFFE;
    for (uint32_t i = 0; i < Size; i++)
    {
        uint32_t index = AFFE;
        EXPECT_THAT(this->m_loffli.pop(index), Eq(true));
        EXPECT_THAT(index, Eq(i));
    }

    uint32_t index = AFFE;
    EXPECT_THAT(this->m_loffli.pop(index), Eq(false));
    EXPECT_THAT(index, Eq(AFFE));
}

TYPED_TEST(LoFFLi_test, PopFromUninitializedLoFFLi)
{
    constexpr uint32_t AFFE = 0xAFFE;
    uint32_t index = AFFE;

    decltype(this->m_loffli) loFFLi;
    EXPECT_THAT(loFFLi.pop(index), Eq(false));
}

TYPED_TEST(LoFFLi_test, SinglePush)
{
    constexpr uint32_t AFFE = 0xAFFE;
    uint32_t index;
    this->m_loffli.pop(index);

    uint32_t indexPush = index;
    EXPECT_THAT(this->m_loffli.push(indexPush), Eq(true));
    index = AFFE;
    EXPECT_THAT(this->m_loffli.pop(index), Eq(true));
    EXPECT_THAT(index, Eq(indexPush));
}

TYPED_TEST(LoFFLi_test, PushTillFull)
{
    std::vector<uint32_t> useList;
    uint32_t index;
    while (this->m_loffli.pop(index))
    {
        useList.push_back(index);
    }

    for (const auto& item : useList)
    {
        EXPECT_THAT(this->m_loffli.push(item), Eq(true));
    }
}

TYPED_TEST(LoFFLi_test, PushRandomOrder)
{
    std::vector<uint32_t> useListToPush;
    std::vector<uint32_t> useListPoped;
    uint32_t index;
    while (this->m_loffli.pop(index))
    {
        useListToPush.push_back(index);
    }

    std::random_device randomDevice;
    std::default_random_engine randomEngine(randomDevice());

    std::shuffle(useListToPush.begin(), useListToPush.end(), randomEngine);

    for (const auto& item : useListToPush)
    {
        EXPECT_THAT(this->m_loffli.push(item), Eq(true));
    }

    while (this->m_loffli.pop(index))
    {
        useListPoped.push_back(index);
    }

    std::sort(useListToPush.begin(), useListToPush.end());
    std::sort(useListPoped.begin(), useListPoped.end());

    EXPECT_THAT(useListPoped, Eq(useListToPush));
}

TYPED_TEST(LoFFLi_test, PushWrongIndex)
{
    uint32_t index;
    this->m_loffli.pop(index);

    uint32_t indexPush = index + 1;
    EXPECT_THAT(this->m_loffli.push(indexPush), Eq(false));
}

TYPED_TEST(LoFFLi_test, PushOutOfBoundIndex)
{
    uint32_t index;
    this->m_loffli.pop(index);

    EXPECT_THAT(this->m_loffli.push(Size), Eq(false));
    EXPECT_THAT(this->m_loffli.push(Size + 42), Eq(false));
}

TYPED_TEST(LoFFLi_test, PushWhenFull)
{
    uint32_t indexPush = 0;
    EXPECT_THAT(this->m_loffli.push(indexPush), Eq(false));
}

TYPED_TEST(LoFFLi_test, PushToUninitializedLoFFLi)
{
    decltype(this->m_loffli) loFFLi;
    EXPECT_THAT(loFFLi.push(0), Eq(false));
}
} // namespace
