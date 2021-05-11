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

#include "iceoryx_utils/cxx/optional.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;

/// @todo create a typed test with to check if our optional has the same behaviour as the std::optional
/// @todo create a parameterized test with an additional non-POD class

class Optional_test : public Test
{
  public:
    struct TestClass
    {
        int64_t value;
        int64_t secondValue;
    };

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    iox::cxx::optional<int64_t> m_sutWithValue{0};
    iox::cxx::optional<int64_t> m_sutNoValue{iox::cxx::nullopt_t()};
};

TEST_F(Optional_test, DefaultCTorHasValue)
{
    EXPECT_THAT(m_sutWithValue.has_value(), Eq(true));
}

TEST_F(Optional_test, NulloptCTor)
{
    EXPECT_THAT(m_sutNoValue.has_value(), Eq(false));
}

TEST_F(Optional_test, emplaceWithoutPresetValue)
{
    m_sutNoValue.emplace(123);
    EXPECT_THAT(m_sutNoValue.has_value(), Eq(true));
    EXPECT_THAT(m_sutNoValue.value(), Eq(123));
}

TEST_F(Optional_test, emplaceWithPresetValue)
{
    m_sutWithValue.emplace(123);
    EXPECT_THAT(m_sutWithValue.has_value(), Eq(true));
    EXPECT_THAT(m_sutWithValue.value(), Eq(123));
}

TEST_F(Optional_test, value_or_NoValue)
{
    EXPECT_THAT(m_sutNoValue.value_or(1337), Eq(1337));
}

TEST_F(Optional_test, value_or_WithValue)
{
    m_sutWithValue.emplace(42);
    EXPECT_THAT(m_sutWithValue.value_or(1337), Eq(42));
}

TEST_F(Optional_test, value)
{
    m_sutWithValue = 1234;
    EXPECT_THAT(m_sutWithValue.value(), Eq(1234));
}

TEST_F(Optional_test, const_value)
{
    m_sutWithValue = 1234;
    EXPECT_THAT(const_cast<const iox::cxx::optional<int64_t>*>(&m_sutWithValue)->value(), Eq(1234));
}

TEST_F(Optional_test, resetWithValue)
{
    m_sutWithValue.reset();
    EXPECT_THAT(m_sutWithValue.has_value(), Eq(false));
}

TEST_F(Optional_test, resetWithoutValue)
{
    m_sutNoValue.reset();
    EXPECT_THAT(m_sutNoValue.has_value(), Eq(false));
}

TEST_F(Optional_test, boolOperatorNoValue)
{
    EXPECT_THAT(static_cast<bool>(m_sutNoValue), Eq(false));
}

TEST_F(Optional_test, boolOperatorWithValue)
{
    EXPECT_THAT(static_cast<bool>(m_sutWithValue), Eq(true));
}

TEST_F(Optional_test, ArrowOperator)
{
    iox::cxx::optional<TestClass> sut{{0, 0}};
    sut->value = 1234;
    EXPECT_THAT(sut->value, Eq(1234));
}

TEST_F(Optional_test, ConstArrowOperator)
{
    iox::cxx::optional<TestClass> sut{{0, 0}};
    sut->value = 12345;
    EXPECT_THAT((*const_cast<const iox::cxx::optional<TestClass>*>(&sut))->value, Eq(12345));
}

TEST_F(Optional_test, DereferenceOperator)
{
    *m_sutWithValue = 789;
    EXPECT_THAT(*m_sutWithValue, Eq(789));
}

TEST_F(Optional_test, ConstDereferenceOperator)
{
    *m_sutWithValue = 789;
    EXPECT_THAT(**const_cast<const iox::cxx::optional<int64_t>*>(&m_sutWithValue), Eq(789));
}

TEST_F(Optional_test, UserDefinedTypeAssignment)
{
    iox::cxx::optional<TestClass> sut;
    sut = TestClass{1234, 22};
    EXPECT_THAT(sut->value, Eq(1234));
}

TEST_F(Optional_test, CompareWithEqualValue)
{
    m_sutWithValue = 123;
    iox::cxx::optional<int64_t> sut;
    sut = 123;
    EXPECT_THAT(m_sutWithValue == sut, Eq(true));
}

TEST_F(Optional_test, CompareWithEqualNullopt)
{
    iox::cxx::optional<int64_t> sut{iox::cxx::nullopt_t()};
    EXPECT_THAT(m_sutNoValue == sut, Eq(true));
}

TEST_F(Optional_test, CompareWithInequalValue)
{
    m_sutWithValue = 123;
    iox::cxx::optional<int64_t> sut;
    sut = 1231;
    EXPECT_THAT(m_sutWithValue == sut, Eq(false));
}

