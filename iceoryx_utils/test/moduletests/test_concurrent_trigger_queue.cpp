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

#include "iceoryx_utils/concurrent/lockfree_queue.hpp"
#include "iceoryx_utils/concurrent/resizeable_lockfree_queue.hpp"
#include "iceoryx_utils/internal/concurrent/fifo.hpp"
#include "iceoryx_utils/internal/concurrent/trigger_queue.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;

using namespace iox;
using namespace iox::concurrent;

template <typename QueueType>
class TriggerQueue_test : public Test
{
  public:
    void SetUp() override
    {
        m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    }

    void TearDown() override
    {
    }

    bool fillQueue(const uint64_t offset = 0U)
    {
        for (uint64_t i = 0U; i < this->m_sut.capacity(); ++i)
        {
            EXPECT_THAT(this->m_sut.push(i + offset), Eq(true));
            EXPECT_THAT(this->m_sut.empty(), Eq(false));
            EXPECT_THAT(this->m_sut.size(), Eq(i + 1U));
        }
        return this->m_sut.size() == this->m_sut.capacity();
    }

    QueueType m_sut;
    Watchdog m_watchdog{units::Duration::fromSeconds(2)};
};

using TriggerQueueTestSubjects = Types<TriggerQueue<uint64_t, 1, FiFo>,
                                       TriggerQueue<uint64_t, 10, FiFo>,
                                       TriggerQueue<uint64_t, 100, FiFo>,
                                       TriggerQueue<uint64_t, 1, LockFreeQueue>,
                                       TriggerQueue<uint64_t, 10, LockFreeQueue>,
                                       TriggerQueue<uint64_t, 100, LockFreeQueue>,
                                       TriggerQueue<uint64_t, 1, ResizeableLockFreeQueue>,
                                       TriggerQueue<uint64_t, 10, ResizeableLockFreeQueue>,
                                       TriggerQueue<uint64_t, 100, ResizeableLockFreeQueue>>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(TriggerQueue_test, TriggerQueueTestSubjects);
#pragma GCC diagnostic pop

TYPED_TEST(TriggerQueue_test, EmptyOnConstruction)
{
    EXPECT_THAT(this->m_sut.empty(), Eq(true));
    EXPECT_THAT(this->m_sut.size(), Eq(0U));
}

TYPED_TEST(TriggerQueue_test, PushOneElement)
{
    EXPECT_THAT(this->m_sut.push(5U), Eq(true));
    EXPECT_THAT(this->m_sut.empty(), Eq(false));
    EXPECT_THAT(this->m_sut.size(), Eq(1U));
}

TYPED_TEST(TriggerQueue_test, PushTillFull)
{
    EXPECT_TRUE(this->fillQueue());
}

TYPED_TEST(TriggerQueue_test, PopOnEmptyReturnsNullopt)
{
    EXPECT_THAT(this->m_sut.pop(), Eq(cxx::nullopt));
}

TYPED_TEST(TriggerQueue_test, PopOneElement)
{
    this->m_sut.push(123U);
    auto result = this->m_sut.pop();
    ASSERT_THAT(result.has_value(), Eq(true));
    EXPECT_THAT(*result, Eq(123U));
}

TYPED_TEST(TriggerQueue_test, PopFullQueue)
{
    constexpr uint64_t OFFSET = 231;
    this->fillQueue(OFFSET);

    for (uint64_t i = 0U; i < this->m_sut.capacity(); ++i)
    {
        auto result = this->m_sut.pop();
        ASSERT_THAT(result.has_value(), Eq(true));
        EXPECT_THAT(*result, Eq(i + OFFSET));
    }
}

TYPED_TEST(TriggerQueue_test, PushBlocksUntilPopWhenFull)
{
    constexpr int64_t TIMEOUT_IN_MS = 100;
    this->fillQueue();

    std::atomic<uint64_t> counter{0U};

    std::thread t([&] {
        this->m_sut.push(0U);
        ++counter;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_THAT(counter.load(), Eq(0));

    auto result = this->m_sut.pop();
    ASSERT_THAT(result.has_value(), Eq(true));
    EXPECT_THAT(*result, Eq(0U));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_THAT(counter.load(), Eq(1));
    t.join();
}

TYPED_TEST(TriggerQueue_test, PushBlocksUntilDestroyWasCalled)
{
    constexpr int64_t TIMEOUT_IN_MS = 100;
    this->fillQueue();

    std::atomic<uint64_t> counter{0U};

    std::thread t([&] {
        this->m_sut.push(1U);
        this->m_sut.push(2U);
        this->m_sut.push(3U);
        this->m_sut.push(4U);
        ++counter;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_THAT(counter.load(), Eq(0));

    this->m_sut.destroy();
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_THAT(counter.load(), Eq(1));
    t.join();
}

TYPED_TEST(TriggerQueue_test, AfterDestroyPushAddsNoElements)
{
    this->m_sut.destroy();
    this->m_sut.push(123U);

    EXPECT_THAT(this->m_sut.size(), Eq(0U));
}

} // namespace
