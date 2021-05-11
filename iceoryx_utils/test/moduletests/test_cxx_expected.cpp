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

#include "iceoryx_utils/cxx/expected.hpp"
#include "test.hpp"

using namespace ::testing;
using namespace ::iox::cxx;

namespace
{
class MockCallables
{
  public:
    MockCallables() = default;
    MOCK_METHOD0(onSuccess, void());
    MOCK_METHOD0(onEmpty, void());
    MOCK_METHOD0(onError, void());
};


struct TestClass
{
    TestClass(int a, int b)
        : m_a(a)
        , m_b(b)
    {
    }

    int gimme()
    {
        return m_a + m_b;
    }

    int constGimme() const
    {
        return m_a + m_b;
    }

    bool operator==(const TestClass& rhs) const
    {
        return (m_a == rhs.m_a) && (m_b == rhs.m_b);
    }

    int m_a;
    int m_b;
};

} // namespace

namespace iox
{
namespace cxx
{
template <>
struct ErrorTypeAdapter<TestClass>
{
    static TestClass getInvalidState()
    {
        return TestClass(-1, -1);
    }
};

template <>
struct ErrorTypeAdapter<std::string>
{
    static std::string getInvalidState()
    {
        return std::string("IAmInvalid");
    }
};
} // namespace cxx
} // namespace iox

namespace
{
class expected_test : public Test
{
};

using TestClassAdapter = iox::cxx::ErrorTypeAdapter<TestClass>;

enum class TestError : uint8_t
{
    INVALID_STATE,
    ERROR1,
    ERROR2,
    ERROR3
};

TEST_F(expected_test, CreateWithPODTypeIsSuccessful)
{
    auto sut = expected<int, TestError>::create_value(123);
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut.value(), Eq(123));
}

TEST_F(expected_test, CreateWithErrorResultsInError)
{
    auto sut = expected<int, TestError>::create_error(TestError::ERROR1);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR1));
}

TEST_F(expected_test, ErrorTypeOnlyConstCreateWithErrorResultsInError)
{
    const auto sut = expected<TestError>::create_error(TestError::ERROR2);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR2));
}

TEST_F(expected_test, ErrorTypeOnlyCreateWithErrorResultsInError)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR1);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR1));
}

TEST_F(expected_test, CreateFromConstErrorResultsInError)
{
    auto constError = error<TestError>(TestError::ERROR3);
    auto sut = expected<int, TestError>(constError);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR3));
}

TEST_F(expected_test, ErrorTypeOnlyCreateFromConstErrorResultsInError)
{
    auto constError = error<TestError>(TestError::ERROR1);
    auto sut = expected<TestError>(constError);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR1));
}

TEST_F(expected_test, CreateFromConstSuccessResultsInCorrectValue)
{
    auto constSuccess = success<int>(424242);
    auto sut = expected<int, TestError>(constSuccess);
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut.value(), Eq(424242));
}

TEST_F(expected_test, CreateWithComplexTypeIsSuccessful)
{
    auto sut = expected<TestClass, TestError>::create_value(12, 222);
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut.value().m_a, Eq(12));
}

TEST_F(expected_test, CreateWithSTLTypeIsSuccessful)
{
    auto sut = expected<int, std::string>::create_error("RedAlert");
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq("RedAlert"));
}

TEST_F(expected_test, CreateWithComplexErrorResultsInError)
{
    auto sut = expected<int, TestClass>::create_error(313, 212);
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error().m_b, Eq(212));
}

TEST_F(expected_test, CreateRValueAndGetErrorResultsInCorrectError)
{
    auto sut = expected<int, TestClass>::create_error(131, 121).get_error();
    EXPECT_THAT(sut.m_b, Eq(121));
}

TEST_F(expected_test, ConstCreateLValueAndGetErrorResultsInCorrectError)
{
    const auto& sut = expected<int, TestClass>::create_error(343, 232);
    EXPECT_THAT(sut.get_error().m_b, Eq(232));
}

TEST_F(expected_test, CreateWithValueAndMoveCtorLeadsToInvalidState)
{
    auto sut = expected<int, TestClass>::create_value(177);
    auto movedValue{std::move(sut)};
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestClassAdapter::getInvalidState()));
}

TEST_F(expected_test, CreateWithErrorAndMoveCtorLeadsToInvalidState)
{
    auto sut = expected<int, TestClass>::create_error(22, 33);
    auto movedValue{std::move(sut)};
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestClassAdapter::getInvalidState()));
}

TEST_F(expected_test, CreateWithValueAndMoveAssignmentLeadsToInvalidState)
{
    auto sut = expected<int, TestClass>::create_value(73);
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestClassAdapter::getInvalidState()));
}

