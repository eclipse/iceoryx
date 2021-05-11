// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/function_ref.hpp"
#include "test.hpp"


namespace
{
using namespace ::testing;
using namespace iox::cxx;

constexpr int freeFuncTestValue = 42 + 42;
constexpr int functorTestValue = 11;
constexpr int memberFuncTestValue = 4273;
constexpr int sameSignatureIntTestValue = 12345;
constexpr int sameSignatureVoidTestValue = 12346;
constexpr int sameSignatureIntIntTestValue = 12347;

int freeFunction()
{
    return freeFuncTestValue;
}

class Functor
{
  public:
    int operator()()
    {
        return functorTestValue;
    }
};

struct ComplexType
{
    char a;
    int b;
    float c;

    bool operator==(const ComplexType& rhs) const
    {
        if (a == rhs.a && b == rhs.b && c == rhs.c)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

ComplexType returnComplexType(ComplexType foo)
{
    return foo;
}

int SameSignature(function_ref<int(int)> callback)
{
    return callback(sameSignatureIntTestValue);
}

int SameSignature(function_ref<int(void)> callback)
{
    return callback();
}

int SameSignature(function_ref<int(int, int)> callback)
{
    return callback(sameSignatureIntIntTestValue, sameSignatureIntIntTestValue);
}

class function_refTest : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    int foobar()
    {
        return memberFuncTestValue;
    }

    uint8_t m_iter{0};
};

using function_refDeathTest = function_refTest;

TEST_F(function_refTest, CreateEmptyIsFalse)
{
    function_ref<void()> sut;
    EXPECT_FALSE(sut);
}

TEST_F(function_refDeathTest, CallEmptyLeadsToTermination)
{
    function_ref<void()> sut;
    EXPECT_DEATH(sut(), ".*");
}

TEST_F(function_refTest, CreateValidByAssignIsTrue)
{
    auto lambda = [] {};
    function_ref<void()> sut;
    sut = lambda;
    EXPECT_TRUE(sut);
}

TEST_F(function_refTest, CallValidByAssignResultEqual)
{
    auto lambda = []() -> int { return 7253; };
    function_ref<int()> sut;
    sut = lambda;
    EXPECT_THAT(sut(), Eq(7253));
}

TEST_F(function_refTest, CallValidByCopyConstructResultEqual)
{
    auto lambda = []() -> int { return 3527; };
    function_ref<int()> sut1{lambda};
    function_ref<int()> sut2(sut1);
    ASSERT_TRUE(sut2);
    EXPECT_THAT(sut2(), Eq(3527));
}

TEST_F(function_refTest, CreateValidByCopyAssignResultEqual)
{
    auto lambda = []() -> int { return 43; };
    function_ref<int()> sut2;
    {
        function_ref<int()> sut1{lambda};
        EXPECT_THAT(sut1(), Eq(43));
        EXPECT_FALSE(sut2);
        sut2 = sut1;
    }
    EXPECT_THAT(sut2(), Eq(43));
}

TEST_F(function_refTest, CreateInvalidByCopyAssignIsFalse)
{
    auto lambda = []() -> int { return 44; };
    function_ref<int()> sut2{lambda};
    EXPECT_THAT(sut2(), Eq(44));
    {
        function_ref<int()> sut1;
        EXPECT_FALSE(sut1);
        sut2 = sut1;
    }
    EXPECT_FALSE(sut2);
}

TEST_F(function_refTest, CreateValidByMoveResultEqual)
{
    auto lambda = []() -> int { return 123; };
    function_ref<int()> sut1{lambda};
    function_ref<int()> sut2{std::move(sut1)};
    ASSERT_TRUE(sut2);
    EXPECT_FALSE(sut1);
    EXPECT_THAT(sut2(), Eq(123));
}

TEST_F(function_refTest, CreateInvalidByMoveIsFalse)
{
    function_ref<void()> sut1;
    function_ref<void()> sut2{std::move(sut1)};
    EXPECT_FALSE(sut2);
}

TEST_F(function_refTest, CreateValidByMoveAssignResultEqual)
{
    auto lambda1 = []() -> int { return 118; };
    auto lambda2 = []() -> int { return 999; };
    function_ref<int()> sut1{lambda1};
    {
        function_ref<int()> sut2{lambda2};
        sut1 = std::move(sut2);
    }
    EXPECT_TRUE(sut1);
    EXPECT_THAT(sut1(), Eq(999));
}

TEST_F(function_refTest, CreateInvalidByMoveAssignIsFalse)
{
    auto lambda1 = [] {};
    function_ref<void()> sut1{lambda1};
    {
        function_ref<void()> sut2;
        sut1 = std::move(sut2);
    }
    EXPECT_FALSE(sut1);
}

TEST_F(function_refTest, CreateValidAndSwapResultEqual)
{
    auto lambda1 = []() -> int { return 42; };
    auto lambda2 = []() -> int { return 73; };
    function_ref<int()> sut1(lambda1);
    function_ref<int()> sut2(lambda2);
    EXPECT_THAT(sut1(), Eq(42));
    EXPECT_THAT(sut2(), Eq(73));
    sut1.swap(sut2);
    EXPECT_THAT(sut1(), Eq(73));
    EXPECT_THAT(sut2(), Eq(42));
}

TEST_F(function_refTest, CreateInvalidAndSwapWithValidResultNotEqual)
{
    auto lambda2 = []() -> int { return 7331; };
    function_ref<int()> sut1;
    function_ref<int()> sut2(lambda2);
    EXPECT_FALSE(sut1);
    EXPECT_THAT(sut2(), Eq(7331));
    sut1.swap(sut2);
    EXPECT_THAT(sut1(), Eq(7331));
    EXPECT_FALSE(sut2);
}

TEST_F(function_refTest, CreateValidWithCapturingLambdaVoidVoidIncremented)
{
    auto lambda = [&] { m_iter++; };
    function_ref<void(void)> sut(lambda);
    sut();
    EXPECT_THAT(m_iter, Eq(1));
}

TEST_F(function_refTest, CreateValidWithLambdaIntVoidResultEqual)
{
    auto lambda = [](void) -> int { return 1337; };
    function_ref<int(void)> sut(lambda);
    EXPECT_THAT(sut(), Eq(1337));
}

TEST_F(function_refTest, CreateValidWithLambdaIntIntIncremented)
{
    auto lambda = [](int var) -> int { return ++var; };
    function_ref<int(int)> sut(lambda);
    EXPECT_THAT(sut(0), Eq(1));
}

TEST_F(function_refTest, CreateValidWithFreeFunctionResultEqual)
{
    function_ref<int()> sut(freeFunction);
    EXPECT_THAT(sut(), Eq(freeFuncTestValue));
}

TEST_F(function_refTest, CreateValidWithComplexTypeResultEqual)
{
    ComplexType fuubar{1, 2, 1.3f};
    function_ref<ComplexType(ComplexType)> sut(returnComplexType);
    EXPECT_THAT(sut(fuubar), Eq(fuubar));
}

TEST_F(function_refTest, CreateValidWithFunctorResultEqual)
{
    Functor foo;
    function_ref<int()> sut(foo);
    EXPECT_THAT(sut(), Eq(functorTestValue));
}

TEST_F(function_refTest, CreateValidWithStdBindResultEqual)
{
    auto callable = std::bind(&function_refTest::foobar, this);
    function_ref<int()> sut(callable);
    EXPECT_THAT(sut(), Eq(memberFuncTestValue));
}

TEST_F(function_refTest, CreateValidWithStdFunctionResultEqual)
{
    std::function<int()> baz;
    baz = []() -> int { return 24; };
    function_ref<int()> sut(baz);
    EXPECT_THAT(sut(), Eq(24));
}

TEST_F(function_refTest, StoreInStdFunctionResultEqual)
{
    auto lambda = []() -> int { return 37; };
    function_ref<int()> moep(lambda);
    // Moves cxx::function_ref into std::function, needs copy c'tor
    std::function<int()> sut(moep);
    EXPECT_THAT(sut(), Eq(37));
}

TEST_F(function_refTest, CallOverloadedFunctionResultsInCallOfInt)
{
    auto value = SameSignature([](int value) -> int { return value; });
    EXPECT_THAT(value, Eq(sameSignatureIntTestValue));
}

TEST_F(function_refTest, CallOverloadedFunctionResultsInCallOfVoid)
{
    auto value = SameSignature([](void) -> int { return sameSignatureVoidTestValue; });
    EXPECT_THAT(value, Eq(sameSignatureVoidTestValue));
}

TEST_F(function_refTest, CallOverloadedFunctionResultsInCallOfIntInt)
{
    auto value = SameSignature([](int value1, int value2 IOX_MAYBE_UNUSED) -> int { return value1; });
    EXPECT_THAT(value, Eq(sameSignatureIntIntTestValue));
}
} // namespace
