// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "test.hpp"

#include "iceoryx_utils/internal/concurrent/lockfree_queue/cyclic_index.hpp"
namespace
{
using namespace ::testing;

using iox::concurrent::CyclicIndex;

template <typename T>
class LockFreeQueueCyclicIndexTest : public ::testing::Test
{
  public:
    using Index = T;

  protected:
    LockFreeQueueCyclicIndexTest()
    {
    }

    ~LockFreeQueueCyclicIndexTest()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

typedef ::testing::Types<CyclicIndex<1>, CyclicIndex<2>, CyclicIndex<10>, CyclicIndex<1000>> TestIndices;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(LockFreeQueueCyclicIndexTest, TestIndices);
#pragma GCC diagnostic pop

// note that in all tests we will check whether the getCycle and getIndex methods
// behave as expected after certain operations (mainly addition),
// ensuring a proper cyclic behavior (modulo CycleLength)
// overflow cases are tested as well


TYPED_TEST(LockFreeQueueCyclicIndexTest, defaultConstructedIndexIsZero)
{
    using Index = typename TestFixture::Index;
    Index index;

    EXPECT_EQ(index.getIndex(), 0);
    EXPECT_EQ(index.getCycle(), 0);
    EXPECT_EQ(index.getValue(), 0);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, explicitIndexConstructionWithZeroWorks)
{
    using Index = typename TestFixture::Index;

    {
        Index index(0, 0);
        EXPECT_EQ(index.getIndex(), 0);
        EXPECT_EQ(index.getCycle(), 0);
        EXPECT_EQ(index.getValue(), 0);
    }

    {
        Index index(0);
        EXPECT_EQ(index.getIndex(), 0);
        EXPECT_EQ(index.getCycle(), 0);
        EXPECT_EQ(index.getValue(), 0);
    }
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, getValueReturnsValueIndexWasConstructedWith)
{
    using Index = typename TestFixture::Index;
    Index index(73);
    EXPECT_EQ(index.getValue(), 73);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, explicitConstructionWorks)
{
    using Index = typename TestFixture::Index;

    // something inbetween max and min
    const auto v = Index::MAX_VALUE / 2;
    const auto m = Index::MAX_INDEX + 1;
    const auto i = v % m;
    const auto c = v / m;

    {
        Index index(i, c);
        EXPECT_EQ(index.getIndex(), i);
        EXPECT_EQ(index.getCycle(), c);
    }

    // check that cycle and index are consistent if constructed by the value v itself
    {
        Index index(v);
        EXPECT_EQ(index.getIndex(), i);
        EXPECT_EQ(index.getCycle(), c);
    }
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, explicitConstructionWithMaxIndexAndCycleWorks)
{
    using Index = typename TestFixture::Index;
    const auto i = Index::INDEX_AT_MAX_VALUE;
    const auto c = Index::MAX_CYCLE;
    const auto v = Index::MAX_VALUE;

    {
        Index index(i, c);
        EXPECT_EQ(index.getIndex(), i);
        EXPECT_EQ(index.getCycle(), c);
    }

    // check that cycle and index are consistent if constructed by the value v itself
    {
        Index index(v);
        EXPECT_EQ(index.getIndex(), i);
        EXPECT_EQ(index.getCycle(), c);
    }
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, copyConstructorWorks)
{
    using Index = typename TestFixture::Index;

    // something inbetween max and min
    const auto i = Index::MAX_INDEX / 2;
    const auto c = Index::MAX_CYCLE / 2;

    Index index(i, c);
    EXPECT_EQ(index.getIndex(), i);
    EXPECT_EQ(index.getCycle(), c);

    Index indexCopy(index);
    EXPECT_EQ(indexCopy.getIndex(), i);
    EXPECT_EQ(indexCopy.getCycle(), c);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, assignmentWorks)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX / 2;
    const auto c = Index::MAX_CYCLE / 2;

    Index index1(i, c);
    Index index2;

    index2 = index1;
    EXPECT_EQ(index2.getIndex(), i);
    EXPECT_EQ(index2.getCycle(), c);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, selfAssignmentWorks)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX / 2;
    const auto c = Index::MAX_CYCLE / 2;

