// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_utils/cxx/method_callback.hpp"

namespace
{
using namespace ::testing;
using namespace iox;
using namespace iox::cxx;

class MethodCallback_test : public Test
{
  public:
    class TestClass
    {
      public:
        void voidVoidMethod()
        {
        }

        void constVoidVoidMethod() const
        {
        }

        void voidVoidMethod2()
        {
        }

        void constVoidVoidMethod2() const
        {
        }

        int myMethod(int a, int b)
        {
            return a + b + m_id;
        }

        int myConstMethod(int a, int b) const
        {
            return a * b + m_id;
        }

        int myMethod2(int a, int b)
        {
            return a + b + 2 + m_id;
        }

        int myConstMethod2(int a, int b) const
        {
            return a * b + 2 + m_id;
        }

        int m_id = 0;
    };

    TestClass m_testClass;
    TestClass m_testClass2;
};

TEST_F(MethodCallback_test, DefaultMethodCallbackCtorProvidesInvalidCallback)
{
    MethodCallback<void> sut;
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, DefaultConstMethodCallbackCtorProvidesInvalidCallback)
{
    ConstMethodCallback<void> sut;
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, NonDefaultMethodCallbackCtorCreatesValidCallback)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_TRUE(sut.isValid());
}

TEST_F(MethodCallback_test, NonDefaultConstMethodCallbackCtorCreatesValidCallback)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_TRUE(sut.isValid());
}

TEST_F(MethodCallback_test, GetObjectPointerReturnsValidConstMethodCallbackClass)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    EXPECT_EQ(sut.getObjectPointer<TestClass>(), &m_testClass);
}

TEST_F(MethodCallback_test, GetObjectPointerReturnsValidMethodCallbackClass)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    EXPECT_EQ(sut.getObjectPointer<TestClass>(), &m_testClass);
}

TEST_F(MethodCallback_test, GetMethodPointerReturnsValidConstMethodCallbackClass)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    EXPECT_EQ(sut.getMethodPointer<TestClass>(), &TestClass::constVoidVoidMethod);
}

TEST_F(MethodCallback_test, GetMethodPointerReturnsValidMethodCallbackClass)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    EXPECT_EQ(sut.getMethodPointer<TestClass>(), &TestClass::voidVoidMethod);
}

TEST_F(MethodCallback_test, AssignCallbackClassPtrLeadsToValidMethodCallback)
{
    MethodCallback<void> sut;
    sut.setCallback(m_testClass, &TestClass::voidVoidMethod);
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_TRUE(sut.isValid());
}

TEST_F(MethodCallback_test, AssignCallbackClassPtrLeadsToValidConstMethodCallback)
{
    ConstMethodCallback<void> sut;
    sut.setCallback(m_testClass, &TestClass::constVoidVoidMethod);
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_TRUE(sut.isValid());
}

TEST_F(MethodCallback_test, TwoConstCallbacksWithSameClassAndMethodAreEqual)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    ConstMethodCallback<void> sut2(m_testClass, &TestClass::constVoidVoidMethod);

    EXPECT_TRUE(sut == sut2);
    EXPECT_FALSE(sut != sut2);
}

TEST_F(MethodCallback_test, TwoCallbacksWithSameClassAndMethodAreEqual)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    MethodCallback<void> sut2(m_testClass, &TestClass::voidVoidMethod);

    EXPECT_TRUE(sut == sut2);
    EXPECT_FALSE(sut != sut2);
}

TEST_F(MethodCallback_test, TwoConstCallbacksWithDifferentClassPtrAreNotEqual)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    ConstMethodCallback<void> sut2(m_testClass2, &TestClass::constVoidVoidMethod);

    EXPECT_FALSE(sut == sut2);
    EXPECT_TRUE(sut != sut2);
}

TEST_F(MethodCallback_test, TwoCallbacksWithDifferentClassPtrAreNotEqual)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    MethodCallback<void> sut2(m_testClass2, &TestClass::voidVoidMethod);

    EXPECT_FALSE(sut == sut2);
    EXPECT_TRUE(sut != sut2);
}

TEST_F(MethodCallback_test, TwoConstCallbacksWithDifferentMethodPtrAreNotEqual)
{
    ConstMethodCallback<void> sut(m_testClass, &TestClass::constVoidVoidMethod);
    ConstMethodCallback<void> sut2(m_testClass, &TestClass::constVoidVoidMethod2);

    EXPECT_FALSE(sut == sut2);
    EXPECT_TRUE(sut != sut2);
}

TEST_F(MethodCallback_test, TwoCallbacksWithDifferentMethodPtrAreNotEqual)
{
    MethodCallback<void> sut(m_testClass, &TestClass::voidVoidMethod);
    MethodCallback<void> sut2(m_testClass2, &TestClass::voidVoidMethod2);

    EXPECT_FALSE(sut == sut2);
    EXPECT_TRUE(sut != sut2);
}