TEST_F(expected_test, CreateWithErrorAndMoveAssignmentLeadsToInvalidState)
{
    auto sut = expected<int, TestClass>::create_error(44, 55);
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestClassAdapter::getInvalidState()));
}

TEST_F(expected_test, CreateInvalidExpectedAndCallGetErrorLeadsToInvalidState)
{
    auto sut = expected<int, TestError>::create_error(TestError::ERROR1);
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, ErrorTypeOnlyCreateInvalidExpectedAndCallGetErrorLeadsToInvalidState)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR2);
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, BoolOperatorReturnsError)
{
    auto sut = expected<int, TestClass>::create_error(123, 321);
    ASSERT_THAT(sut.operator bool(), Eq(false));
    EXPECT_THAT(sut.get_error().m_b, Eq(321));
}

TEST_F(expected_test, BoolOperatorReturnsNoError)
{
    auto sut = expected<TestClass, TestError>::create_value(123, 321);

    ASSERT_THAT(sut.operator bool(), Eq(true));
    EXPECT_THAT(sut.value().m_a, Eq(123));
}

TEST_F(expected_test, ErrorTypeOnlyBoolOperatorReturnsError)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR1);
    ASSERT_THAT(sut.operator bool(), Eq(false));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR1));
}

TEST_F(expected_test, ErrorTypeOnlyBoolOperatorReturnsNoError)
{
    auto sut = expected<TestError>::create_value();
    ASSERT_THAT(sut.operator bool(), Eq(true));
}

TEST_F(expected_test, ValueOrWithErrorReturnsGivenValue)
{
    auto sut = expected<int, TestError>::create_error(TestError::ERROR1);
    EXPECT_THAT(sut.value_or(90), Eq(90));
}

TEST_F(expected_test, ConstValueOrWithErrorReturnsGivenValue)
{
    const auto sut = expected<int, TestError>::create_error(TestError::ERROR1);
    EXPECT_THAT(sut.value_or(51), Eq(51));
}

TEST_F(expected_test, ValueOrWithSuccessReturnsStoredValue)
{
    auto sut = expected<int, TestError>::create_value(999);
    EXPECT_THAT(sut.value_or(15), Eq(999));
}

TEST_F(expected_test, ConstValueOrWithSuccessReturnsStoredValue)
{
    const auto sut = expected<int, TestError>::create_value(652);
    EXPECT_THAT(sut.value_or(15), Eq(652));
}

TEST_F(expected_test, ArrowOperatorWorks)
{
    auto sut = expected<TestClass, TestError>::create_value(55, 81);
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut->gimme(), Eq(136));
}

TEST_F(expected_test, ConstArrowOperatorWorks)
{
    const expected<TestClass, TestError> sut(success<TestClass>(TestClass(55, 81)));
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut->constGimme(), Eq(136));
}

TEST_F(expected_test, DereferencingOperatorWorks)
{
    auto sut = expected<int, TestError>::create_value(1652);
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(*sut, Eq(1652));
}

TEST_F(expected_test, ConstDereferencingOperatorWorks)
{
    const expected<int, TestError> sut(success<int>(981));
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(*sut, Eq(981));
}

TEST_F(expected_test, ErrorTypeOnlyCreateValueWithoutValueLeadsToValidSut)
{
    auto sut = expected<TestError>::create_value();
    ASSERT_THAT(sut.has_error(), Eq(false));
}

TEST_F(expected_test, ErrorTypeOnlyCreateErrorLeadsToError)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR2);
    ASSERT_THAT(sut.has_error(), Eq(true));
    ASSERT_THAT(sut.get_error(), Eq(TestError::ERROR2));
}

TEST_F(expected_test, ErrorTypeOnlyMoveCtorLeadsToInvalidState)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR2);
    auto movedValue{std::move(sut)};
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, ErrorTypeOnlyMoveAssignmentLeadsToInvalidState)
{
    auto sut = expected<TestError>::create_error(TestError::ERROR1);
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    ASSERT_TRUE(sut.has_error());
    EXPECT_THAT(sut.get_error(), Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, CreateFromEmptySuccessTypeLeadsToValidSut)
{
    expected<TestError> sut{success<>()};
    ASSERT_THAT(sut.has_error(), Eq(false));
}

TEST_F(expected_test, CreateFromSuccessTypeLeadsToValidSut)
{
    expected<int, TestError> sut{success<int>(55)};
    ASSERT_THAT(sut.has_error(), Eq(false));
    EXPECT_THAT(sut.value(), Eq(55));
}

TEST_F(expected_test, CreateFromErrorConstLeadsToCorrectError)
{
    const TestError f = TestError::ERROR1;
    expected<TestError> sut{error<TestError>(f)};
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR1));
}