    Index index(i, c);
    // this construct is used to prevent a self-assign warning
    [](Index& a, Index& b) { a = b; }(index, index);

    EXPECT_EQ(index.getIndex(), i);
    EXPECT_EQ(index.getCycle(), c);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, cyclicAdditionWorks)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX / 2;
    const auto c = Index::MAX_CYCLE - 1;
    const auto m = Index::MAX_INDEX + 1;
    const auto inc = Index::MAX_INDEX + 2;

    const auto expectedIndex = (i + inc) % m;
    const auto expectedCycle = (i + inc) / m + c;

    Index index(i, c);
    Index result = index + inc;

    EXPECT_EQ(result.getIndex(), expectedIndex);
    EXPECT_EQ(result.getCycle(), expectedCycle);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, cyclicIncrementWorks)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX / 2;
    const auto c = Index::MAX_CYCLE - 1;
    const auto m = Index::MAX_INDEX + 1;

    const auto expectedIndex = (i + 1) % m;
    const auto expectedCycle = (i + 1) / m + c;

    Index index(i, c);
    Index next = index.next();

    EXPECT_EQ(next.getIndex(), expectedIndex);
    EXPECT_EQ(next.getCycle(), expectedCycle);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, cyclicIncrementWraparound)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX;
    const auto c = Index::MAX_CYCLE - 1;

    const auto expectedIndex = 0;
    const auto expectedCycle = c + 1;

    Index index(i, c);
    Index next = index.next();

    EXPECT_EQ(next.getIndex(), expectedIndex);
    EXPECT_EQ(next.getCycle(), expectedCycle);

    // consistency check with operator +
    next = index + 1;
    EXPECT_EQ(next.getIndex(), expectedIndex);
    EXPECT_EQ(next.getCycle(), expectedCycle);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, cyclicIncrementOverflow)
{
    using Index = typename TestFixture::Index;

    const auto v = Index::MAX_VALUE;
    const auto i = Index::INDEX_AT_MAX_VALUE;
    const auto c = Index::MAX_CYCLE;

    const auto expectedIndex = Index::OVERFLOW_START_INDEX;
    const auto expectedCycle = 0;

    Index index(v);
    EXPECT_EQ(index.getIndex(), i);
    EXPECT_EQ(index.getCycle(), c);

    Index next = index.next();
    EXPECT_EQ(next.getIndex(), expectedIndex);
    EXPECT_EQ(next.getCycle(), expectedCycle);

    next = index + 1;
    EXPECT_EQ(next.getIndex(), expectedIndex);
    EXPECT_EQ(next.getCycle(), expectedCycle);
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, cyclicAdditionOverflow)
{
    using Index = typename TestFixture::Index;

    const auto v = Index::MAX_VALUE;
    const auto m = Index::MAX_INDEX + 1;

    // general case: overflow more than one cycle, care must be taken for m = 1
    const auto deltaToMax = 1 + m;

    // add deltaToMax to reach Max, + 1 to cause overflow wraparound to 1, + (1+m) to overflow more
    // than one cycle
    const auto inc = deltaToMax + 2 + m;
    const auto expectedIndex = (Index::OVERFLOW_START_INDEX + 1) % m;
    const auto expectedCycle = (1 + m) / m; // this is 1 except for m = 2 where it is 2

    Index index(v - deltaToMax);
    Index result = index + inc;
    EXPECT_EQ(result.getIndex(), expectedIndex);
    EXPECT_EQ(result.getCycle(), expectedCycle);
}


