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

#include "iceoryx_utils/internal/concurrent/sofi.hpp"

#include <gtest/gtest.h>
#include <stdlib.h>

namespace
{
class CUnitTestContainerSoFi : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

    /// @brief returns the ID of the current test in the format "TestCaseName.TestName"
    /// @return std::string: the id of the test
    std::string testId();

    /// @brief pushes some serial numbers to the SoFi with the expectation to not overflow
    /// @param[in] serNumStart is the offset for the serial numbers to push into the SoFi
    /// @param[in] numberOfItems is the number of items to push into the SoFi
    /// @return int: the first not pushed serial number
    int pushSome(int serNumStart, uint32_t numberOfItems);

    /// @brief pops some items from the SoFi with the expectation of a non empty SoFi at the last pop
    /// @param[in] serNumOldest is the oldest serial numbers in the SoFi
    /// @param[in] numberOfItems is the number of items to pop from the SoFi
    void popSome(int serNumOldest, uint32_t numberOfItems);


    /// @brief pops all items from the SoFi
    /// @param[in] serNumOldest is the oldest serial numbers in the SoFi
    void popAll(int serNumOldest);

    /// @brief checks the SoFi empty behaviour; pop on empty SoFi, pushing and popping the same amount of items
    /// @param[in] scope is the identifier for gTest SCOPED_TRACE to trace the failure when subroutines are used
    /// @param[in] serNumStart is the offset for the serial numbers to push into the SoFi
    void checkEmpty(const std::string& scope, int serNumStart);

    /// @brief checks the capacity ot the SoFi
    /// @param[in] scope is the identifier for gTest SCOPED_TRACE to trace the failure when subroutines are used
    /// @param[in] serNumStart is the offset for the serial numbers to push into the SoFi
    void checkCapacity(const std::string& scope, int serNumStart);

    /// @brief checks if the SoFi overflow works as expected with one overflow
    /// @param[in] scope is the identifier for gTest SCOPED_TRACE to trace the failure when subroutines are used
    /// @param[in] serNumStart is the offset for the serial numbers to push into the SoFi
    void checkOverflow(const std::string& scope, int serNumStart);

    /// @brief checks if the SoFi overflow works as expected with multiple overflowing
    /// @param[in] scope is the identifier for gTest SCOPED_TRACE to trace the failure when subroutines are used
    /// @param[in] serNumStart is the offset for the serial numbers to push into the SoFi
    void checkMultiOverflow(const std::string& scope, int serNumStart);

    static constexpr uint64_t TEST_SOFI_CAPACITY = 10;
    iox::concurrent::SoFi<int, TEST_SOFI_CAPACITY> m_sofi;
};

std::string CUnitTestContainerSoFi::testId()
{
    return std::string(::testing::UnitTest::GetInstance()->current_test_info()->test_case_name()) + "."
           + std::string(::testing::UnitTest::GetInstance()->current_test_info()->name());
}

int CUnitTestContainerSoFi::pushSome(int serNumStart, uint32_t numberOfItems)
{
    int valIn = serNumStart;
    int valOut;

    // fill the SoFi; SoFi has an internal capacity with two more items than specified the ones
    // the write position must always point to an empty position
    // -> SoFi is full when we are at TEST_INTERNAL_SOFI_CAPACITY-1
    for (uint32_t i = 0; i < numberOfItems; i++, valIn++)
    {
        valOut = -1; // we push only positive values, so this is a value we shouldn't get from the SoFi
        EXPECT_TRUE(this->m_sofi.push(valIn, valOut)) << "There shouldn't be an overflow here!";
        EXPECT_EQ(-1, valOut); // valOut should not be changed if there was no overflow
    }

    return valIn;
}

void CUnitTestContainerSoFi::popSome(int serNumOldest, uint32_t numberOfItems)
{
    int valOut;
    for (uint32_t i = 0; i < numberOfItems; i++)
    {
        valOut = -2; // set valOut to a value not present in the SoFi
        EXPECT_TRUE(m_sofi.pop(valOut)) << "SoFi shouldn't be empty here!";
        EXPECT_EQ(serNumOldest + static_cast<int>(i), valOut); // check if the serial numbers are really consecutive
    }
}

