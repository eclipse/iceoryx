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
#include "iceoryx_utils/cxx/list.hpp"
#include "test.hpp"


namespace
{
using namespace ::testing;
using namespace iox::cxx;

static constexpr uint64_t TESTLISTCAPACITY{10U};
static constexpr int64_t TEST_LIST_ELEMENT_DEFAULT_VALUE{-99L};

class list_test : public Test
{
  public:
    static uint64_t cTor;
    static uint64_t customCTor;
    static uint64_t copyCTor;
    static uint64_t moveCTor;
    static uint64_t moveAssignment;
    static uint64_t copyAssignment;
    static uint64_t dTor;
    static int64_t classValue;

    class TestListElement
    {
      public:
        TestListElement()
        {
            cTor++;
            classValue = m_value;
        }

        TestListElement(const int64_t value)
            : m_value(value)
        {
            customCTor++;
            classValue = m_value;
        }

        TestListElement(const TestListElement& rhs)
        {
            copyCTor++;
            m_value = rhs.m_value;
            classValue = m_value;
        }

        TestListElement(TestListElement&& rhs)
        {
            moveCTor++;
            m_value = rhs.m_value;
            classValue = m_value;
        }

        TestListElement& operator=(const TestListElement& rhs)
        {
            copyAssignment++;
            m_value = rhs.m_value;
            classValue = m_value;
            return *this;
        }

        TestListElement& operator=(TestListElement&& rhs)
        {
            moveAssignment++;
            m_value = rhs.m_value;
            classValue = m_value;
            return *this;
        }

        bool operator==(const TestListElement& rhs) const
        {
            return rhs.m_value == m_value;
        }

        ~TestListElement()
        {
            dTor++;
            classValue = m_value;
        }

        int64_t m_value = TEST_LIST_ELEMENT_DEFAULT_VALUE;
    };


    void SetUp()
    {
        cTor = 0U;
        customCTor = 0U;
        copyCTor = 0U;
        moveCTor = 0U;
        moveAssignment = 0U;
        copyAssignment = 0U;
        dTor = 0U;
        classValue = 0U;
    }

