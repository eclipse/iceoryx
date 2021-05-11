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

#include "iceoryx_utils/concurrent/resizeable_lockfree_queue.hpp"

/// Test the added functionality of ResizeableLockFreeQueue
/// to change the capacity (setCapacity).
/// The remaining functionality is identical to LockFreeQueue
/// and tested in test_lockfree_queue.cpp (as a typed test).
namespace
{
using namespace ::testing;

// use a non-POD type for testing (just a boxed version of int)
struct Integer
{
    Integer(uint64_t value = 0)
        : value(value)
    {
    }

    uint64_t value{0};

    // so that it behaves like an int for comparison purposes
    operator uint64_t() const
    {
        return value;
    }
};

template <typename T>
class ResizeableLockFreeQueueTest : public ::testing::Test
{
  protected:
    ResizeableLockFreeQueueTest()
    {
    }

    ~ResizeableLockFreeQueueTest()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    void fillQueue(uint64_t start = 0)
    {
        uint64_t element{start};
        for (uint64_t i = 0; i < queue.capacity(); ++i)
        {
            queue.tryPush(element);
            element++;
        }
    }

    using Queue = T;
    Queue queue;
};

template <size_t Capacity>
using IntegerQueue = iox::concurrent::ResizeableLockFreeQueue<Integer, Capacity>;

template <size_t Capacity>
using IntQueue = iox::concurrent::ResizeableLockFreeQueue<uint64_t, Capacity>;

typedef ::testing::Types<IntegerQueue<1>, IntegerQueue<11>, IntQueue<10>> TestQueues;


/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(ResizeableLockFreeQueueTest, TestQueues);
#pragma GCC diagnostic pop

TEST(ResizeableLockFreeQueueTest, maxCapacityIsConsistent)
{
    using Queue = IntegerQueue<37U>;
    EXPECT_EQ(Queue::maxCapacity(), 37U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, initialCapacityIsMaximalbyDefault)
{
    using Queue = typename TestFixture::Queue;
    auto& q = this->queue;
    EXPECT_EQ(q.capacity(), q.maxCapacity());
    EXPECT_EQ(q.capacity(), Queue::maxCapacity());
}

TYPED_TEST(ResizeableLockFreeQueueTest, constructWithMaxCapacity)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    typename TestFixture::Queue q(MAX_CAP);
    EXPECT_EQ(q.capacity(), q.maxCapacity());
}

TYPED_TEST(ResizeableLockFreeQueueTest, constructWithMoreThanMaxCapacitySaturatesAtMaxCapacity)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    typename TestFixture::Queue q(MAX_CAP + 1U);
    EXPECT_EQ(q.capacity(), q.maxCapacity());
}

