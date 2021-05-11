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

#include "test.hpp"

#include "iceoryx_utils/internal/concurrent/lockfree_queue/buffer.hpp"

namespace
{
using namespace ::testing;

using iox::concurrent::Buffer;

template <typename T>
class LockFreeQueueBufferTest : public ::testing::Test
{
  public:
    using Buffer = T;

  protected:
    LockFreeQueueBufferTest()
    {
    }

    ~LockFreeQueueBufferTest()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    Buffer buffer;

    void fillBuffer(int startValue)
    {
        auto capacity = buffer.capacity();

        int value = startValue;
        for (uint64_t i = 0; i < capacity; ++i)
        {
            buffer[i] = value++;
        }
    }
};

struct Integer
{
    Integer(int value = 0)
        : value(value)
    {
    }

    int value{0};

    // so that it behaves like an int for comparison purposes
    operator int() const
    {
        return value;
    }
};

TEST(LockFreeQueueBufferTest, capacityIsCorrect)
{
    constexpr uint64_t capacity = 7;
    Buffer<int, capacity> buffer;
    EXPECT_EQ(buffer.capacity(), capacity);
}

typedef ::testing::Types<Buffer<int, 10>, Buffer<Integer, 10>> TestBuffers;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(LockFreeQueueBufferTest, TestBuffers);
#pragma GCC diagnostic pop

TYPED_TEST(LockFreeQueueBufferTest, accessElements)
{
    auto& buffer = this->buffer;
    auto capacity = buffer.capacity();

    int startValue = 73;
    this->fillBuffer(startValue);

    int expected = startValue;
    for (uint64_t i = 0; i < capacity; ++i)
    {
        EXPECT_EQ(buffer[i], expected++);
    }
}

TYPED_TEST(LockFreeQueueBufferTest, accessElementsOfConstBuffer)
{
    auto& buffer = this->buffer;
    auto capacity = buffer.capacity();

    int startValue = 37;
    this->fillBuffer(startValue);

    const auto& constBuffer = this->buffer;
    int expected = startValue;
    for (uint64_t i = 0; i < capacity; ++i)
    {
        EXPECT_EQ(constBuffer[i], expected++);
    }
}

TYPED_TEST(LockFreeQueueBufferTest, accessElementsViaPtr)
{
    auto& buffer = this->buffer;
    auto capacity = buffer.capacity();

    int startValue = 21;
    this->fillBuffer(startValue);

    int expected = startValue;
    for (uint64_t i = 0; i < capacity; ++i)
    {
        auto ptr = buffer.ptr(i);
        EXPECT_EQ(*ptr, expected++);
    }
}

TYPED_TEST(LockFreeQueueBufferTest, accessElementsOfConstBufferViaPtr)
{
    auto& buffer = this->buffer;
    auto capacity = buffer.capacity();

    int startValue = 12;
    this->fillBuffer(startValue);

    const auto& constBuffer = this->buffer;
    int expected = startValue;
    for (uint64_t i = 0; i < capacity; ++i)
    {
        auto ptr = constBuffer.ptr(i);
        EXPECT_EQ(*ptr, expected++);
    }
}

} // namespace