    bool isSetupState()
    {
        if (cTor == 0U && customCTor == 0U && copyCTor == 0U && moveCTor == 0U && moveAssignment == 0U
            && copyAssignment == 0U && dTor == 0U && classValue == 0U)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    list<TestListElement, TESTLISTCAPACITY> sut;
};

// list_test statics
uint64_t list_test::cTor;
uint64_t list_test::customCTor;
uint64_t list_test::copyCTor;
uint64_t list_test::moveCTor;
uint64_t list_test::moveAssignment;
uint64_t list_test::copyAssignment;
uint64_t list_test::dTor;
int64_t list_test::classValue;


template <typename IterType>
int64_t iteratorTraitReturnDoubleValue(IterType iter)
{
    typedef typename std::iterator_traits<IterType>::value_type IterValueType;
    IterValueType m_value = *iter;
    return (2 * m_value); // will only work for integer-convertible m_value types
}

// in context of EXPECT_DEATH tests, dummyFunc() shall help suppressing following warning :
// -Wunused-comparison
// reason: the warning is already addressed with the internal handling, which shall be tested here
bool dummyFunc(bool whatever)
{
    std::cerr << "Never get here - ever " << whatever << std::endl;
    return whatever;
}
} // namespace


TEST_F(list_test, NewlyCreatedListIsEmpty)
{
    EXPECT_THAT(sut.empty(), Eq(true));
}

TEST_F(list_test, NewlyCreatedListHasSizeZero)
{
    EXPECT_THAT(sut.size(), Eq(0U));
}

TEST_F(list_test, ReadCapacityOnList)
{
    EXPECT_THAT(sut.capacity(), Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, ReadMax_sizeOnList)
{
    EXPECT_THAT(sut.max_size(), Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, NewListCTorWithZeroElements)
{
    constexpr uint64_t CAPACITY{42U};
    EXPECT_THAT(isSetupState(), Eq(true));
    const list<int, CAPACITY> sut1;
    EXPECT_THAT(sut1.empty(), Eq(true));
    EXPECT_THAT(isSetupState(), Eq(true));
}

TEST_F(list_test, CbeginCendAreTheSameWhenEmpty)
{
    EXPECT_THAT(sut.cbegin() == sut.cend(), Eq(true));
}
TEST_F(list_test, BeginEndAreTheSameWhenEmpty)
{
    EXPECT_THAT(sut.begin() == sut.end(), Eq(true));
}
TEST_F(list_test, CbeginEndAreTheSameWhenEmpty)
{
    EXPECT_THAT(sut.cbegin() == sut.end(), Eq(true));
}
TEST_F(list_test, BeginCendAreTheSameWhenEmpty)
{
    EXPECT_THAT(sut.begin() == sut.cend(), Eq(true));
}

TEST_F(list_test, CbeginCendAreDifferentWhenFilled)
{
    EXPECT_THAT(sut.emplace_front().m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT(sut.cbegin() != sut.cend(), Eq(true));
}
TEST_F(list_test, BeginEndAreDifferentWhenFilled)
{
    sut.emplace_front();
    EXPECT_THAT(sut.begin() != sut.end(), Eq(true));
}
TEST_F(list_test, CbeginEndAreDifferentWhenFilled)
{
    sut.emplace_front();
    EXPECT_THAT(sut.cbegin() != sut.end(), Eq(true));
}
TEST_F(list_test, BeginCendAreDifferentWhenFilled)
{
    sut.emplace_front();
    EXPECT_THAT(sut.begin() != sut.cend(), Eq(true));
}

TEST_F(list_test, NotEmptyWhenFilled)
{
    sut.emplace_front();
    EXPECT_THAT(sut.empty(), Eq(false));
}

TEST_F(list_test, NotFullWhenEmpty)
{
    EXPECT_THAT(sut.full(), Eq(false));
}
TEST_F(list_test, NotFullWhenPartialFilled)
{
    sut.emplace_front();
    EXPECT_THAT(TESTLISTCAPACITY, Gt(1U));
    EXPECT_THAT(sut.full(), Eq(false));
}
TEST_F(list_test, FullWhenFilledWithCapacityElements)
{
    for (uint64_t i = 0U; i < sut.capacity(); ++i)
    {
        EXPECT_THAT(sut.emplace_front().m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    }
    EXPECT_THAT(sut.full(), Eq(true));
}
TEST_F(list_test, FullWhenFilledWithMoreThanCapacityElements)
{
    for (uint64_t i = 0U; i < sut.capacity(); ++i)
    {
        sut.emplace_front();
    }

    EXPECT_THAT(sut.full(), Eq(true));
    EXPECT_DEATH(sut.emplace_front(), "");
}
TEST_F(list_test, NotFullWhenFilledWithCapacityAndEraseOneElements)
{
    for (uint64_t i = 0U; i < sut.capacity(); ++i)
    {
        sut.emplace_front();
    }
    sut.erase(sut.cbegin());

    EXPECT_THAT(sut.size(), Eq(sut.capacity() - 1U));
    EXPECT_THAT(sut.full(), Eq(false));
}

TEST_F(list_test, NotFullWhenFilledWithCapacityAndEraseOneAndReinsertElements)
{
    uint64_t i = 0U;
    for (; i < sut.capacity(); ++i)
    {
        sut.emplace_back(i);
    }
    sut.erase(sut.cbegin());
    sut.erase(sut.cbegin());
    sut.emplace_back(i);
    sut.emplace_back(++i);

    i = 1;
    for (auto& element : sut)
    {
        EXPECT_THAT(element, Eq(++i));
    }

    EXPECT_THAT(sut.size(), Eq(sut.capacity()));
    EXPECT_THAT(sut.full(), Eq(true));
}

TEST_F(list_test, CTorWithOneElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{1U};
    list<TestListElement, CAPACITY> sut1;

    EXPECT_THAT(cTor, Eq(0U));
    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace_front();
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, CustomCTorWithOneElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{1U};
    constexpr uint64_t DEFAULT_VALUE{3U};
    list<TestListElement, CAPACITY> sut1;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace_front(DEFAULT_VALUE);
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
    EXPECT_THAT(classValue, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, CTorWithSomeElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{37U};
    list<TestListElement, CAPACITY> sut1;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace_front();
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, CTorWithCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY};
    list<TestListElement, CAPACITY> sut1;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace_front();
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, CTorWithMoreThanCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY};
    list<TestListElement, CAPACITY> sut1;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.push_front({});
    }
    sut1.emplace(sut1.cbegin(), 2U);

    EXPECT_THAT(sut1.size(), Eq(CAPACITY));
    EXPECT_THAT(cTor, Eq(CAPACITY));
    EXPECT_THAT(customCTor, Eq(0U));
}


TEST_F(list_test, EmplaceWithOneElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{1U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.begin();
    decltype(TestListElement::m_value) cnt = 0U;

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(0U));

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        iter = sut1.emplace(iter, cnt);
        ++cnt;
    }

    for (auto& listElement : sut1)
    {
        --cnt;
        EXPECT_THAT(listElement.m_value, Eq(cnt));
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceWithSomeElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{37U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(0U));

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        iter = sut1.emplace(iter, cnt);
        ++cnt;
    }

    for (auto& listElement : sut1)
    {
        --cnt;
        EXPECT_THAT(listElement.m_value, Eq(cnt));
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceWithCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        iter = sut1.emplace(iter, cnt);
        ++cnt;
    }

    for (auto& listElement : sut1)
    {
        --cnt;
        EXPECT_THAT(listElement.m_value, Eq(cnt));
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceWithMoreThanCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY + 1U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        iter = sut1.emplace(iter, cnt);
        ++cnt;
    }

    cnt = CAPACITY;
    for (auto& listElement : sut1)
    {
        --cnt;
        EXPECT_THAT(listElement.m_value, Eq(cnt));
    }

    EXPECT_THAT(sut1.size(), Eq(CAPACITY));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(CAPACITY));
}


TEST_F(list_test, EmplaceReverseWithOneElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{1U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace(iter, cnt);
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceReverseWithSomeElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{3U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace(iter, cnt);
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceReverseWithCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace(iter, cnt);
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT));
}

TEST_F(list_test, EmplaceReverseWithWithMoreThanCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY + 1U};
    list<TestListElement, CAPACITY> sut1;
    auto iter = sut1.cbegin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut1.emplace(iter, cnt);
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(CAPACITY));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(CAPACITY));
}


TEST_F(list_test, EmplaceBackWithOneElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{1U};
    list<TestListElement, CAPACITY> sut1;
    // TestListElement compareElement{};
    decltype(TestListElement::m_value) cnt = 0U;

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(0U));

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        EXPECT_THAT(sut1.emplace_back(cnt), Eq(TestListElement{cnt}));
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT * 2U));
}

TEST_F(list_test, EmplaceBackWithSomeElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{37U};
    list<TestListElement, CAPACITY> sut1;
    decltype(TestListElement::m_value) cnt = 0U;

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(0U));

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        EXPECT_THAT(sut1.emplace_back(cnt), Eq(TestListElement{cnt}));
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT * 2U));
}

TEST_F(list_test, EmplaceBackWithCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY};
    list<TestListElement, CAPACITY> sut1;
    decltype(TestListElement::m_value) cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        EXPECT_THAT(sut1.emplace_back(cnt), Eq(TestListElement{cnt}));
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(ELEMENT_COUNT));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(ELEMENT_COUNT * 2U));
}

TEST_F(list_test, EmplaceBackWithMoreThanCapacityElements)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{CAPACITY + 1U};
    list<TestListElement, CAPACITY> sut1;
    decltype(TestListElement::m_value) cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        if (i < CAPACITY)
        {
            EXPECT_THAT(sut1.emplace_back(cnt), Eq(TestListElement{cnt}));
        }
        else
        {
            EXPECT_DEATH(sut1.emplace_back(cnt), "");
        }
        ++cnt;
    }

    cnt = 0U;
    for (auto& listElement : sut1)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
    }

    EXPECT_THAT(sut1.size(), Eq(CAPACITY));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(CAPACITY * 2U));
}