TEST_F(MethodCallback_test, InvalidConstCallbackCalledWillReturnError)
{
    ConstMethodCallback<void> sut;

    auto result = sut();
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), MethodCallbackError::UNINITIALIZED_CALLBACK);
}

TEST_F(MethodCallback_test, InvalidCallbackCalledWillReturnError)
{
    MethodCallback<void> sut;

    auto result = sut();
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), MethodCallbackError::UNINITIALIZED_CALLBACK);
}

TEST_F(MethodCallback_test, ValidConstCallbackReturnsValue)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);

    auto result = sut(4, 5);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 4 * 5);
}

TEST_F(MethodCallback_test, ValidCallbackReturnsValue)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);

    auto result = sut(6, 7);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 6 + 7);
}

TEST_F(MethodCallback_test, SetNewCallbackMethodOnConstMethodCallbackReturnsValue)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    sut.setCallback(m_testClass, &TestClass::myConstMethod2);

    auto result = sut(4, 5);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 4 * 5 + 2);
}

TEST_F(MethodCallback_test, SetNewCallbackMethodOnMethodCallbackReturnsValue)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    sut.setCallback(m_testClass, &TestClass::myMethod2);

    auto result = sut(6, 7);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 6 + 7 + 2);
}

TEST_F(MethodCallback_test, SetNewCallbackOriginOnConstMethodCallbackReturnsValue)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    sut.setCallback(m_testClass2, &TestClass::myConstMethod);
    m_testClass2.m_id = 567;

    auto result = sut(4, 5);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 4 * 5 + 567);
}

TEST_F(MethodCallback_test, SetNewCallbackOriginOnMethodCallbackReturnsValue)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    sut.setCallback(m_testClass2, &TestClass::myMethod);
    m_testClass2.m_id = 5671;

    auto result = sut(6, 7);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 6 + 7 + 5671);
}

TEST_F(MethodCallback_test, SetNewCallbackFullOnConstMethodCallbackReturnsValue)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    sut.setCallback(m_testClass2, &TestClass::myConstMethod2);
    m_testClass2.m_id = 1567;

    auto result = sut(4, 5);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 4 * 5 + 1567 + 2);
}

TEST_F(MethodCallback_test, SetNewCallbackFullOnMethodCallbackReturnsValue)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    sut.setCallback(m_testClass2, &TestClass::myMethod2);
    m_testClass2.m_id = 56711;

    auto result = sut(6, 7);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 6 + 7 + 56711 + 2);
}

TEST_F(MethodCallback_test, MoveCTorInvalidatesOriginForConstMethod)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    ConstMethodCallback<int, int, int> sut2(std::move(sut));

    EXPECT_TRUE(sut2.isValid());
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, MoveCTorInvalidatesOriginForMethod)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    MethodCallback<int, int, int> sut2(std::move(sut));

    EXPECT_TRUE(sut2.isValid());
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, MoveAssignmentInvalidatesOriginForConstMethod)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    ConstMethodCallback<int, int, int> sut2;
    sut2 = std::move(sut);

    EXPECT_TRUE(sut2.isValid());
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, MoveAssignmentInvalidatesOriginForMethod)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    MethodCallback<int, int, int> sut2;
    sut2 = std::move(sut);

    EXPECT_TRUE(sut2.isValid());
    EXPECT_FALSE(sut.isValid());
}

TEST_F(MethodCallback_test, MoveCTorDestinationCanCallCallbackForConstMethod)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    ConstMethodCallback<int, int, int> sut2(std::move(sut));

    auto result = sut2(8, 9);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 72);
}

TEST_F(MethodCallback_test, MoveAssignemtnDestinationCanCallCallbackForConstMethod)
{
    ConstMethodCallback<int, int, int> sut(m_testClass, &TestClass::myConstMethod);
    ConstMethodCallback<int, int, int> sut2;
    sut2 = (std::move(sut));

    auto result = sut2(10, 11);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 110);
}

TEST_F(MethodCallback_test, MoveCTorDestinationCanCallCallbackForMethod)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    MethodCallback<int, int, int> sut2(std::move(sut));

    auto result = sut2(12, 14);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 26);
}

TEST_F(MethodCallback_test, MoveAssignemtnDestinationCanCallCallbackForMethod)
{
    MethodCallback<int, int, int> sut(m_testClass, &TestClass::myMethod);
    MethodCallback<int, int, int> sut2;
    sut2 = (std::move(sut));

    auto result = sut2(11, 11);
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(*result, 22);
}
} // namespace