void CUnitTestContainerSoFi::popAll(int serNumOldest)
{
    int valOut{-2}; // set valOut to a value not present in the SoFi
    int serNum = serNumOldest;
    while (m_sofi.pop(valOut))
    {
        EXPECT_EQ(serNum, valOut); // check if we have valid data
        serNum++;
        valOut = -2;
    }
}

void CUnitTestContainerSoFi::checkEmpty(const std::string& scope, int serNumStart)
{
    SCOPED_TRACE(scope); // just a helper to trace the failure when subroutines are used

    int valOut{-1}; // set valOut to a value not present in the SoFi

    EXPECT_TRUE(m_sofi.empty()) << "SoFi should be empty!";

    EXPECT_FALSE(m_sofi.pop(valOut)) << "It shouldn't be possible to pop from empty SoFi!";

    // push an item
    valOut = -1;                                   // set valOut to a value not present in the SoFi
    EXPECT_TRUE(m_sofi.push(serNumStart, valOut)); // if empty, it should be possible to push
    EXPECT_EQ(-1, valOut);                         // if push successful, valOut should not change

    EXPECT_FALSE(m_sofi.empty()) << "SoFi shouldn't be empty anymore!";

    valOut = -1;                     // set valOut to a value not present in the SoFi
    EXPECT_TRUE(m_sofi.pop(valOut)); // if not empty, pop should be successful
    EXPECT_EQ(serNumStart, valOut);  // if pop successful, valOut should be serNumStart

    EXPECT_TRUE(m_sofi.empty()) << "SoFi should be empty again!";
}

void CUnitTestContainerSoFi::checkCapacity(const std::string& scope, int serNumStart)
{
    SCOPED_TRACE(scope); // just a helper to trace the failure when subroutines are used

    // fill the SoFi
    int valIn = pushSome(serNumStart, TEST_SOFI_CAPACITY);
    // one more element should cause an overflow, which means the SoFi was already full
    int valOut{-1}; // set valOut to a value not present in the SoFi
    EXPECT_FALSE(m_sofi.push(valIn, valOut)) << "No overflow occured! SoFi is not full yet!";
    EXPECT_EQ(serNumStart, valOut); // in the case of an overflow, the oldest item is returned

    // empty the SoFi
    popAll(valOut + 1);
}

void CUnitTestContainerSoFi::checkOverflow(const std::string& scope, int serNumStart)
{
    SCOPED_TRACE(scope); // just a helper to trace the failure when subroutines are used

    int valIn{0};
    int valOut{-2};

    // fill the SoFi and return the first not pushed serial number
    valIn = pushSome(serNumStart, TEST_SOFI_CAPACITY);
    // pushing another item, should cause an overflow and returning the oldest pushed item
    EXPECT_FALSE(m_sofi.push(valIn, valOut)) << "Expected overflow didn't occur";
    EXPECT_EQ(serNumStart, valOut);

    // popping should return the remaining item
    popSome(serNumStart + 1, TEST_SOFI_CAPACITY); // we had an overflow, so the serial number is off by one

    // SoFi should now be empty
    valOut = -2; // set valOut to a value not present in the SoFi
    EXPECT_FALSE(m_sofi.pop(valOut)) << "SoFi is not empty as expected!";
    EXPECT_EQ(-2, valOut); // valOut not changed if empty
}

void CUnitTestContainerSoFi::checkMultiOverflow(const std::string& scope, int serNumStart)
{
    SCOPED_TRACE(scope); // just a helper to trace the failure when subroutines are used

    int valIn{0};
    int valOut{-2};

    // fill the SoFi and return the first not pushed serial number
    valIn = pushSome(serNumStart, TEST_SOFI_CAPACITY);
    // pushing additional items, should cause an overflow and returning the oldest pushed item
    // lets run three times throug the container
    int serNumExp = serNumStart;
    for (uint32_t i = 0; i < 3 * TEST_SOFI_CAPACITY; i++)
    {
        valOut = -2; // set valOut to a value not present in the SoFi
        EXPECT_FALSE(m_sofi.push(valIn, valOut)) << "Expected overflow didn't occur at iteration " << i << "!";
        EXPECT_EQ(serNumExp, valOut); // in case of an overflow, we should get the oldest item out of the SoFi
        valIn++;
        serNumExp++;
    }

    // popping should return the remaining item
    popSome(serNumExp, TEST_SOFI_CAPACITY); // we had an overflow, so the serial number is off by one

    // SoFi should now be empty
    valOut = -2; // set valOut to a value not present in the SoFi
    EXPECT_FALSE(m_sofi.pop(valOut)) << "SoFi is not empty as expected!";
    EXPECT_EQ(-2, valOut); // valOut not changed if empty
}