TEST_F(list_test, EmplaceWithWrongListIterator)
{
    constexpr uint64_t CAPACITY{42U};
    constexpr uint64_t ELEMENT_COUNT{13U};
    list<TestListElement, CAPACITY> sut11, sut12;
    auto iterOfSut1 = sut11.begin();
    auto iterOfSut2 = sut12.begin();
    uint64_t cnt = 0U;

    for (uint64_t i = 0U; i < ELEMENT_COUNT; ++i)
    {
        sut11.emplace(iterOfSut1, cnt);
        ++cnt;
    }

    EXPECT_DEATH(sut11.emplace(iterOfSut2, cnt), "");
}

TEST_F(list_test, PushFrontConstCustomSuccessfullWhenSpaceAvailableLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};
    EXPECT_TRUE(sut.push_front(a));
    ASSERT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, PushFrontConstSuccessfullWhenSpaceAvailableLValue)
{
    const TestListElement a{};
    EXPECT_TRUE(sut.push_front(a));
    ASSERT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT(cTor, Eq(1U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
}

TEST_F(list_test, PushFrontFailsWhenSpaceNotAvailableLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    EXPECT_THAT(sut.size(), Eq(0U));
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_TRUE(sut.push_front(a));
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }
    EXPECT_FALSE(sut.push_front(a));
    EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, PushFrontSuccessfullWhenSpaceAvailableRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    EXPECT_THAT(sut.size(), Eq(0U));

    sut.push_front(DEFAULT_VALUE);
    EXPECT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, PushFrontFailsWhenSpaceNotAvailableRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    EXPECT_THAT(sut.size(), Eq(0U));
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_TRUE(sut.push_front(DEFAULT_VALUE));
    }

    EXPECT_FALSE(sut.push_front(DEFAULT_VALUE));

    EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY));

    for (auto& listElement : sut)
    {
        EXPECT_THAT(listElement.m_value, Eq(DEFAULT_VALUE));
    }
}


TEST_F(list_test, PushBackConstCustomSuccessfullWhenSpaceAvailableLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};
    EXPECT_TRUE(sut.push_back(a));
    ASSERT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, PushBackConstSuccessfullWhenSpaceAvailableLValue)
{
    const TestListElement a{};
    EXPECT_TRUE(sut.push_back(a));
    ASSERT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT(cTor, Eq(1U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
}

TEST_F(list_test, PushBackFailsWhenSpaceNotAvailableLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    EXPECT_THAT(sut.size(), Eq(0U));
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_TRUE(sut.push_back(a));
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }
    EXPECT_FALSE(sut.push_back(a));
    EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, PushBackSuccessfullWhenSpaceAvailableRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    EXPECT_THAT(sut.size(), Eq(0U));

    sut.push_back(DEFAULT_VALUE);
    EXPECT_THAT(sut.size(), Eq(1U));
    EXPECT_THAT((*sut.begin()).m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, PushBackFailsWhenSpaceNotAvailableRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    uint64_t i = 0U;

    EXPECT_THAT(sut.size(), Eq(0U));
    for (; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_TRUE(sut.push_back(DEFAULT_VALUE));
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    EXPECT_FALSE(sut.push_back(DEFAULT_VALUE));

    EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY));

    for (auto& listElement : sut)
    {
        EXPECT_THAT(listElement.m_value, Eq(DEFAULT_VALUE));
    }
}

TEST_F(list_test, PushBackCheckInsertPosition)
{
    uint64_t i = 0U;

    for (; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_TRUE(sut.push_back(i));
    }

    i = 0U;
    for (auto& listElement : sut)
    {
        EXPECT_THAT(listElement.m_value, Eq(i));
        ++i;
    }
}

TEST_F(list_test, AccessFrontElement)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.push_front({});
    sut.push_front(a);

    TestListElement& b{sut.front()};
    const TestListElement& c{sut.front()};
    EXPECT_THAT(b.m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT(c.m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, AccessFrontElementFromConstList)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.push_front({});
    sut.push_front(a);

    const list<TestListElement, TESTLISTCAPACITY> sut1{sut};
    const TestListElement& c = sut1.front();

    EXPECT_THAT(c.m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, AccessBackElement)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.push_front(a);
    sut.push_front({});

    TestListElement& b{sut.back()};
    const TestListElement& c{sut.back()};
    EXPECT_THAT(b.m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT(c.m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, AccessBackElementFromConstList)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.push_front(a);
    sut.push_front({});

    const list<TestListElement, TESTLISTCAPACITY> sut1{sut};
    const TestListElement& c = sut1.back();

    EXPECT_THAT(c.m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, PopFrontOnEmptyList)
{
    EXPECT_FALSE(sut.pop_front());
    ASSERT_THAT(sut.size(), Eq(0U));
    EXPECT_THAT(isSetupState(), Eq(true));
}

TEST_F(list_test, PopFrontNonEmptyList)
{
    sut.emplace_front(101U);
    ASSERT_THAT(sut.size(), Eq(1U));

    EXPECT_TRUE(sut.pop_front());

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    ASSERT_THAT(dTor, Eq(1U));
}

TEST_F(list_test, PopFrontFullToEmptyList)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut.emplace_front();
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY - i));
        EXPECT_TRUE(sut.pop_front());
    }

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(TESTLISTCAPACITY));
    ASSERT_THAT(dTor, Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, PopFrontFullPlusOneToEmptyList)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut.emplace(sut.cbegin());
        EXPECT_THAT(sut.size(), Eq((i + 1U) > TESTLISTCAPACITY ? TESTLISTCAPACITY : (i + 1U)));
    }

    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY - i));
        EXPECT_TRUE(sut.pop_front());
    }

    EXPECT_FALSE(sut.pop_front());

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(TESTLISTCAPACITY));
    ASSERT_THAT(customCTor, Eq(0U));
    ASSERT_THAT(dTor, Eq(TESTLISTCAPACITY));
}


TEST_F(list_test, PopBackOnEmptyList)
{
    EXPECT_FALSE(sut.pop_back());
    ASSERT_THAT(sut.size(), Eq(0U));
    EXPECT_THAT(isSetupState(), Eq(true));
}

TEST_F(list_test, PopBackNonEmptyList)
{
    sut.emplace_front(101U);
    ASSERT_THAT(sut.size(), Eq(1U));

    EXPECT_TRUE(sut.pop_back());

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    ASSERT_THAT(dTor, Eq(1U));
}

