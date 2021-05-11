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

#include "iceoryx_utils/cxx/unique_ptr.hpp"
#include "test.hpp"

#include <iostream>

namespace
{
using namespace ::testing;

static constexpr double_t X_POS = 0.0;
static constexpr double_t Y_POS = 1.1;
static constexpr double_t Z_POS = 2.2;

struct Position
{
    double_t x{X_POS};
    double_t y{Y_POS};
    double_t z{Z_POS};
};

class UniquePtrTest : public Test
{
  public:
    void SetUp()
    {
        m_deleterCalled = false;
        m_anotherDeleterCalled = false;
    }

    void TearDown()
    {
    }

    bool m_deleterCalled;
    bool m_anotherDeleterCalled;

    std::function<void(Position* const)> deleter = [this](Position* const p) {
        m_deleterCalled = true;
        delete p;
    };

    std::function<void(Position* const)> anotherDeleter = [this](Position* const p) {
        m_anotherDeleterCalled = true;
        delete p;
    };
};

TEST_F(UniquePtrTest, CtorWithOnlyDeleterSetsPtrToNullAndDoesntCallDeleter)
{
    {
        auto sut = iox::cxx::unique_ptr<Position>(deleter);
        EXPECT_FALSE(sut);
        EXPECT_EQ(sut.get(), nullptr);
    }
    // SUT is out of scope but shouldn't have called deleter as SUT is NULL
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, CtorWithObjectPtrAndDeleterSetsPtrToObjectAndCallsDeleter)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        EXPECT_TRUE(sut);
        EXPECT_EQ(sut.get(), object);
    }
    // SUT is out of scope and should have called deleter
    EXPECT_TRUE(m_deleterCalled);
}

TEST_F(UniquePtrTest, CtorWithObjectPtrToNullAndDeleterSetsPtrToObjectAndDoesntCallsDeleter)
{
    {
        auto sut = iox::cxx::unique_ptr<Position>(nullptr, deleter);
        EXPECT_FALSE(sut);
        EXPECT_EQ(sut.get(), nullptr);
    }
    // SUT is out of scope but shouldn't have called deleter as SUT is NULL
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, CtorUsingMoveWithObjectPtrAndDeleterSetsPtrToObjectAndCallsDeleter)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        {
            auto anotherSut = iox::cxx::unique_ptr<Position>(std::move(sut));

            // no deleter called during move
            EXPECT_FALSE(m_deleterCalled);
            EXPECT_FALSE(sut);
            EXPECT_EQ(anotherSut.get(), object);
        }
        // anotherSUT is out of scope and should have called deleter
        EXPECT_TRUE(m_deleterCalled);

        // reset deleter as it shouldn't be called again when SUT goes out of scope
        m_deleterCalled = false;
    }
    // no deleter called when SUT goes out of scope as it was moved
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, MoveAssignmentUniquePtrsSetsPtrToObjectAndCallsDeleter)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        {
            auto anotherSut = std::move(sut);

            // no deleter called during move
            EXPECT_FALSE(m_deleterCalled);
            EXPECT_FALSE(sut);
            EXPECT_EQ(anotherSut.get(), object);
        }
        // anotherSUT is out of scope and should have called deleter
        EXPECT_TRUE(m_deleterCalled);

        // reset deleter as it shouldn't be called again when SUT goes out of scope
        m_deleterCalled = false;
    }
    // no deleter called when SUT goes out of scope as it was moved
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, MoveAssignmentOverwriteAUniquePtrWithAnotherOneAndCallsAnotherDeleterOnMove)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        {
            auto anotherObject = new Position();
            auto anotherSut = iox::cxx::unique_ptr<Position>(anotherObject, anotherDeleter);

            anotherSut = std::move(sut);

            // anotherSUT is overwritten so it should have called its anotherDeleter
            EXPECT_TRUE(m_anotherDeleterCalled);
            // SUT deleter not called during move
            EXPECT_FALSE(m_deleterCalled);
            EXPECT_FALSE(sut);
            EXPECT_EQ(anotherSut.get(), object);
        }
        // anotherSUT is out of scope and should have called deleter that has been moved to it
        EXPECT_TRUE(m_deleterCalled);

        // reset deleter as it shouldn't be called again when SUT goes out of scope
        m_deleterCalled = false;
    }
    // no deleter called when SUT goes out of scope as it was moved
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, AccessUnderlyingObjectResultsInCorrectValue)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_EQ(sut->x, X_POS);
}

TEST_F(UniquePtrTest, AccessUnderlyingObjectViaGetResultsInCorrectValue)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    auto objectPtr = sut.get();

    EXPECT_EQ(objectPtr->x, X_POS);
}

TEST_F(UniquePtrTest, ReleaseAnObjectResultsInUniquePtrBeingInvalidAndReturnOfObjectPtr)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_EQ(sut.release(), object);
    EXPECT_FALSE(sut);
    delete object;
}

TEST_F(UniquePtrTest, ReleaseNullObjectResultsInUniquePtrBeingInvalidAndReturnOfNull)
{
    auto sut = iox::cxx::unique_ptr<Position>(nullptr, deleter);

    EXPECT_EQ(sut.release(), nullptr);
    EXPECT_FALSE(sut);
}

TEST_F(UniquePtrTest, ReleaseDeleterOnlyUniquePtrResultsInUniquePtrBeingInvalidAndReturnOfNull)
{
    auto sut = iox::cxx::unique_ptr<Position>(deleter);

    EXPECT_EQ(sut.release(), nullptr);
    EXPECT_FALSE(sut);
}

