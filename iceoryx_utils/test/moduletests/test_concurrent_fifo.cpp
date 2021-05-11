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

#include "iceoryx_utils/internal/concurrent/fifo.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdlib.h>

namespace
{
using namespace testing;
using namespace iox::concurrent;

constexpr size_t FIFO_CAPACITY = 10;

class FiFo_Test : public Test
{
  public:
    void SetUp()
    {
    }

    void TearDown()
    {
    }

    FiFo<int, FIFO_CAPACITY> sut;
};

TEST_F(FiFo_Test, SinglePopSinglePush)
{
    EXPECT_THAT(sut.push(25), Eq(true));
    auto result = sut.pop();
    EXPECT_THAT(result.has_value(), Eq(true));
    EXPECT_THAT(result.value(), Eq(25));
}

TEST_F(FiFo_Test, PopFailsWhenEmpty)
{
    auto result = sut.pop();
    EXPECT_THAT(result.has_value(), Eq(false));
}

TEST_F(FiFo_Test, PushFailsWhenFull)
{
    for (size_t k = 0; k < FIFO_CAPACITY; ++k)
    {
        EXPECT_THAT(sut.push(k), Eq(true));
    }
    EXPECT_THAT(sut.push(123), Eq(false));
}

TEST_F(FiFo_Test, IsEmptyWhenPopReturnsNullopt)
{
    for (size_t k = 0; k < FIFO_CAPACITY; ++k)
    {
        EXPECT_THAT(sut.push(k), Eq(true));
    }
    for (size_t k = 0; k < FIFO_CAPACITY; ++k)
    {
        EXPECT_THAT(sut.pop().has_value(), Eq(true));
    }

    EXPECT_THAT(sut.pop().has_value(), Eq(false));
    EXPECT_THAT(sut.empty(), Eq(true));
}

TEST_F(FiFo_Test, OverflowTestWithPushPopAlternation)
{
    for (size_t k = 0; k < 100 * FIFO_CAPACITY; ++k)
    {
        EXPECT_THAT(sut.push(k), Eq(true));
        auto result = sut.pop();
        EXPECT_THAT(result.has_value(), Eq(true));
        EXPECT_THAT(result.value(), Eq(k));
    }
}

TEST_F(FiFo_Test, OverflowFromFullToEmptyRepetition)
{
    size_t m = 0;

    for (size_t repetition = 0; repetition < 10; ++repetition)
    {
        for (size_t k = 0; k < FIFO_CAPACITY; ++k, ++m)
        {
            EXPECT_THAT(sut.push(m), Eq(true));
        }

        for (size_t k = 0; k < FIFO_CAPACITY; ++k)
        {
            auto result = sut.pop();
            EXPECT_THAT(result.has_value(), Eq(true));
            EXPECT_THAT(result.value(), Eq(m - FIFO_CAPACITY + k));
        }
        EXPECT_THAT(sut.empty(), Eq(true));
    }
}
} // namespace