TEST_F(list_test, PopBackFullToEmptyList)
{
    // fill even more than size
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut.emplace_front();
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY - i));
        EXPECT_TRUE(sut.pop_back());
    }

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(TESTLISTCAPACITY));
    ASSERT_THAT(dTor, Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, PopBackFullPlusOneToEmptyList)
{
    // fill even more than size
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut.emplace(sut.cbegin());
        EXPECT_THAT(sut.size(), Eq((i + 1U) > TESTLISTCAPACITY ? TESTLISTCAPACITY : (i + 1U)));
    }

    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        EXPECT_THAT(sut.size(), Eq(TESTLISTCAPACITY - i));
        EXPECT_TRUE(sut.pop_back());
    }

    EXPECT_FALSE(sut.pop_back());

    ASSERT_THAT(sut.size(), Eq(0U));
    ASSERT_THAT(cTor, Eq(TESTLISTCAPACITY));
    ASSERT_THAT(customCTor, Eq(0U));
    ASSERT_THAT(dTor, Eq(TESTLISTCAPACITY));
}


TEST_F(list_test, InsertEmptyListAsLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.insert(sut.cbegin(), a);

    ASSERT_THAT(sut.size(), Eq(1U));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(1U));
    ASSERT_THAT(copyCTor, Eq(1U));
    ASSERT_THAT(moveCTor, Eq(0U));
    ASSERT_THAT(copyAssignment, Eq(0U));
    ASSERT_THAT(moveAssignment, Eq(0U));
}

TEST_F(list_test, InsertLValueCheckReturn)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    auto iter = sut.insert(sut.begin(), a);

    ASSERT_THAT(iter == sut.begin(), Eq(true));
}

TEST_F(list_test, InsertEmptyListAsRValue)
{
    sut.insert(sut.begin(), {10U});

    ASSERT_THAT(sut.size(), Eq(1U));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(1U));
    ASSERT_THAT(copyCTor, Eq(0U));
    ASSERT_THAT(moveCTor, Eq(1U));
    ASSERT_THAT(copyAssignment, Eq(0U));
    ASSERT_THAT(moveAssignment, Eq(0U));
}

TEST_F(list_test, InsertRValueCheckReturn)
{
    auto iter = sut.insert(sut.begin(), {10U});

    ASSERT_THAT(iter == sut.begin(), Eq(true));
    ASSERT_THAT((*iter).m_value, Eq(10U));
}

TEST_F(list_test, InsertBeginListLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.insert(sut.begin(), a);
    sut.emplace_front();

    ASSERT_THAT(sut.size(), Eq(2U));
    ASSERT_THAT(cTor, Eq(1U));
    ASSERT_THAT(customCTor, Eq(1U));
    auto iter = sut.begin();
    EXPECT_THAT(iter->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(DEFAULT_VALUE));
}


TEST_F(list_test, InsertBeforeBeginListLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.emplace_front();
    sut.insert(sut.begin(), a);

    ASSERT_THAT(sut.size(), Eq(2U));
    ASSERT_THAT(cTor, Eq(1U));
    ASSERT_THAT(customCTor, Eq(1U));
    auto iter = sut.begin();
    EXPECT_THAT((*iter).m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
}

TEST_F(list_test, InsertBeforeBeginListRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};

    sut.emplace_front(a);
    sut.insert(sut.begin(), {});

    ASSERT_THAT(sut.size(), Eq(2U));
    ASSERT_THAT(cTor, Eq(1U));
    ASSERT_THAT(customCTor, Eq(1U));
    auto iter = sut.begin();
    EXPECT_THAT((*iter).m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(DEFAULT_VALUE));
}

TEST_F(list_test, InsertSomeElementsListLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};
    uint64_t loopCounter = 0U;

    // this tests the test case setup (the following code needs a minimum testlist capacity)
    ASSERT_THAT(TESTLISTCAPACITY, Ge(10U));

    // fill half
    for (uint64_t i = 0U; i < 5U; ++i)
    {
        sut.emplace_front(i);
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    auto iter = sut.begin();
    // 2 increments
    for (uint64_t i = 0U; i < 2U; ++i)
    {
        ++iter;
    }
    sut.insert(iter, a);

    for (auto& x IOX_MAYBE_UNUSED : sut)
    {
        ++loopCounter;
    }

    ASSERT_THAT(sut.size(), Eq(6U));
    ASSERT_THAT(loopCounter, Eq(6U));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(6U));

    iter = sut.begin();
    EXPECT_THAT(iter->m_value, Eq(4U));
    EXPECT_THAT((++iter)->m_value, Eq(3U));
    EXPECT_THAT((++iter)->m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(2U));
    EXPECT_THAT((++iter)->m_value, Eq(1U));
    EXPECT_THAT((++iter)->m_value, Eq(0U));
}

TEST_F(list_test, InsertSomeElementsListRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    // test requires a minimum capacity
    ASSERT_THAT(TESTLISTCAPACITY, Ge(10U));

    // fill half
    for (uint64_t i = 0U; i < 5U; ++i)
    {
        sut.emplace_front(i);
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    auto iter = sut.begin();
    // 2 increments
    for (uint64_t i = 0U; i < 2U; ++i)
    {
        ++iter;
    }
    sut.insert(iter, DEFAULT_VALUE);

    ASSERT_THAT(sut.size(), Eq(6U));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(6U));

    iter = sut.begin();
    EXPECT_THAT(iter->m_value, Eq(4U));
    EXPECT_THAT((++iter)->m_value, Eq(3U));
    EXPECT_THAT((++iter)->m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(2U));
    EXPECT_THAT((++iter)->m_value, Eq(1U));
    EXPECT_THAT((++iter)->m_value, Eq(0U));
}


TEST_F(list_test, InsertFullElementsListLValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    const TestListElement a{DEFAULT_VALUE};
    int64_t cnt = 0U;

    auto iter = sut.begin();

    // fill full-1
    for (uint64_t i = 0U; i < TESTLISTCAPACITY - 1; ++i)
    {
        sut.emplace(iter, i);
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    sut.insert(iter, a);

    ASSERT_THAT(sut.size(), Eq(TESTLISTCAPACITY));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(TESTLISTCAPACITY));

    for (auto& listElement : sut)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
        if (TESTLISTCAPACITY - 1L == cnt)
        {
            // for the last element (insert) check for different m_value
            cnt = DEFAULT_VALUE;
        }
    }
}