TEST_F(UniquePtrTest, ResetToAnExistingObjectPtrResultsInDeleterCalledTwice)
{
    {
        auto object = new Position();
        auto anotherObject = new Position();

        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

        sut.reset(anotherObject);

        // deleter called first time then SUT is resetted
        EXPECT_TRUE(m_deleterCalled);
        EXPECT_EQ(sut.get(), anotherObject);

        // reset deleter as it's called again when SUT goes out of scope
        m_deleterCalled = false;
    }
    // deleter called second time when SUT is going of scope
    EXPECT_TRUE(m_deleterCalled);
}

TEST_F(UniquePtrTest, SwapTwoValidUniquePtrsWithDifferentDeletersSucceeds)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        {
            auto anotherObject = new Position();
            auto anotherSut = iox::cxx::unique_ptr<Position>(anotherObject, anotherDeleter);

            sut.swap(anotherSut);

            // no deleter calls during swap
            EXPECT_FALSE(m_deleterCalled);
            EXPECT_EQ(sut.get(), anotherObject);
            EXPECT_EQ(anotherSut.get(), object);
        }
        // anotherSUT is out of scope and calls its deleter, which has been swapped and is now 'deleter'
        EXPECT_TRUE(m_deleterCalled);
        EXPECT_FALSE(m_anotherDeleterCalled);
    }
    // SUT is out of scope calling its anotherDeleter as it was swapped
    EXPECT_TRUE(m_anotherDeleterCalled);
}

TEST_F(UniquePtrTest, SwapUniquePtrWithADeleterOnlyUniquePtrLeadsToDeletedUniquePtr)
{
    {
        auto object = new Position();
        auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
        {
            auto anotherSut = iox::cxx::unique_ptr<Position>(anotherDeleter);

            sut.swap(anotherSut);

            // no deleter calls during swap
            EXPECT_FALSE(m_deleterCalled);
            EXPECT_FALSE(sut);
            EXPECT_EQ(anotherSut.get(), object);
        }
        // anotherSUT is out of scope and calls its deleter, which has been swapped and is now 'deleter'
        EXPECT_TRUE(m_deleterCalled);
    }
    // SUT is out of scope not calling its anotherDeleter as it's NULL
    EXPECT_FALSE(m_anotherDeleterCalled);
}

TEST_F(UniquePtrTest, SwapADeleterOnlyUniquePtrWithUniquePtrLeadsToOneValidAndOneInvalidUniquePtrs)
{
    {
        auto anotherObject = new Position();
        auto anotherSut = iox::cxx::unique_ptr<Position>(anotherObject, anotherDeleter);
        {
            auto sut = iox::cxx::unique_ptr<Position>(deleter);

            sut.swap(anotherSut);

            // no deleter calls during swap
            EXPECT_FALSE(m_anotherDeleterCalled);
            EXPECT_FALSE(anotherSut);
            EXPECT_EQ(sut.get(), anotherObject);
        }
        // SUT is out of scope and calls its anotherDeleter, which has been swapped
        EXPECT_TRUE(m_anotherDeleterCalled);
    }
    // anotherSUT is out of scope not calling its deleter as it's NULL
    EXPECT_FALSE(m_deleterCalled);
}

TEST_F(UniquePtrTest, CompareAUniquePtrWithItselfIsTrue)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_TRUE(sut == sut);
}

TEST_F(UniquePtrTest, CompareAUniquePtrWithNullIsFalse)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_FALSE(sut == nullptr);
    EXPECT_FALSE(nullptr == sut);
}

TEST_F(UniquePtrTest, CompareAUniquePtrWithAnotherOneOfAnotherObjectIsFalse)
{
    auto object = new Position;
    auto anotherObject = new Position;
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    auto anotherSut = iox::cxx::unique_ptr<Position>(anotherObject, anotherDeleter);

    EXPECT_FALSE(sut == anotherSut);
    EXPECT_FALSE(anotherSut == sut);
}

TEST_F(UniquePtrTest, NotEqualCompareOfAUniquePtrWithItselfIsFalse)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_FALSE(sut != sut);
}

TEST_F(UniquePtrTest, NotEqualCompareOfAUniquePtrWithAnotherOneOfAnotherObjectIsTrue)
{
    auto object = new Position;
    auto anotherObject = new Position;
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    auto anotherSut = iox::cxx::unique_ptr<Position>(anotherObject, anotherDeleter);

    EXPECT_TRUE(sut != anotherSut);
    EXPECT_TRUE(anotherSut != sut);
}

TEST_F(UniquePtrTest, NotEqualCompareAUniquePtrWithNullIsTrue)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);

    EXPECT_TRUE(sut != nullptr);
    EXPECT_TRUE(nullptr != sut);
}

TEST_F(UniquePtrTest, CanGetUnderlyingPtrFromConstUniquePtr)
{
    auto object = new Position();
    const auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    EXPECT_TRUE(sut.get() != nullptr);
}

TEST_F(UniquePtrTest, CanUseArrowOperatorToAccessObjectInConstUniquePtr)
{
    auto object = new Position();
    const auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    EXPECT_EQ(X_POS, sut->x);
    EXPECT_EQ(Y_POS, sut->y);
    EXPECT_EQ(Z_POS, sut->z);
}

TEST_F(UniquePtrTest, AssigningUniquePtrToNullptrDeletesTheManagedObject)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    sut = nullptr;
    EXPECT_TRUE(m_deleterCalled);
}

TEST_F(UniquePtrTest, AssigningUniquePtrToNullptrSetsUnderlyingObjectToNullptr)
{
    auto object = new Position();
    auto sut = iox::cxx::unique_ptr<Position>(object, deleter);
    sut = nullptr;
    EXPECT_EQ(nullptr, sut.get());
}
} // namespace
