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

#include "iceoryx_utils/internal/concurrent/taco.hpp"
#include "test.hpp"

#include <iostream>

namespace
{
using namespace ::testing;

struct TestData
{
    TestData() = default;
    constexpr TestData(uint32_t f_index, uint32_t f_counter, uint64_t f_timestamp)
        : index(f_index)
        , counter(f_counter)
        , timestamp(f_timestamp)
    {
    }
    uint32_t index;
    uint32_t counter;
    uint64_t timestamp;

    bool operator==(const TestData& rhs) const
    {
        return index == rhs.index && counter == rhs.counter && timestamp == rhs.timestamp;
    }
};

enum class Context : uint32_t
{
    Huey,
    Dewey,
    Louie,
    END_OF_LIST
};

class TACO_test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    using MyTACO = iox::concurrent::TACO<TestData, Context>;
};

TEST_F(TACO_test, Initialized_SameContextAllowed)
{
    // TACO must be empty when initialized
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    auto retVal = taco.take(Context::Huey);
    EXPECT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, Initialized_SameContextDenied)
{
    // TACO must be empty when initialized
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    auto retVal = taco.take(Context::Huey);
    EXPECT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, StoreAndTake_FromSameContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.take(Context::Huey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, StoreAndTake_FromDifferentContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.take(Context::Dewey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, StoreAndTake_FromSameContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.take(Context::Huey);

    EXPECT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, StoreAndTake_FromDifferentContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.take(Context::Dewey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, MultipleStoresSingleTake_FromSameContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData_1{1, 42, 73};
    constexpr TestData ExpectedData_2{13, 111, 666};

    taco.store(ExpectedData_1, Context::Huey);
    taco.store(ExpectedData_2, Context::Huey);
    auto retVal = taco.take(Context::Huey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData_2));
}

TEST_F(TACO_test, MultipleStoresSingleTake_FromDifferentContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData_1{1, 42, 73};
    constexpr TestData ExpectedData_2{13, 111, 666};

    taco.store(ExpectedData_1, Context::Huey);
    taco.store(ExpectedData_2, Context::Dewey);
    auto retVal = taco.take(Context::Louie);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData_2));
}

TEST_F(TACO_test, MultipleStoresSingleTake_FromSameContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData_1{1, 42, 73};
    constexpr TestData ExpectedData_2{13, 111, 666};

    taco.store(ExpectedData_1, Context::Huey);
    taco.store(ExpectedData_2, Context::Huey);
    auto retVal = taco.take(Context::Huey);

    ASSERT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, MultipleStoresSingleTake_FromDifferentContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData_1{1, 42, 73};
    constexpr TestData ExpectedData_2{13, 111, 666};

    taco.store(ExpectedData_1, Context::Huey);
    taco.store(ExpectedData_2, Context::Dewey);
    auto retVal = taco.take(Context::Louie);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData_2));
}

TEST_F(TACO_test, DoubleTake)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};

    taco.store(ExpectedData, Context::Huey);
    taco.take(Context::Dewey);
    auto retVal = taco.take(Context::Louie);

    EXPECT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, Exchange_FromSameContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};
    constexpr TestData DummyData{37, 4242, 123456};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.exchange(DummyData, Context::Huey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, Exchange_FromDifferentContext_SameContextAllowed)
{
    MyTACO taco(iox::concurrent::TACOMode::AccecptDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};
    constexpr TestData DummyData{37, 4242, 123456};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.exchange(DummyData, Context::Dewey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, Exchange_FromSameContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};
    constexpr TestData DummyData{37, 4242, 123456};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.exchange(DummyData, Context::Huey);

    ASSERT_THAT(retVal.has_value(), Eq(false));
}

TEST_F(TACO_test, Exchange_FromDifferentContext_SameContextDenied)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData{1, 42, 73};
    constexpr TestData DummyData{37, 4242, 123456};

    taco.store(ExpectedData, Context::Huey);
    auto retVal = taco.exchange(DummyData, Context::Dewey);

    ASSERT_THAT(retVal.has_value(), Eq(true));
    EXPECT_THAT(*retVal, Eq(ExpectedData));
}

TEST_F(TACO_test, DoubleExchange)
{
    MyTACO taco(iox::concurrent::TACOMode::DenyDataFromSameContext);
    constexpr TestData ExpectedData_1{1, 42, 73};
    constexpr TestData ExpectedData_2{13, 111, 666};
    constexpr TestData DummyData{37, 4242, 123456};

    taco.store(ExpectedData_1, Context::Huey);
    auto retVal_1 = taco.exchange(ExpectedData_2, Context::Dewey);
    auto retVal_2 = taco.exchange(DummyData, Context::Louie);

    ASSERT_THAT(retVal_1.has_value(), Eq(true));
    EXPECT_THAT(*retVal_1, Eq(ExpectedData_1));

    ASSERT_THAT(retVal_2.has_value(), Eq(true));
    EXPECT_THAT(*retVal_2, Eq(ExpectedData_2));
}
} // namespace