TEST_F(list_test, InsertFullElementsListRValue)
{
    constexpr int64_t DEFAULT_VALUE{13L};
    int64_t cnt = 0U;

    auto iter = sut.begin();

    // fill full-1
    for (uint64_t i = 0U; i < TESTLISTCAPACITY - 1; ++i)
    {
        sut.emplace(iter, i);
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    sut.insert(iter, DEFAULT_VALUE);

    ASSERT_THAT(sut.size(), Eq(TESTLISTCAPACITY));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(TESTLISTCAPACITY));

    for (auto& listElement : sut)
    {
        EXPECT_THAT(listElement.m_value, Eq(cnt));
        ++cnt;
        if (TESTLISTCAPACITY - 1L == cnt)
        {
            // for the last element (insert) check for different m_value
            cnt = DEFAULT_VALUE;
        }
    }
}

TEST_F(list_test, IteratorArrowOperator)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    ASSERT_THAT(TESTLISTCAPACITY, Ge(10U)); // for the rest of magic numbers to fit

    // fill half
    for (uint64_t i = 0U; i < 5U; ++i)
    {
        sut.emplace_front(i);
        EXPECT_THAT(sut.size(), Eq(i + 1U));
    }

    auto iter = sut.cbegin();
    // 2 increments
    for (uint64_t i = 0U; i < 2U; ++i)
    {
        ++iter;
    }
    sut.insert(iter, DEFAULT_VALUE);

    ASSERT_THAT(sut.size(), Eq(6U));
    ASSERT_THAT(cTor, Eq(0U));
    ASSERT_THAT(customCTor, Eq(6U));

    iter = sut.cbegin();
    EXPECT_THAT(iter->m_value, Eq(4U));
    EXPECT_THAT((++iter)->m_value, Eq(3U));
    EXPECT_THAT((++iter)->m_value, Eq(DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(2U));
    EXPECT_THAT((++iter)->m_value, Eq(1U));
    EXPECT_THAT((++iter)->m_value, Eq(0U));
}

TEST_F(list_test, IteratorIncrementOperatorBeyondEnd)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    sut.push_front(DEFAULT_VALUE);

    auto iter = sut.begin();
    EXPECT_THAT(iter->m_value, Eq(DEFAULT_VALUE));
    EXPECT_TRUE((++iter) == sut.cend());
    EXPECT_TRUE((++iter) == sut.cend());
}

TEST_F(list_test, ConstIteratorIncrementOperatorBeyondEnd)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    sut.push_front(DEFAULT_VALUE);

    auto iter = sut.cbegin();
    EXPECT_THAT(iter->m_value, Eq(DEFAULT_VALUE));
    EXPECT_TRUE((++iter) == sut.cend());
    EXPECT_TRUE((++iter) == sut.cend());
}

TEST_F(list_test, IteratorDecrementOperatorBeyondBegin)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    sut.push_front(DEFAULT_VALUE);

    auto iter = sut.end();
    EXPECT_THAT((--iter)->m_value, Eq(DEFAULT_VALUE));
    EXPECT_TRUE((--iter) == sut.cbegin());
    EXPECT_TRUE((--iter) == sut.cbegin());
}

TEST_F(list_test, ConstIteratorDecrementOperatorBeyondBegin)
{
    constexpr int64_t DEFAULT_VALUE{13L};

    sut.push_front(DEFAULT_VALUE);

    auto iter = sut.cend();
    EXPECT_THAT((--iter)->m_value, Eq(DEFAULT_VALUE));
    EXPECT_TRUE((--iter) == sut.cbegin());
    EXPECT_TRUE((--iter) == sut.cbegin());
}

TEST_F(list_test, IteratorDecrementOperatorBeyondBeginWithFullList)
{
    for (uint64_t i = 0U; i < sut.capacity(); ++i)
    {
        sut.emplace_front(i);
    }

    auto iter = sut.end();
    for (uint64_t i = 0U; i < sut.capacity(); ++i)
    {
        EXPECT_THAT((--iter)->m_value, Eq(i));
    }
    EXPECT_TRUE((iter) == sut.cbegin());
    EXPECT_TRUE((--iter) == sut.cbegin());
}


TEST_F(list_test, IteratorComparisonOfDifferentLists)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(15842U);
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    sut12.emplace_front(1313U);
    sut12.emplace_front(13131U);


    auto iterSut1 = sut11.begin();
    auto iterSut2 = sut12.begin();
    EXPECT_DEATH(dummyFunc(iterSut1 == iterSut2), "");

    iterSut1 = sut11.begin();
    iterSut2 = sut12.begin();
    EXPECT_DEATH(dummyFunc(iterSut1 == iterSut2), "");

    iterSut1 = sut11.end();
    iterSut2 = sut12.end();
    EXPECT_DEATH(dummyFunc(iterSut1 == iterSut2), "");

    iterSut1 = sut11.begin();
    iterSut2 = sut12.begin();
    EXPECT_DEATH(dummyFunc(iterSut1 != iterSut2), "");

    iterSut1 = sut11.begin();
    iterSut2 = sut12.begin();
    EXPECT_DEATH(dummyFunc(iterSut1 != iterSut2), "");

    iterSut1 = sut11.end();
    iterSut2 = sut12.end();
    EXPECT_DEATH(dummyFunc(iterSut1 != iterSut2), "");
}


TEST_F(list_test, ComparingConstIteratorAndIterator)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(15842U);
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    sut12.emplace_front(1313U);
    sut12.emplace_front(13131U);


    list<TestListElement, TESTLISTCAPACITY>::const_iterator iterSut1 = sut11.cbegin();
    list<TestListElement, TESTLISTCAPACITY>::const_iterator iterSut2 = sut11.cend();
    list<TestListElement, TESTLISTCAPACITY>::iterator iterSut3 = sut11.begin();
    list<TestListElement, TESTLISTCAPACITY>::iterator iterSut4 = sut11.end();

    ASSERT_THAT(iterSut1 == iterSut3, Eq(true));
    ASSERT_THAT(iterSut3 == iterSut1, Eq(true));

    ASSERT_THAT(iterSut1 == iterSut2, Eq(false));
    ASSERT_THAT(iterSut4 == iterSut3, Eq(false));
}