TEST_F(CUnitTestContainerSoFi, Empty)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // check if a new instace of the SoFi is empty
    EXPECT_TRUE(m_sofi.empty());

    // test with an initial SoFi read and write position of zero
    int serNumStart{1000};
    checkEmpty("first", serNumStart);
    // repeat the thest with a non zero initial read and write position
    serNumStart = 2000;
    checkEmpty("second", serNumStart);
}

TEST_F(CUnitTestContainerSoFi, Capacity)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // check if SoFi sets the right capacity
    EXPECT_EQ(static_cast<uint32_t>(TEST_SOFI_CAPACITY), m_sofi.capacity());

    // check if SoFi doesn't lie to us

    // test with an initial SoFi read and write position of zero
    int serNumStart{1000};
    checkCapacity("first", serNumStart);
    // repeat the thest with a non zero initial read and write position
    serNumStart = 2000;
    checkCapacity("second", serNumStart);
}

TEST_F(CUnitTestContainerSoFi, NewlyCreatedSoFiIsEmpty)
{
    EXPECT_TRUE(m_sofi.empty());
}

TEST_F(CUnitTestContainerSoFi, NewlyCreatedSoFiHasSizeZero)
{
    EXPECT_EQ(m_sofi.size(), 0);
}

TEST_F(CUnitTestContainerSoFi, SoFiSizeEqualsNumberOfPushes)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // check if a new instace of the SoFi is empty
    EXPECT_TRUE(m_sofi.empty());

    int ret;

    // Push 10 items till SoFi is full and check size
    for (uint32_t i = 0; i < TEST_SOFI_CAPACITY; i++)
    {
        EXPECT_EQ(m_sofi.size(), i);
        m_sofi.push(i, ret);
        EXPECT_EQ(m_sofi.size(), i + 1);
    }
}

TEST_F(CUnitTestContainerSoFi, SoFiSizeEqualsNumberOfPushesOverflow)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // check if a new instace of the SoFi is empty
    EXPECT_TRUE(m_sofi.empty());

    int ret;

    // Push 11 items to provoke overflow and check size
    for (uint32_t i = 0; i < TEST_SOFI_CAPACITY; i++)
    {
        EXPECT_EQ(m_sofi.size(), i);
        m_sofi.push(i, ret);
        EXPECT_EQ(m_sofi.size(), i + 1);
    }
}

TEST_F(CUnitTestContainerSoFi, Overflow)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // test with an initial SoFi read and write position of zero
    int serNumStart{1000};
    checkOverflow("first", serNumStart);
    // repeat the thest with a non zero initial read and write position
    serNumStart = 2000;
    checkOverflow("second", serNumStart);
}

TEST_F(CUnitTestContainerSoFi, MultiOverflow)
{
    SCOPED_TRACE(testId()); // just a helper to trace the failure when subroutines are used

    // test with an initial SoFi read and write position of zero
    int serNumStart{1000};
    checkMultiOverflow("first", serNumStart);
    // repeat the test with a non zero initial read and write position
    serNumStart = 2000;
    checkMultiOverflow("second", serNumStart);
}

TEST_F(CUnitTestContainerSoFi, ResizeFailsWhenContainingASingleElement)
{
    int ret;
    m_sofi.push(123, ret);
    EXPECT_EQ(m_sofi.setCapacity(4), false);
}

TEST_F(CUnitTestContainerSoFi, ResizeFailsWhenContainingAMultipleElements)
{
    int ret;
    m_sofi.push(123, ret);
    m_sofi.push(13, ret);
    m_sofi.push(23, ret);
    EXPECT_EQ(m_sofi.setCapacity(4), false);
}

TEST_F(CUnitTestContainerSoFi, ResizeFailsWhenFull)
{
    for (int ret; !m_sofi.push(123, ret);)
    {
    }
    EXPECT_EQ(m_sofi.setCapacity(4), false);
}

