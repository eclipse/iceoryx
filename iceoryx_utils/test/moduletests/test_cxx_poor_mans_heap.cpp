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

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/poor_mans_heap.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;

enum class Identity : uint32_t
{
    None,
    Bar,
    Foo
};

enum class LuckyNumber : uint32_t
{
    None,
    Bar = 13,
    Foo = 42
};

std::vector<Identity> g_destructionIdentities;

class Interface
{
  public:
    Interface(Identity identity)
        : m_identity(identity)
    {
    }
    virtual ~Interface()
    {
        g_destructionIdentities.push_back(m_identity);
    }

    Identity identity() const
    {
        return m_identity;
    }

    virtual LuckyNumber luckyNumber() const = 0;

  protected:
    Identity m_identity{Identity::None};
};

class Bar : public Interface
{
  public:
    Bar(LuckyNumber luckyNumber)
        : Interface(Identity::Bar)
        , m_luckyNumber(luckyNumber)
    {
    }

    LuckyNumber luckyNumber() const override
    {
        return m_luckyNumber;
    }

  private:
    LuckyNumber m_luckyNumber{LuckyNumber::None};
};

class Foo : public Interface
{
  public:
    Foo()
        : Interface(Identity::Foo)
    {
    }

    LuckyNumber luckyNumber() const override
    {
        return LuckyNumber::Foo;
    }

    // protected instead of private to prevent a unused member warning
  protected:
    alignas(32) uint8_t m_dummy[73];
};

class PoorMansHeap_test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    static constexpr auto MaxSize = iox::cxx::maxSize<Bar, Foo>();
    static constexpr auto MaxAlignment = iox::cxx::maxAlignment<Bar, Foo>();

    using SUT = iox::cxx::PoorMansHeap<Interface, MaxSize, MaxAlignment>;

    SUT m_sut;
};

TEST_F(PoorMansHeap_test, SizeAndAlignment)
{
    constexpr uint32_t BookkeepingSize{MaxAlignment}; // offset of the aligned storage is the MaxAlignment value
    EXPECT_THAT(sizeof(m_sut), Eq(MaxSize + BookkeepingSize));
    EXPECT_THAT(alignof(SUT), Eq(MaxAlignment));
}

TEST_F(PoorMansHeap_test, CTor_default)
{
    EXPECT_THAT(m_sut.hasInstance(), Eq(false));
}

TEST_F(PoorMansHeap_test, CTorDTor_BaseClass)
{
    {
        SUT sut{iox::cxx::PoorMansHeapType<Bar>(), LuckyNumber::Bar};
        ASSERT_THAT(sut.hasInstance(), Eq(true));
        EXPECT_THAT(sut->identity(), Eq(Identity::Bar));
        EXPECT_THAT(sut->luckyNumber(), Eq(LuckyNumber::Bar));

        g_destructionIdentities.clear();
    }

    ASSERT_THAT(g_destructionIdentities.size(), Eq(1u));
    EXPECT_THAT(g_destructionIdentities[0], Eq(Identity::Bar));
}

TEST_F(PoorMansHeap_test, CTorDTor_NonDerived)
{
    {
        iox::cxx::PoorMansHeap<Bar, sizeof(Bar), alignof(Bar)> sut{iox::cxx::PoorMansHeapType<Bar>(), LuckyNumber::Bar};
        ASSERT_THAT(sut.hasInstance(), Eq(true));
        EXPECT_THAT(sut->identity(), Eq(Identity::Bar));
        EXPECT_THAT(sut->luckyNumber(), Eq(LuckyNumber::Bar));

        g_destructionIdentities.clear();
    }

    ASSERT_THAT(g_destructionIdentities.size(), Eq(1u));
    EXPECT_THAT(g_destructionIdentities[0], Eq(Identity::Bar));
}

TEST_F(PoorMansHeap_test, newInstance)
{
    m_sut.newInstance<Foo>();

    ASSERT_THAT(m_sut.hasInstance(), Eq(true));
    EXPECT_THAT(m_sut->identity(), Eq(Identity::Foo));
    EXPECT_THAT(m_sut->luckyNumber(), Eq(LuckyNumber::Foo));
}

TEST_F(PoorMansHeap_test, deleteInstance)
{
    m_sut.newInstance<Bar>(LuckyNumber::Bar);

    g_destructionIdentities.clear();
    m_sut.deleteInstance();
    ASSERT_THAT(g_destructionIdentities.size(), Eq(1u));
    EXPECT_THAT(g_destructionIdentities[0], Eq(Identity::Bar));
    EXPECT_THAT(m_sut.hasInstance(), Eq(false));
}

TEST_F(PoorMansHeap_test, overwriteInstance)
{
    m_sut.newInstance<Bar>(LuckyNumber::Bar);

    g_destructionIdentities.clear();

    m_sut.newInstance<Foo>();

    ASSERT_THAT(g_destructionIdentities.size(), Eq(1u));
    EXPECT_THAT(g_destructionIdentities[0], Eq(Identity::Bar));

    ASSERT_THAT(m_sut.hasInstance(), Eq(true));
    EXPECT_THAT(m_sut->identity(), Eq(Identity::Foo));
    EXPECT_THAT(m_sut->luckyNumber(), Eq(LuckyNumber::Foo));
}

TEST_F(PoorMansHeap_test, instanceAccess)
{
    m_sut.newInstance<Bar>(LuckyNumber::Bar);

    ASSERT_THAT(m_sut.hasInstance(), Eq(true));
    EXPECT_THAT(m_sut->identity(), Eq(Identity::Bar));
    EXPECT_THAT((*m_sut).identity(), Eq(Identity::Bar));
}
} // namespace