TEST_F(list_test, IteratorTraitsGetValueType)
{
    list<int, 10U> sut1;

    sut1.emplace_front(5U);
    auto iter{sut1.begin()};

    // using a function call here is closer to the actual use case (-> intentionally did not inline all code here)
    auto ret = iteratorTraitReturnDoubleValue(iter);

    EXPECT_THAT(ret, Eq(10U));
}

TEST_F(list_test, IteratorTraitsCheckIteratorCategoryOnConstIterator)
{
    auto iter = sut.cbegin();
    ASSERT_NE(typeid(std::iterator_traits<decltype(iter)>::iterator_category), typeid(std::random_access_iterator_tag));
    EXPECT_EQ(typeid(std::iterator_traits<decltype(iter)>::iterator_category), typeid(std::bidirectional_iterator_tag));
}

TEST_F(list_test, EmptyAfterClear)
{
    sut.emplace_front(5U);
    sut.clear();
    EXPECT_THAT(sut.empty(), Eq(true));
}

TEST_F(list_test, SizeZeroAfterClear)
{
    sut.emplace_front(5U);
    sut.clear();
    EXPECT_THAT(sut.size(), Eq(0U));
}

TEST_F(list_test, CopyConstructor)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    sut11.emplace_front(101U);
    sut11.emplace_front(102);
    EXPECT_THAT(customCTor, Eq(2U));

    list<TestListElement, TESTLISTCAPACITY> sut12(sut11);

    EXPECT_THAT(customCTor, Eq(2U));
    EXPECT_THAT(copyCTor, Eq(2U));
    EXPECT_THAT(moveCTor, Eq(0U));
    EXPECT_THAT(moveAssignment, Eq(0U));
    EXPECT_THAT(copyAssignment, Eq(0U));
    auto iter = sut12.begin();
    EXPECT_THAT(iter->m_value, Eq(102));
    EXPECT_THAT((++iter)->m_value, Eq(101U));
    EXPECT_THAT(sut12.empty(), Eq(false));
    EXPECT_THAT(sut12.size(), Eq(2U));
}

TEST_F(list_test, CopyConstructorWithEmptyList)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    list<TestListElement, TESTLISTCAPACITY> sut12(sut11);
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(sut12.size(), Eq(0U));
    EXPECT_THAT(sut12.empty(), Eq(true));
}

TEST_F(list_test, CopyConstructorWithFullList)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    decltype(TestListElement::m_value) i = 0U;

    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut11.emplace_front(i);
    }

    list<TestListElement, TESTLISTCAPACITY> sut12(sut11);
    for (auto& listElement : sut12)
    {
        listElement.m_value = i;
        ++i;
    }

    EXPECT_THAT(copyCTor, Eq(TESTLISTCAPACITY));
    EXPECT_THAT(i, Eq(TESTLISTCAPACITY));
    EXPECT_THAT(sut12.size(), Eq(TESTLISTCAPACITY));
    EXPECT_THAT(sut12.empty(), Eq(false));
}

TEST_F(list_test, MoveConstructor)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    sut11.emplace_front(8101U);
    sut11.emplace_front(8102);

    list<TestListElement, TESTLISTCAPACITY> sut12(std::move(sut11));

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(2U));
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(moveCTor, Eq(2U));
    EXPECT_THAT(copyAssignment, Eq(0U));
    EXPECT_THAT(moveAssignment, Eq(0U));
    EXPECT_THAT(dTor, Eq(2U));
    auto iter = sut12.begin();
    EXPECT_THAT(iter->m_value, Eq(8102));
    EXPECT_THAT((++iter)->m_value, Eq(8101U));
    EXPECT_THAT(sut12.empty(), Eq(false));
    EXPECT_THAT(sut12.size(), Eq(2U));
    EXPECT_THAT(sut11.empty(), Eq(true));
}

TEST_F(list_test, MoveConstructorWithEmptyList)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    list<TestListElement, TESTLISTCAPACITY> sut12(sut11);
    EXPECT_THAT(moveCTor, Eq(0U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT(sut12.size(), Eq(0U));
    EXPECT_THAT(sut12.empty(), Eq(true));
}

TEST_F(list_test, MoveConstructorWithFullList)
{
    list<TestListElement, TESTLISTCAPACITY> sut11;
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        sut11.emplace_front(i);
    }

    list<TestListElement, TESTLISTCAPACITY> sut12(std::move(sut11));

    EXPECT_THAT(moveCTor, Eq(TESTLISTCAPACITY));
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(TESTLISTCAPACITY));
    EXPECT_THAT(sut12.size(), Eq(TESTLISTCAPACITY));
    EXPECT_THAT(sut12.empty(), Eq(false));
}

TEST_F(list_test, DestructorWithEmptyList)
{
    {
        list<TestListElement, TESTLISTCAPACITY> sut11;
    }
    EXPECT_THAT(dTor, Eq(0U));
}

TEST_F(list_test, DestructorSomeElements)
{
    {
        list<TestListElement, TESTLISTCAPACITY> sut11;
        sut11.emplace_front(891U);
        sut11.emplace_front(9191U);
        sut11.emplace_front(1U);
    }
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(3U));
    EXPECT_THAT(dTor, Eq(3U));
}

TEST_F(list_test, DestructorWithFullList)
{
    {
        list<TestListElement, TESTLISTCAPACITY> sut11;
        for (uint64_t i = 0U; i < sut11.capacity(); ++i)
        {
            sut11.emplace_front(1231U);
        }
    }

    EXPECT_THAT(dTor, Eq(TESTLISTCAPACITY));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(TESTLISTCAPACITY));
}

TEST_F(list_test, CopyAssignmentWithEmptySource)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(812U);
    sut11.emplace_front(81122U);
    sut11.emplace_front(8132U);

    sut11 = sut12;
    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(copyAssignment, Eq(0U));
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(3U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(sut11.empty(), Eq(true));
}