TYPED_TEST(ResizeableLockFreeQueueTest, constructWithNoCapacity)
{
    typename TestFixture::Queue q(0U);
    EXPECT_EQ(q.capacity(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, constructWithHalfOfMaxCapacity)
{
    constexpr auto cap = TestFixture::Queue::MAX_CAPACITY / 2U;
    typename TestFixture::Queue q(cap);
    EXPECT_EQ(q.capacity(), cap);
}

TYPED_TEST(ResizeableLockFreeQueueTest, decreaseCapacityToZeroOneByOne)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    auto i = MAX_CAP;
    while (i > 0U)
    {
        EXPECT_TRUE(q.setCapacity(--i));
        ASSERT_EQ(q.capacity(), i);
        ASSERT_EQ(q.size(), i);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, decreaseCapacityToZeroOneByOneWithHandler)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    auto& q = this->queue;

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    auto i = MAX_CAP;
    while (i > 0U)
    {
        EXPECT_TRUE(q.setCapacity(--i, removeHandler));
        ASSERT_EQ(q.capacity(), i);
        ASSERT_EQ(q.size(), i);
        EXPECT_EQ(removedElements.size(), MAX_CAP - i);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, increaseToMaxCapacityOneByOne)
{
    typename TestFixture::Queue q(0U);
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    EXPECT_EQ(q.capacity(), 0U);

    for (uint64_t i = 0U; i < MAX_CAP;)
    {
        EXPECT_TRUE(q.setCapacity(++i));
        ASSERT_EQ(q.capacity(), i);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, increaseToMaxCapacityOneByOneWithHandler)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    typename TestFixture::Queue q(0U);
    EXPECT_EQ(q.capacity(), 0U);

    for (uint64_t i = 0U; i < MAX_CAP;)
    {
        EXPECT_TRUE(q.setCapacity(++i, removeHandler));
        ASSERT_EQ(q.capacity(), i);
        EXPECT_EQ(removedElements.size(), 0U);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToZero)
{
    auto& q = this->queue;
    EXPECT_TRUE(q.setCapacity(0U));
    EXPECT_EQ(q.capacity(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToZeroWithHandler)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    auto& q = this->queue;
    EXPECT_TRUE(q.setCapacity(0U, removeHandler));
    EXPECT_EQ(q.capacity(), 0U);
    EXPECT_EQ(removedElements.size(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToOne)
{
    auto& q = this->queue;
    EXPECT_TRUE(q.setCapacity(1U));
    EXPECT_EQ(q.capacity(), 1U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToOneWithHandler)
{
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    auto& q = this->queue;
    EXPECT_TRUE(q.setCapacity(1U, removeHandler));
    EXPECT_EQ(q.capacity(), 1U);
    EXPECT_EQ(removedElements.size(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToMaxCapacity)
{
    typename TestFixture::Queue q(0U);
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    EXPECT_TRUE(q.setCapacity(MAX_CAP));
    EXPECT_EQ(q.capacity(), MAX_CAP);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToMaxCapacityWithHandler)
{
    typename TestFixture::Queue q(0U);
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;

    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    EXPECT_TRUE(q.setCapacity(MAX_CAP, removeHandler));
    EXPECT_EQ(q.capacity(), MAX_CAP);
    EXPECT_EQ(removedElements.size(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToHalfOfMaxCapacityAndFillIt)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;
    EXPECT_EQ(q.setCapacity(NEW_CAP), true);
    EXPECT_EQ(q.capacity(), NEW_CAP);

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    EXPECT_EQ(q.capacity(), NEW_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);
    EXPECT_EQ(element, NEW_CAP);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityToHalfOfMaxCapacityAndFillItWithHandler)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;

    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    EXPECT_EQ(q.setCapacity(NEW_CAP, removeHandler), true);
    EXPECT_EQ(q.capacity(), NEW_CAP);
    EXPECT_EQ(removedElements.size(), 0U);

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    EXPECT_EQ(q.capacity(), NEW_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);
    EXPECT_EQ(element, NEW_CAP);
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityFromHalfOfMaxCapacityToMaxCapacity)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;
    q.setCapacity(NEW_CAP);

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    EXPECT_EQ(q.setCapacity(MAX_CAP), true);
    EXPECT_EQ(q.capacity(), MAX_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);

    while (q.tryPush(element))
    {
        ++element;
    }

    // we want to find all elements we pushed
    for (element = 0U; element < MAX_CAP; ++element)
    {
        const auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityFromHalfOfMaxCapacityToMaxCapacityWithHandler)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;

    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    q.setCapacity(NEW_CAP);

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    }

    EXPECT_EQ(q.setCapacity(MAX_CAP, removeHandler), true);
    EXPECT_EQ(q.capacity(), MAX_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);
    EXPECT_EQ(removedElements.size(), 0U);

    while (q.tryPush(element))
    {
        ++element;
    }

    // we want to find all elements we pushed
    for (element = 0U; element < MAX_CAP; ++element)
    {
        const auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityOfFullQueueToHalfOfMaxCapacity)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    };

    EXPECT_TRUE(q.setCapacity(NEW_CAP));
    EXPECT_EQ(q.capacity(), NEW_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);

    // the least recent values are removed due to the capacity being decreased
    // how man elements remain depends on whether MAX_CAP is divisable by 2
    for (element = NEW_CAP + MAX_CAP % 2U; element < MAX_CAP; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }
}

TYPED_TEST(ResizeableLockFreeQueueTest, setCapacityOfFullQueueToHalfOfMaxCapacityWithHandler)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;
    constexpr auto NEW_CAP = MAX_CAP / 2U;

    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    uint64_t element = 0U;
    while (q.tryPush(element))
    {
        ++element;
    };

    EXPECT_TRUE(q.setCapacity(NEW_CAP, removeHandler));
    EXPECT_EQ(q.capacity(), NEW_CAP);
    EXPECT_EQ(q.size(), NEW_CAP);
    EXPECT_EQ(removedElements.size(), MAX_CAP / 2 + MAX_CAP % 2U);

    // the least recent values are removed due to the capacity being decreased
    // how many elements remain depends on whether MAX_CAP is divisable by 2
    for (element = NEW_CAP + MAX_CAP % 2U; element < MAX_CAP; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }
}

// note this is one of the most general cases and necessary to test:
// decreasing the capacity starting with a partially filled queue and checking whether the last values
// remain (and the others are removed)
TYPED_TEST(ResizeableLockFreeQueueTest, DecreaseCapacityOfAPartiallyFilledQueue)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;

    const auto CAP = MAX_CAP / 2U;

    q.setCapacity(CAP);

    uint64_t element = 0U;
    while (q.tryPush(element++))
        ;

    const auto CAP2 = CAP + MAX_CAP / 4U; // roughly 3 quarters of max (integer division)
    q.setCapacity(CAP2);

    // queue is now partially filled with elements (neither full nor empty)
    // decrease the capacity
    // verify that the test was set up correctly

    EXPECT_EQ(q.capacity(), CAP2);
    EXPECT_EQ(q.size(), CAP);

    // decrease the capacity of the partially filled queue again

    const auto CAP3 = CAP2 - CAP; // roughly a quarter of max

    EXPECT_TRUE(q.setCapacity(CAP3));
    EXPECT_EQ(q.capacity(), CAP3);
    EXPECT_EQ(q.size(), CAP3);

    // are the remaining elements correct? (i.e. we did not remove too many elements)
    for (element = CAP - CAP3; element < CAP; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }

    // refill to verify the capacity can really be used

    element = 0U;
    while (q.tryPush(element++))
        ;

    for (element = 0U; element < CAP3; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }

    EXPECT_EQ(q.size(), 0U);
}

TYPED_TEST(ResizeableLockFreeQueueTest, DecreaseCapacityOfAPartiallyFilledQueueWithHandler)
{
    auto& q = this->queue;
    constexpr auto MAX_CAP = TestFixture::Queue::MAX_CAPACITY;

    using element_t = typename TestFixture::Queue::element_t;
    iox::cxx::vector<element_t, MAX_CAP> removedElements;
    auto removeHandler = [&](const element_t& value) { removedElements.emplace_back(std::move(value)); };

    const auto CAP = MAX_CAP / 2U;

    q.setCapacity(CAP);

    uint64_t element = 0U;
    while (q.tryPush(element++))
        ;

    const auto CAP2 = CAP + MAX_CAP / 4U; // roughly 3 quarters of max (integer division)
    q.setCapacity(CAP2);

    // queue is now partially filled with elements (neither full nor empty)
    // decrease the capacity
    // verify that the test was set up correctly

    EXPECT_EQ(q.capacity(), CAP2);
    EXPECT_EQ(q.size(), CAP);

    // decrease the capacity of the partially filled queue again

    const auto CAP3 = CAP2 - CAP; // roughly a quarter of max

    EXPECT_TRUE(q.setCapacity(CAP3, removeHandler));
    EXPECT_EQ(q.capacity(), CAP3);
    EXPECT_EQ(q.size(), CAP3);

    // CAP3 elements remain, the first CAP - CAP3 elements are removed

    // were the least recent elements removed?
    EXPECT_EQ(removedElements.size(), CAP - CAP3);
    element = 0U;
    for (auto& removedElement : removedElements)
    {
        EXPECT_EQ(removedElement, element++);
    }

    // are the remaining elements correct? (i.e. we did not remove too many elements)
    for (element = CAP - CAP3; element < CAP; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }

    // refill to verify the capacity can really be used

    element = 0U;
    while (q.tryPush(element++))
        ;

    for (element = 0U; element < CAP3; ++element)
    {
        auto result = q.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), element);
    }

    EXPECT_EQ(q.size(), 0U);
}

} // namespace