TEST_F(CUnitTestContainerSoFi, ResizingLargeThanCapacityFails)
{
    EXPECT_EQ(m_sofi.setCapacity(TEST_SOFI_CAPACITY + 1), false);
}

TEST_F(CUnitTestContainerSoFi, ResizingToZeroIsValid)
{
    EXPECT_EQ(m_sofi.setCapacity(0), true);
}

TEST_F(CUnitTestContainerSoFi, ResizingDefault)
{
    EXPECT_EQ(m_sofi.setCapacity(TEST_SOFI_CAPACITY - 1), true);
}

TEST_F(CUnitTestContainerSoFi, ResizeAndSizeCheck)
{
    for (uint32_t i = 0; i < TEST_SOFI_CAPACITY; ++i)
    {
        EXPECT_EQ(m_sofi.setCapacity(i), true);
        EXPECT_EQ(m_sofi.capacity(), i);
    }
}

TEST_F(CUnitTestContainerSoFi, ResizeAndSizeFillUp)
{
    for (uint32_t i = 0; i < TEST_SOFI_CAPACITY - 1; ++i)
    {
        EXPECT_EQ(m_sofi.setCapacity(i), true);
        for (uint32_t k = 0; k < i; ++k)
        {
            int temp;
            EXPECT_EQ(m_sofi.push(static_cast<int>(k), temp), true);
        }
        for (uint32_t k = 0; k < i; ++k)
        {
            int temp;
            EXPECT_EQ(m_sofi.pop(temp), true);
            EXPECT_EQ(temp, static_cast<int>(k));
        }
    }
}

TEST_F(CUnitTestContainerSoFi, PopIfWithValidCondition)
{
    int returnValue;
    m_sofi.push(10, returnValue);
    m_sofi.push(11, returnValue);
    m_sofi.push(12, returnValue);

    int output;
    bool result = m_sofi.popIf(output, [](const int& peek) { return peek < 20; });

    EXPECT_EQ(result, true);
    EXPECT_EQ(output, 10);
}

TEST_F(CUnitTestContainerSoFi, PopIfWithInvalidCondition)
{
    int returnValue;
    m_sofi.push(15, returnValue);
    m_sofi.push(16, returnValue);
    m_sofi.push(17, returnValue);

    int output;
    bool result = m_sofi.popIf(output, [](const int& peek) { return peek < 5; });

    EXPECT_EQ(result, false);
}

TEST_F(CUnitTestContainerSoFi, PopIfOnEmpty)
{
    int output;
    bool result = m_sofi.popIf(output, [](const int& peek) { return peek < 7; });

    EXPECT_EQ(result, false);
}

TEST_F(CUnitTestContainerSoFi, PopIfFullWithValidCondition)
{
    int output;
    constexpr int INITIAL_VALUE = 100;
    constexpr int OFFSET = 2;
    for (int i = 0; i < static_cast<int>(m_sofi.capacity()) + OFFSET; i++)
        m_sofi.push(i + INITIAL_VALUE, output);

    bool result = m_sofi.popIf(output, [](const int& peek) { return peek < 150; });

    EXPECT_EQ(result, true);
    EXPECT_EQ(output, INITIAL_VALUE + OFFSET);
}

TEST_F(CUnitTestContainerSoFi, PopIfFullWithInvalidCondition)
{
    int output;
    for (int i = 0; i < static_cast<int>(m_sofi.capacity()) + 2; i++)
        m_sofi.push(i + 100, output);

    bool result = m_sofi.popIf(output, [](const int& peek) { return peek < 50; });

    EXPECT_EQ(result, false);
}

TEST_F(CUnitTestContainerSoFi, PopIfValidEmptyAfter)
{
    int output;
    m_sofi.push(2, output);

    m_sofi.popIf(output, [](const int& peek) { return peek < 50; });

    EXPECT_EQ(m_sofi.empty(), true);
}

TEST_F(CUnitTestContainerSoFi, PopIfInvalidNotEmptyAfter)
{
    int output;
    m_sofi.push(200, output);

    m_sofi.popIf(output, [](const int& peek) { return peek < 50; });

    EXPECT_EQ(m_sofi.empty(), false);
}

/// @todo popif empty test
} // namespace