TEST_F(list_test, CopyAssignmentWithEmptyDestination)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(5812U);
    sut11.emplace_front(581122U);
    sut11.emplace_front(58132U);

    sut12 = sut11;
    EXPECT_THAT(dTor, Eq(0U));
    EXPECT_THAT(copyAssignment, Eq(0U));
    EXPECT_THAT(copyCTor, Eq(3U));
    EXPECT_THAT(sut12.size(), Eq(3U));
    EXPECT_THAT(sut12.empty(), Eq(false));

    auto iter = sut12.cbegin();
    EXPECT_THAT(iter->m_value, Eq(58132U));
    EXPECT_THAT((++iter)->m_value, Eq(581122U));
    EXPECT_THAT((++iter)->m_value, Eq(5812U));
}


TEST_F(list_test, CopyAssignmentWithLargerDestination)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(5842U);
    sut11.emplace_front(584122U);
    sut11.emplace_front(58432U);
    sut11.emplace_front(58432U);

    sut12.emplace_front(313U);
    sut12.emplace_front(3131U);

    sut11 = sut12;

    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(copyAssignment, Eq(2U));
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(sut11.size(), Eq(2U));
    EXPECT_THAT(sut11.empty(), Eq(false));

    auto iter = sut11.cbegin();
    EXPECT_THAT(iter->m_value, Eq(3131U));
    EXPECT_THAT((++iter)->m_value, Eq(313U));
}

TEST_F(list_test, CopyAssignmentWithLargerSource)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(15842U);
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    sut12.emplace_front(1313U);
    sut12.emplace_front(13131U);

    sut12 = sut11;

    EXPECT_THAT(dTor, Eq(0U));
    EXPECT_THAT(copyAssignment, Eq(2U));
    EXPECT_THAT(copyCTor, Eq(2U));
    EXPECT_THAT(sut12.size(), Eq(4U));
    EXPECT_THAT(sut12.empty(), Eq(false));

    auto iter = sut12.cbegin();
    EXPECT_THAT(iter->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}


TEST_F(list_test, MoveAssignmentWithEmptySource)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(812U);
    sut11.emplace_front(81122U);
    sut11.emplace_front(8132U);

    sut11 = std::move(sut12);

    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(moveAssignment, Eq(0U));
    EXPECT_THAT(moveCTor, Eq(0U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(sut11.empty(), Eq(true));
}

TEST_F(list_test, MoveAssignmentWithEmptyDestination)
{
    list<TestListElement, TESTLISTCAPACITY> sut11, sut12;
    sut11.emplace_front(5812U);
    sut11.emplace_front(581122U);
    sut11.emplace_front(58132U);

    sut12 = std::move(sut11);

    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(moveAssignment, Eq(0U));
    EXPECT_THAT(copyCTor, Eq(0U));
    EXPECT_THAT(moveCTor, Eq(3U));
    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(3U));

    EXPECT_THAT(sut12.size(), Eq(3U));
    EXPECT_THAT(sut12.empty(), Eq(false));

    auto iter = sut12.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(58132U));
    EXPECT_THAT((++iter)->m_value, Eq(581122U));
    EXPECT_THAT((++iter)->m_value, Eq(5812U));
}


TEST_F(list_test, MoveAssignmentWithLargerDestination)
{
    list<TestListElement, 10U> sut11, sut12;
    sut11.emplace_front(5842U);
    sut11.emplace_front(584122U);
    sut11.emplace_front(58432U);
    sut11.emplace_front(58432U);

    sut12.emplace_front(313U);
    sut12.emplace_front(3131U);

    sut11 = std::move(sut12);

    EXPECT_THAT(dTor, Eq(4U));
    EXPECT_THAT(moveAssignment, Eq(2U));
    EXPECT_THAT(moveCTor, Eq(0U));
    EXPECT_THAT(sut11.size(), Eq(2U));
    EXPECT_THAT(sut11.empty(), Eq(false));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(3131U));
    EXPECT_THAT((++iter)->m_value, Eq(313U));
}

TEST_F(list_test, MoveAssignmentWithLargerSource)
{
    list<TestListElement, 10U> sut11, sut12;
    sut11.emplace_front(15842U);
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    sut12.emplace_front(1313U);
    sut12.emplace_front(13131U);

    sut12 = std::move(sut11);

    EXPECT_THAT(dTor, Eq(4U));
    EXPECT_THAT(moveAssignment, Eq(2U));
    EXPECT_THAT(moveCTor, Eq(2U));
    EXPECT_THAT(sut12.size(), Eq(4U));
    EXPECT_THAT(sut12.empty(), Eq(false));


    auto iter = sut12.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}