TEST_F(expected_test, ErrorTypeOnlyCreateFromErrorLeadsToCorrectError)
{
    expected<TestError> sut{error<TestError>(TestError::ERROR2)};
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR2));
}

TEST_F(expected_test, CreateFromErrorLeadsToCorrectError)
{
    expected<int, TestError> sut{error<TestError>(TestError::ERROR2)};
    ASSERT_THAT(sut.has_error(), Eq(true));
    EXPECT_THAT(sut.get_error(), Eq(TestError::ERROR2));
}

TEST_F(expected_test, WhenHavingAnErrorCallsOrElse)
{
    expected<int, TestError> sut{error<TestError>(TestError::ERROR1)};
    TestError error;
    sut.and_then([&](auto&) { error = TestError::ERROR2; }).or_else([&](auto& r) { error = r; });

    EXPECT_THAT(error, Eq(TestError::ERROR1));
}

TEST_F(expected_test, ConstWhenHavingAnErrorCallsOrElse)
{
    const expected<int, TestError> sut{error<TestError>(TestError::ERROR2)};
    TestError error;
    sut.and_then([&](auto&) { error = TestError::ERROR1; }).or_else([&](auto& r) { error = r; });

    EXPECT_THAT(error, Eq(TestError::ERROR2));
}

TEST_F(expected_test, ErrorTypeOnlyWhenHavingAnErrorCallsOrElse)
{
    expected<TestError> sut{error<TestError>(TestError::ERROR2)};
    TestError error;
    sut.and_then([&]() { error = TestError::ERROR1; }).or_else([&](auto& r) { error = r; });

    EXPECT_THAT(error, Eq(TestError::ERROR2));
}

TEST_F(expected_test, ErrorTypeOnlyConstWhenHavingAnErrorCallsOrElse)
{
    const expected<TestError> sut{error<TestError>(TestError::ERROR1)};
    float a = 55.44f;
    sut.and_then([&]() { a = 91.f; }).or_else([&](auto&) { a = 612.1f; });

    EXPECT_THAT(a, Eq(612.1f));
}

TEST_F(expected_test, ErrorTypeOnlyWhenHavingSuccessCallsAndThen)
{
    expected<TestError> sut{success<>()};
    int a = 0;
    sut.and_then([&]() { a = 65; }).or_else([&](auto&) { a = 111111; });

    EXPECT_THAT(a, Eq(65));
}

TEST_F(expected_test, WhenHavingSuccessCallsAndThen)
{
    expected<int, TestError> sut{success<int>(112)};
    int a = 0;
    sut.and_then([&](auto& r) { a = r; }).or_else([&](auto&) { a = 3; });

    EXPECT_THAT(a, Eq(112));
}

TEST_F(expected_test, ConstWhenHavingSuccessCallsAndThen)
{
    const expected<int, TestError> sut{success<int>(1142)};
    int a = 0;
    sut.and_then([&](auto& r) { a = r; }).or_else([&](auto&) { a = 3; });

    EXPECT_THAT(a, Eq(1142));
}