TEST_F(Optional_test, CompareWithNoValueWithValue)
{
    iox::cxx::optional<int64_t> sut;
    sut = 1231;
    EXPECT_THAT(m_sutNoValue == sut, Eq(false));
}

TEST_F(Optional_test, CompareWithNoValueWithNullopt)
{
    EXPECT_THAT(m_sutNoValue == iox::cxx::nullopt_t(), Eq(true));
}

TEST_F(Optional_test, CompareWithValueWithNullopt)
{
    EXPECT_THAT(m_sutWithValue == iox::cxx::nullopt_t(), Eq(false));
}

TEST_F(Optional_test, NotCompareWithEqualValue)
{
    m_sutWithValue = 123;
    iox::cxx::optional<int64_t> sut;
    sut = 123;
    EXPECT_THAT(m_sutWithValue != sut, Eq(false));
}

TEST_F(Optional_test, NotCompareWithNullopt)
{
    EXPECT_THAT(m_sutWithValue != iox::cxx::nullopt_t(), Eq(true));
}

TEST_F(Optional_test, CopyCTorWithValue)
{
    iox::cxx::optional<TestClass> sut{TestClass{4711, 1337}};

    iox::cxx::optional<TestClass> sut2(sut);

    ASSERT_THAT(sut2.has_value(), Eq(true));
    EXPECT_THAT(sut2->value, Eq(4711));
    EXPECT_THAT(sut2->secondValue, Eq(1337));
}

TEST_F(Optional_test, CopyCTorWithNoValue)
{
    iox::cxx::optional<TestClass> sut = iox::cxx::nullopt_t();
    iox::cxx::optional<TestClass> sut2(sut);

    ASSERT_THAT(sut2.has_value(), Eq(false));
}

TEST_F(Optional_test, CopyAssignmentWithValue)
{
    iox::cxx::optional<TestClass> sut2{TestClass{7474, 33331}};
    {
        iox::cxx::optional<TestClass> sut{TestClass{4711, 1337}};
        sut2 = sut;
    }

    ASSERT_THAT(sut2.has_value(), Eq(true));
    EXPECT_THAT(sut2->value, Eq(4711));
    EXPECT_THAT(sut2->secondValue, Eq(1337));
}

TEST_F(Optional_test, CopyAssignmentNoValue)
{
    iox::cxx::optional<TestClass> sut2{TestClass{7474, 33331}};
    {
        iox::cxx::optional<TestClass> sut = iox::cxx::nullopt_t();
        sut2 = sut;
    }

    ASSERT_THAT(sut2.has_value(), Eq(false));
}

TEST_F(Optional_test, CopyAssignmentFromNoValueToNoValue)
{
    iox::cxx::optional<TestClass> sut1;
    iox::cxx::optional<TestClass> sut2;
    sut2 = sut1;

    ASSERT_THAT(sut2.has_value(), Eq(false));
}

TEST_F(Optional_test, MoveCTorWithValue)
{
    iox::cxx::optional<TestClass> sut{TestClass{4711, 1337}};

    iox::cxx::optional<TestClass> sut2(std::move(sut));

    ASSERT_THAT(sut2.has_value(), Eq(true));
    EXPECT_THAT(sut2->value, Eq(4711));
    EXPECT_THAT(sut2->secondValue, Eq(1337));
    EXPECT_THAT(sut.has_value(), Eq(false));
}


TEST_F(Optional_test, MoveCTorWithNoValue)
{
    iox::cxx::optional<TestClass> sut = iox::cxx::nullopt_t();
    iox::cxx::optional<TestClass> sut2(std::move(sut));

    ASSERT_THAT(sut2.has_value(), Eq(false));
    EXPECT_THAT(sut.has_value(), Eq(false));
}

TEST_F(Optional_test, MoveAssignmentWithValue)
{
    iox::cxx::optional<TestClass> sut2{TestClass{7718, 80091}};
    {
        iox::cxx::optional<TestClass> sut{TestClass{4711, 1337}};
        sut2 = std::move(sut);
    }

    ASSERT_THAT(sut2.has_value(), Eq(true));
    EXPECT_THAT(sut2->value, Eq(4711));
    EXPECT_THAT(sut2->secondValue, Eq(1337));
}

TEST_F(Optional_test, MoveAssignmentWithNoValue)
{
    iox::cxx::optional<TestClass> sut2{{0, 0}};
    sut2->value = 7718;
    sut2->secondValue = 80091;
    {
        iox::cxx::optional<TestClass> sut = iox::cxx::nullopt_t();
        sut2 = std::move(sut);
    }

    ASSERT_THAT(sut2.has_value(), Eq(false));
}

TEST_F(Optional_test, MoveAssignmentFromNoValueToNoValue)
{
    iox::cxx::optional<TestClass> sut1;
    iox::cxx::optional<TestClass> sut2;
    sut2 = std::move(sut1);

    ASSERT_THAT(sut2.has_value(), Eq(false));
}