TYPED_TEST(LockFreeQueueCyclicIndexTest, isOneCycleBehindCheckNegative)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX;
    const auto c = Index::MAX_CYCLE / 2;
    const auto m = Index::MAX_INDEX + 1;

    Index cycleStart(0, c);
    Index cycleMid(m / 2, c);
    Index cycleEnd(i, c);

    Index nextCycleEnd = cycleEnd + m;

    Index nextCycleStart2 = nextCycleEnd.next();

    // note: we do not iterate over all possible cases because depending on cyclelength
    // there could be too many (test parameterization)

    EXPECT_FALSE(cycleStart.isOneCycleBehind(cycleStart));
    EXPECT_FALSE(cycleEnd.isOneCycleBehind(cycleEnd));
    EXPECT_FALSE(cycleMid.isOneCycleBehind(cycleMid));

    EXPECT_FALSE(cycleMid.isOneCycleBehind(cycleEnd));
    EXPECT_FALSE(cycleEnd.isOneCycleBehind(cycleMid));

    EXPECT_FALSE(cycleStart.isOneCycleBehind(cycleEnd));
    EXPECT_FALSE(cycleEnd.isOneCycleBehind(cycleStart));

    EXPECT_FALSE(cycleEnd.isOneCycleBehind(nextCycleStart2));
    EXPECT_FALSE(nextCycleStart2.isOneCycleBehind(cycleEnd));

    EXPECT_FALSE(cycleMid.isOneCycleBehind(nextCycleStart2));
    EXPECT_FALSE(nextCycleStart2.isOneCycleBehind(cycleMid));

    EXPECT_FALSE(cycleStart.isOneCycleBehind(nextCycleStart2));
    EXPECT_FALSE(nextCycleStart2.isOneCycleBehind(cycleStart));
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, isOneCycleBehindCheckPositive)
{
    using Index = typename TestFixture::Index;

    const auto i = Index::MAX_INDEX;
    const auto c = Index::MAX_CYCLE / 2;
    const auto m = Index::MAX_INDEX + 1;

    Index cycleStart(0, c);
    Index cycleMid(m / 2, c);
    Index cycleEnd(i, c);

    Index nextCycleStart = cycleStart + m;
    Index nextCycleMid = cycleMid + m;
    Index nextCycleEnd = cycleEnd + m;

    EXPECT_TRUE(cycleStart.isOneCycleBehind(nextCycleStart));
    EXPECT_TRUE(cycleStart.isOneCycleBehind(nextCycleMid));
    EXPECT_TRUE(cycleStart.isOneCycleBehind(nextCycleEnd));

    EXPECT_TRUE(cycleMid.isOneCycleBehind(nextCycleStart));
    EXPECT_TRUE(cycleMid.isOneCycleBehind(nextCycleMid));
    EXPECT_TRUE(cycleMid.isOneCycleBehind(nextCycleEnd));

    EXPECT_TRUE(cycleEnd.isOneCycleBehind(nextCycleStart));
    EXPECT_TRUE(cycleEnd.isOneCycleBehind(nextCycleMid));
    EXPECT_TRUE(cycleMid.isOneCycleBehind(nextCycleEnd));
}

TYPED_TEST(LockFreeQueueCyclicIndexTest, isOneCycleBehindCheckDuringOverflow)
{
    using Index = typename TestFixture::Index;

    // last cycle before overflow
    Index lastCycleStart(0, Index::MAX_CYCLE);
    Index lastIndexBeforeOverflow(Index::MAX_VALUE);

    // cycle after overflow, will not necessarily start with index 0
    Index firstIndexAfterOverflow(Index::OVERFLOW_START_INDEX, 0);
    Index firstCycleEnd(Index::MAX_INDEX, 0);

    // second cycle (after overflow)
    Index secondCycleStart = firstCycleEnd.next();

    EXPECT_FALSE(lastCycleStart.isOneCycleBehind(secondCycleStart));
    EXPECT_FALSE(secondCycleStart.isOneCycleBehind(lastCycleStart));

    EXPECT_FALSE(lastIndexBeforeOverflow.isOneCycleBehind(secondCycleStart));
    EXPECT_FALSE(secondCycleStart.isOneCycleBehind(lastIndexBeforeOverflow));

    EXPECT_TRUE(lastCycleStart.isOneCycleBehind(firstIndexAfterOverflow));
    EXPECT_TRUE(lastCycleStart.isOneCycleBehind(firstCycleEnd));

    EXPECT_TRUE(lastIndexBeforeOverflow.isOneCycleBehind(firstIndexAfterOverflow));
    EXPECT_TRUE(lastIndexBeforeOverflow.isOneCycleBehind(firstCycleEnd));
}


} // namespace