TEST_F(expected_test, WhenHavingSuccessAndMoveAssignmentCallsOrElse)
{
    expected<int, TestError> sut{success<int>(1143)};
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    TestError error{TestError::ERROR1};
    sut.and_then([&](auto&) { error = TestError::ERROR2; }).or_else([&](auto& e) { error = e; });
    EXPECT_THAT(error, Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, WhenHavingAnErrorAndMoveAssignmentCallsOrElse)
{
    expected<int, TestError> sut{error<TestError>(TestError::ERROR1)};
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    TestError error{TestError::ERROR1};
    sut.and_then([&](auto&) { error = TestError::ERROR2; }).or_else([&](auto& e) { error = e; });
    EXPECT_THAT(error, Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, ErrorTypeOnlyWhenHavingSuccessAndMoveAssignmentCallsOrElse)
{
    expected<TestError> sut{success<>()};
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    TestError error{TestError::ERROR1};
    sut.and_then([&]() { error = TestError::ERROR2; }).or_else([&](auto& e) { error = e; });
    EXPECT_THAT(error, Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, ErrorTypeOnlyWhenHavingAnErrorAndMoveAssignmentCallsOrElse)
{
    expected<TestError> sut{error<TestError>(TestError::ERROR1)};
    auto movedValue = std::move(sut);
    IOX_DISCARD_RESULT(movedValue);
    TestError error{TestError::ERROR1};
    sut.and_then([&]() { error = TestError::ERROR2; }).or_else([&](auto& e) { error = e; });
    EXPECT_THAT(error, Eq(TestError::INVALID_STATE));
}

TEST_F(expected_test, ConvertNonEmptySuccessResultToErrorTypeOnlyResult)
{
    expected<int, TestError> sut{success<int>(123)};
    expected<TestError> sut2 = sut;
    EXPECT_THAT(sut2.has_error(), Eq(false));
}

TEST_F(expected_test, ConvertConstNonEmptySuccessResultToErrorTypeOnlyResult)
{
    const expected<int, TestError> sut{success<int>(123)};
    expected<TestError> sut2 = sut;
    EXPECT_THAT(sut2.has_error(), Eq(false));
}

TEST_F(expected_test, ConvertNonEmptyErrorResultToErrorTypeOnlyResult)
{
    expected<int, TestError> sut{error<TestError>(TestError::ERROR2)};
    expected<TestError> sut2 = sut;
    EXPECT_THAT(sut2.has_error(), Eq(true));
    EXPECT_THAT(sut2.get_error(), Eq(TestError::ERROR2));
}

TEST_F(expected_test, ExpectedWithValueConvertsToOptionalWithValue)
{
    expected<int, TestError> sut{success<int>(4711)};
    optional<int> value = sut.to_optional();
    ASSERT_THAT(value.has_value(), Eq(true));
    EXPECT_THAT(*value, Eq(4711));
}

TEST_F(expected_test, ExpectedWithErrorConvertsToOptionalWithoutValue)
{
    expected<int, TestError> sut{error<TestError>(TestError::ERROR1)};
    optional<int> value = sut.to_optional();
    ASSERT_THAT(value.has_value(), Eq(false));
}

TEST_F(expected_test, AndThenUnpacksOptionalWhenNonEmptyOptionalValue)
{
    auto sut = expected<iox::cxx::optional<int>, TestError>::create_value(123);
    MockCallables mocks{};
    EXPECT_CALL(mocks, onSuccess).Times(1);

    sut.and_then([&mocks](int& val) {
        mocks.onSuccess();
        ASSERT_THAT(val, Eq(123));
    });
}

TEST_F(expected_test, ConstAndThenUnpacksOptionalWhenNonEmptyOptionalValue)
{
    const auto sut = expected<iox::cxx::optional<int>, TestError>::create_value(321);
    MockCallables mocks{};
    EXPECT_CALL(mocks, onSuccess).Times(1);

    sut.and_then([&mocks](int& val) {
        mocks.onSuccess();
        ASSERT_THAT(val, Eq(321));
    });
}

TEST_F(expected_test, AndThenNotCalledWhenEmptyOptionalValue)
{
    auto sut = expected<iox::cxx::optional<int>, TestError>::create_value(iox::cxx::nullopt);
    MockCallables mocks{};
    EXPECT_CALL(mocks, onSuccess).Times(0);

    sut.and_then([&mocks](int&) { mocks.onSuccess(); });
}

TEST_F(expected_test, AndThenInValueExpectedWithEmptyCallableDoesNotDie)
{
    auto sut1 = expected<int, TestError>::create_value(123);
    const auto sut2 = expected<int, TestError>::create_value(123);
    auto sut3 = expected<iox::cxx::optional<int>, TestError>::create_value(123);
    const auto sut4 = expected<iox::cxx::optional<int>, TestError>::create_value(123);

    // we test here that std::terminate is not called from the function_ref
    sut1.and_then(iox::cxx::function_ref<void(int&)>());
    sut2.and_then(iox::cxx::function_ref<void(int&)>());
    sut3.and_then(iox::cxx::function_ref<void(int&)>());
    sut4.and_then(iox::cxx::function_ref<void(int&)>());
}

TEST_F(expected_test, OrElseInValueExpectedWithEmptyCallableDoesNotDie)
{
    auto sut1 = expected<int, TestError>::create_error(TestError::ERROR1);
    const auto sut2 = expected<int, TestError>::create_error(TestError::ERROR1);

    // we test here that std::terminate is not called from the function_ref
    sut1.or_else(iox::cxx::function_ref<void(TestError&)>());
    sut2.or_else(iox::cxx::function_ref<void(TestError&)>());
}

TEST_F(expected_test, AndThenInErrorExpectedWithEmptyCallableDoesNotDie)
{
    auto sut1 = expected<TestError>::create_value();
    const auto sut2 = expected<TestError>::create_value();

    // we test here that std::terminate is not called from the function_ref
    sut1.and_then(iox::cxx::function_ref<void()>());
    sut2.and_then(iox::cxx::function_ref<void()>());
}

TEST_F(expected_test, OrElseInErrorExpectedWithEmptyCallableDoesNotDie)
{
    auto sut1 = expected<TestError>::create_error(TestError::ERROR1);
    const auto sut2 = expected<TestError>::create_error(TestError::ERROR1);

    // we test here that std::terminate is not called from the function_ref
    sut1.or_else(iox::cxx::function_ref<void(TestError&)>());
    sut2.or_else(iox::cxx::function_ref<void(TestError&)>());
}
} // namespace