namespace
{
struct DTorTest
{
    static uint64_t dtorCounter;
    ~DTorTest()
    {
        dtorCounter++;
    }
};
uint64_t DTorTest::dtorCounter = 0;
} // namespace

TEST_F(Optional_test, Destructor)
{
    {
        DTorTest::dtorCounter = 0;
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of temporary object only
        DTorTest::dtorCounter = 0;
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(1));
}

TEST_F(Optional_test, DestructorOnCopyCTor)
{
    {
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        {
            DTorTest::dtorCounter = 0;
            iox::cxx::optional<DTorTest> sut2{sut};
            EXPECT_THAT(DTorTest::dtorCounter, Eq(0));
        }
        EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut2
        EXPECT_THAT(sut.has_value(), Eq(true));
        DTorTest::dtorCounter = 0;
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut
}

TEST_F(Optional_test, DestructorOnCopyAssignment)
{
    {
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        {
            iox::cxx::optional<DTorTest> sut2{DTorTest()};
            DTorTest::dtorCounter = 0;
            sut = sut2;
            EXPECT_THAT(DTorTest::dtorCounter, Eq(0));
        }
        EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut2
        EXPECT_THAT(sut.has_value(), Eq(true));
        DTorTest::dtorCounter = 0;
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut
}

TEST_F(Optional_test, DestructorOnMoveCTor)
{
    {
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        {
            DTorTest::dtorCounter = 0;
            iox::cxx::optional<DTorTest> sut2{std::move(sut)};
            EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut
            EXPECT_THAT(sut.has_value(), Eq(false));
            DTorTest::dtorCounter = 0;
        }
        EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut2
        DTorTest::dtorCounter = 0;
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(0)); // sut dtor already called
}

TEST_F(Optional_test, DestructorOnMoveAssignment)
{
    {
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        {
            iox::cxx::optional<DTorTest> sut2{DTorTest()};
            DTorTest::dtorCounter = 0;
            sut = std::move(sut2);
            EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut2
            DTorTest::dtorCounter = 0;
        }
        EXPECT_THAT(DTorTest::dtorCounter, Eq(0));
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut
}

TEST_F(Optional_test, DestructorOnEmplace)
{
    {
        iox::cxx::optional<DTorTest> sut{DTorTest()};
        {
            iox::cxx::optional<DTorTest> sut2{DTorTest()};
            DTorTest::dtorCounter = 0;
            sut2.emplace(sut.value());
            EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of previous sut2 value
            DTorTest::dtorCounter = 0;
        }
        EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut2
        DTorTest::dtorCounter = 0;
    }
    EXPECT_THAT(DTorTest::dtorCounter, Eq(1)); // dtor of sut
}

TEST_F(Optional_test, MakeOptional)
{
    struct Make
    {
        Make(int a, int b)
            : a(a)
            , b(b)
        {
        }
        int a;
        int b;
    };

    auto sut = iox::cxx::make_optional<Make>(123, 456);
    EXPECT_THAT(sut->a, Eq(123));
    EXPECT_THAT(sut->b, Eq(456));
}

TEST_F(Optional_test, AndThenWhenContainingValue)
{
    iox::cxx::optional<int> sut(123);
    int value = 0;

    sut.and_then([&](int& v) { value = v; });
    EXPECT_THAT(value, Eq(123));
}

TEST_F(Optional_test, AndThenWhenNotContainingValue)
{
    iox::cxx::optional<int> sut;
    int value = 0;

    sut.and_then([&](int&) { value = 42; });
    EXPECT_THAT(value, Eq(0));
}

TEST_F(Optional_test, OrElseWhenContainingValue)
{
    iox::cxx::optional<int> sut(123);
    int value = 0;

    sut.or_else([&] { value = 42; });
    EXPECT_THAT(value, Eq(0));
}

TEST_F(Optional_test, OrElseWhenNotContainingValue)
{
    iox::cxx::optional<int> sut;
    int value = 0;

    sut.or_else([&] { value = 42; });
    EXPECT_THAT(value, Eq(42));
}

TEST_F(Optional_test, ReturningNulloptWithoutConstruction)
{
    auto val = []() -> iox::cxx::optional<int> { return iox::cxx::nullopt; }();
    EXPECT_THAT(val.has_value(), Eq(false));
}

TEST_F(Optional_test, CopyConstructionWithElementWorks)
{
    const TestClass testClass{5, 6};
    iox::cxx::optional<TestClass> sut(testClass);

    ASSERT_TRUE(sut.has_value());
    EXPECT_THAT(sut->value, Eq(5));
    EXPECT_THAT(sut->secondValue, Eq(6));
}
} // namespace
