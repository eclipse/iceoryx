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

#include "iceoryx_utils/cxx/algorithm.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::algorithm;
using namespace iox::cxx;

class algorithm_test : public Test
{
  public:
    void SetUp()
    {
        testing::internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = testing::internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};

TEST_F(algorithm_test, MaxOfOneElement)
{
    EXPECT_THAT(max(12.34f), Eq(12.34f));
}

TEST_F(algorithm_test, MaxOfTwoElements)
{
    EXPECT_THAT(max(56.78f, 12.34f), Eq(56.78f));
}

TEST_F(algorithm_test, MaxOfManyElements)
{
    EXPECT_THAT(max(56.78f, 33.44f, 12.34f, -0.1f, 5.5f, 10001.f), Eq(10001.f));
}

TEST_F(algorithm_test, MinOfOneElement)
{
    EXPECT_THAT(min(0.0123f), Eq(0.0123f));
}

TEST_F(algorithm_test, MinOfTwoElements)
{
    EXPECT_THAT(min(0.0123f, -91.12f), Eq(-91.12f));
}

TEST_F(algorithm_test, MinOfManyElements)
{
    EXPECT_THAT(min(0.0123f, -91.12f, 123.92f, -1021.2f, 0.0f), Eq(-1021.2f));
}

TEST_F(algorithm_test, MergeTwoDisjunctNonEmptySortedContainers)
{
    constexpr int64_t OFFSET = 1337;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i + OFFSET);
    }

    for (int64_t i = 5; i < 10; ++i)
    {
        b.emplace_back(i + OFFSET);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(10U));
    for (int64_t i = 0; i < 10; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}

TEST_F(algorithm_test, MergeTwoDisjunctNonEmptySortedContainersWithAGap)
{
    constexpr int64_t OFFSET = 41;
    constexpr int64_t GAP = 13;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i + OFFSET);
    }

    for (int64_t i = 5; i < 10; ++i)
    {
        b.emplace_back(i + OFFSET + GAP);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(10U));
    for (int64_t i = 0; i < 5; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    for (int64_t i = 5; i < 10; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET + GAP));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}

TEST_F(algorithm_test, MergeTwoAlternatingDisjunctNonEmptySortedContainers)
{
    constexpr int64_t OFFSET = 4301;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i * 2 + OFFSET);
    }

    for (int64_t i = 0; i < 5; ++i)
    {
        b.emplace_back(i * 2 + 1 + OFFSET);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(10U));
    for (int64_t i = 0; i < 10; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}

TEST_F(algorithm_test, MergingIdenticalContainerResultsInUnchangedContainer)
{
    constexpr int64_t OFFSET = 313;
    vector<int64_t, 10U> a;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i * 2 + OFFSET);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, a);

    ASSERT_THAT(mergedContainer.size(), Eq(5U));
    for (int64_t i = 0; i < 5; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i * 2 + OFFSET));
}

TEST_F(algorithm_test, MergingWithOneEmptyContainerResultsInUnchangedContainer)
{
    constexpr int64_t OFFSET = 707;
    vector<int64_t, 10U> a;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i * 3 + OFFSET);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, vector<int64_t, 10U>());

    ASSERT_THAT(mergedContainer.size(), Eq(5U));
    for (int64_t i = 0; i < 5; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i * 3 + OFFSET));
}

TEST_F(algorithm_test, MergePartiallyOverlappingSortedContainers)
{
    constexpr int64_t OFFSET = 8055;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 3; i < 10; ++i)
    {
        a.emplace_back(i + OFFSET);
    }

    for (int64_t i = 0; i < 8; ++i)
    {
        b.emplace_back(i + OFFSET);
    }

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(10U));
    for (int64_t i = 0; i < 10; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}

TEST_F(algorithm_test, MergeWithDisjunctOneElementContainer)
{
    constexpr int64_t OFFSET = 333331;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i + OFFSET);
    }

    b.emplace_back(5 + OFFSET);

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(6U));
    for (int64_t i = 0; i < 6; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}

TEST_F(algorithm_test, MergeWithOverlappingOneElementContainer)
{
    constexpr int64_t OFFSET = 29292929;
    vector<int64_t, 10U> a, b;

    for (int64_t i = 0; i < 5; ++i)
    {
        a.emplace_back(i + OFFSET);
    }

    b.emplace_back(0 + OFFSET);

    auto mergedContainer = uniqueMergeSortedContainers(a, b);
    auto mergedContainerSwitched = uniqueMergeSortedContainers(b, a);

    ASSERT_THAT(mergedContainer.size(), Eq(5U));
    for (int64_t i = 0; i < 5; ++i)
        EXPECT_THAT(mergedContainer[i], Eq(i + OFFSET));
    EXPECT_TRUE(mergedContainer == mergedContainerSwitched);
}
} // namespace