TEST_F(list_test, RemoveDefaultElementFromEmptyList)
{
    auto cnt = sut.remove({});

    EXPECT_THAT(cTor, Eq(1U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT(dTor, Eq(1U));
    EXPECT_THAT(sut.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(0U));
}
TEST_F(list_test, RemoveCustomElementFromEmptyList)
{
    auto cnt = sut.remove({10U});

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT(dTor, Eq(1U));
    EXPECT_THAT(sut.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(0U));
}
TEST_F(list_test, RemoveOneDefaultElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt = sut11.remove({});

    EXPECT_THAT(cTor, Eq(3U));
    EXPECT_THAT(customCTor, Eq(4U));
    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(sut11.size(), Eq(4U));
    EXPECT_THAT(cnt, Eq(2U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}
TEST_F(list_test, RemoveOneCustomElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt = sut11.remove({1584122U});

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(5U));
    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(sut11.size(), Eq(5U));
    EXPECT_THAT(cnt, Eq(1U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}
TEST_F(list_test, RemoveNotExistentElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt = sut11.remove({1243U});

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(5U));
    EXPECT_THAT(dTor, Eq(1U));
    EXPECT_THAT(classValue, Eq(1243U));
    EXPECT_THAT(sut11.size(), Eq(6U));
    EXPECT_THAT(cnt, Eq(0U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}

TEST_F(list_test, RemoveOnetoEmptyList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);

    auto cnt = sut11.remove({15842U});

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(2U));
    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(1U));
}

TEST_F(list_test, RemoveWithFewMatches)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt = sut11.remove({});

    EXPECT_THAT(cTor, Eq(3U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(sut11.size(), Eq(1U));
    EXPECT_THAT(cnt, Eq(2U));
}

TEST_F(list_test, RemoveWithAllMatches)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt = sut11.remove({});

    EXPECT_THAT(cTor, Eq(3U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(2U));
}

TEST_F(list_test, RemoveAllFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt = sut11.remove({15842U});
    cnt += sut11.remove({});

    EXPECT_THAT(cTor, Eq(3U));
    EXPECT_THAT(customCTor, Eq(2U));
    EXPECT_THAT(dTor, Eq(5U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(3U));
}


TEST_F(list_test, RemoveIfFromEmptyList)
{
    auto cnt = sut.remove_if([](const TestListElement&) { return true; });

    EXPECT_THAT(isSetupState(), Eq(true));
    EXPECT_THAT(sut.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(0U));
}


TEST_F(list_test, RemoveIfOneDefaultElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt =
        sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == TEST_LIST_ELEMENT_DEFAULT_VALUE; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(4U));
    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(classValue, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT(sut11.size(), Eq(4U));
    EXPECT_THAT(cnt, Eq(2U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}

TEST_F(list_test, RemoveIfOneCustomElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt = sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == 1584122U; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(4U));
    EXPECT_THAT(dTor, Eq(1U));
    EXPECT_THAT(sut11.size(), Eq(5U));
    EXPECT_THAT(cnt, Eq(1U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}

TEST_F(list_test, RemoveIfNotExistentElementFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();
    sut11.emplace_front(1584122U);
    sut11.emplace_front(158432U);
    sut11.emplace_front(158432U);

    auto cnt = sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == 1234U; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(4U));
    EXPECT_THAT(dTor, Eq(0U));
    EXPECT_THAT(sut11.size(), Eq(6U));
    EXPECT_THAT(cnt, Eq(0U));

    auto iter = sut11.cbegin();
    EXPECT_THAT((iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(158432U));
    EXPECT_THAT((++iter)->m_value, Eq(1584122U));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(TEST_LIST_ELEMENT_DEFAULT_VALUE));
    EXPECT_THAT((++iter)->m_value, Eq(15842U));
}

TEST_F(list_test, RemoveIfOnetoEmptyList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);

    auto cnt = sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == 15842U; });

    EXPECT_THAT(cTor, Eq(0U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT(dTor, Eq(1U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(1U));
}

TEST_F(list_test, RemoveIfWithFewMatches)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt =
        sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == TEST_LIST_ELEMENT_DEFAULT_VALUE; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(sut11.size(), Eq(1U));
    EXPECT_THAT(cnt, Eq(2U));
}

TEST_F(list_test, RemoveIfWithAllMatches)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt =
        sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == TEST_LIST_ELEMENT_DEFAULT_VALUE; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(0U));
    EXPECT_THAT(dTor, Eq(2U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(2U));
}

TEST_F(list_test, RemoveIfAllFromList)
{
    list<TestListElement, 10U> sut11;
    sut11.emplace_front(15842U);
    sut11.emplace_front();
    sut11.emplace_front();

    auto cnt = sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == 15842U; });
    cnt += sut11.remove_if([](const TestListElement& sut1) { return sut1.m_value == TEST_LIST_ELEMENT_DEFAULT_VALUE; });

    EXPECT_THAT(cTor, Eq(2U));
    EXPECT_THAT(customCTor, Eq(1U));
    EXPECT_THAT(dTor, Eq(3U));
    EXPECT_THAT(sut11.size(), Eq(0U));
    EXPECT_THAT(cnt, Eq(3U));
}

TEST_F(list_test, writeContentViaDereferencedIterator)
{
    constexpr uint64_t TEST_VALUE{356U};
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_front(j);
    }

    auto sut1{sut};
    auto iter = sut1.begin();
    TestListElement element{TEST_VALUE};
    *iter = element;
    EXPECT_THAT(sut1.front().m_value, Eq(TEST_VALUE));
}

TEST_F(list_test, invalidIteratorErase)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    sut.erase(iter);

    EXPECT_DEATH(sut.erase(iter), "");
}

TEST_F(list_test, invalidIteratorIncrement)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    sut.erase(iter);

    EXPECT_DEATH(++iter, "");
}

TEST_F(list_test, invalidIteratorDecrement)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    sut.erase(iter);

    EXPECT_DEATH(--iter, "");
}

TEST_F(list_test, invalidIteratorComparison)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    auto iter2 IOX_MAYBE_UNUSED = sut.erase(iter);

    EXPECT_DEATH(dummyFunc(sut.cbegin() == iter), "");
}

TEST_F(list_test, invalidIteratorComparisonUnequal)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    auto iter2 = sut.erase(iter);

    EXPECT_DEATH(dummyFunc(iter2 != iter), "");
}

TEST_F(list_test, invalidIteratorDereferencing)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    auto iter2 IOX_MAYBE_UNUSED = sut.erase(iter);

    EXPECT_DEATH(dummyFunc((*iter).m_value), "");
}

TEST_F(list_test, invalidIteratorAddressOfOperator)
{
    for (uint64_t i = 0U; i < TESTLISTCAPACITY; ++i)
    {
        const uint64_t j{i};
        sut.emplace_back(j);
    }

    auto iter = sut.cbegin();
    ++iter;
    auto iter2 IOX_MAYBE_UNUSED = sut.erase(iter);

    EXPECT_DEATH(dummyFunc(iter->m_value == 12U), "");
}

TEST_F(list_test, ListIsCopyableViaMemcpy)
{
    uint64_t i = 0U;
    using TestFwdList = list<TestListElement, TESTLISTCAPACITY>;
    alignas(TestFwdList) uint8_t otherSutBuffer[sizeof(TestFwdList)];
    uint8_t* otherSutPtr = otherSutBuffer;

    {
        TestFwdList sut1;

        for (; i < TESTLISTCAPACITY; ++i)
        {
            const uint64_t j{i};
            sut1.emplace_front(j);
        }

        memcpy(reinterpret_cast<void*>(otherSutPtr), reinterpret_cast<const void*>(&sut1), sizeof(sut1));

        // overwrite copied-from list before it's being destroyed
        sut1.clear();
        for (uint64_t k = 0U; k < TESTLISTCAPACITY; ++k)
        {
            const uint64_t j{k + i};
            sut1.emplace_front(j);
        }
    }

    for (auto& listElement : *reinterpret_cast<TestFwdList*>(otherSutPtr))
    {
        --i;
        EXPECT_THAT(listElement.m_value, Eq(i));
    }
}
