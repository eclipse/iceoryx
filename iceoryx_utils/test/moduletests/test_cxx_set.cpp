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

#include "test.hpp"
using namespace ::testing;

#include "iceoryx_utils/internal/cxx/set.hpp"

#include <vector>

namespace
{
// non primitive type for container used with set semantics
class Foo
{
  public:
    Foo()
    {
    }

    Foo(int data)
        : m_data(data)
    {
    }

    ~Foo()
    {
    }

    int m_data{0};
};


// operator needed for set semantics (internal check whether object exists)

bool operator==(const Foo& a, const Foo& b)
{
    return a.m_data == b.m_data;
}

using Type = Foo;
using Container = std::vector<Type>; /// @todo use fixed size container instead?
using namespace iox::cxx;

class CxxSet_test : public Test
{
  public:
    void SetUp()
    {
        internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    Container testContainer;
};


TEST_F(CxxSet_test, add)
{
    EXPECT_THAT(testContainer.size(), Eq(0U));
    Foo foo(73);
    set::add(testContainer, foo);
    EXPECT_THAT(testContainer.size(), Eq(1U));

    set::add(testContainer, foo);
    EXPECT_THAT(testContainer.size(), Eq(1U));

    for (auto& element : testContainer)
    {
        EXPECT_THAT(element, Eq(foo));
    }

    set::add(testContainer, Foo(37));
    EXPECT_THAT(testContainer.size(), Eq(2U));

    set::add(testContainer, Foo(37));
    EXPECT_THAT(testContainer.size(), Eq(2U));

    set::add(testContainer, Foo(73));
    EXPECT_THAT(testContainer.size(), Eq(2U));

    // look for both elements in the set in any order (we cannot assume the vector/list behaviour of the container)
    Foo otherFoo(37);
    bool found{false};
    bool foundOther{false};
    for (auto& element : testContainer)
    {
        if (element == foo)
        {
            found = true;
        }
        else if (element == otherFoo)
        {
            foundOther = true;
        }
    }

    EXPECT_THAT(found, Eq(true));
    EXPECT_THAT(foundOther, Eq(true));
}


TEST_F(CxxSet_test, remove)
{
    EXPECT_THAT(testContainer.size(), Eq(0U));
    Foo foo(73);
    Foo otherFoo(37);

    set::add(testContainer, foo);
    set::add(testContainer, otherFoo);

    EXPECT_THAT(testContainer.size(), Eq(2U));

    {
        // look for both elements in the set in any order (we cannot assume the vector/list behaviour of the container)
        bool found{false};
        bool foundOther{false};
        for (auto& element : testContainer)
        {
            if (element == foo)
            {
                found = true;
            }
            else if (element == otherFoo)
            {
                foundOther = true;
            }
        }

        EXPECT_THAT(found, Eq(true));
        EXPECT_THAT(foundOther, Eq(true));
    }

    // test removal of nonexisting element
    {
        set::remove(testContainer, Foo(42));
        EXPECT_THAT(testContainer.size(), Eq(2U));

        // need to check that the container is unchanged
        // look for both elements in the set in any order (we cannot assume the vector/list behaviour of the container)
        bool found{false};
        bool foundOther{false};
        for (auto& element : testContainer)
        {
            if (element == foo)
            {
                found = true;
            }
            else if (element == otherFoo)
            {
                foundOther = true;
            }
        }

        EXPECT_THAT(found, Eq(true));
        EXPECT_THAT(foundOther, Eq(true));
    }

    // test removal of existing element
    {
        set::remove(testContainer, foo);
        EXPECT_THAT(testContainer.size(), Eq(1U));

        // need to check that the container is unchanged
        // look for both elements in the set in any order (we cannot assume the vector/list behaviour of the container)
        bool found{false};
        bool foundOther{false};
        for (auto& element : testContainer)
        {
            if (element == foo)
            {
                found = true;
            }
            else if (element == otherFoo)
            {
                foundOther = true;
            }
        }

        EXPECT_THAT(found, Eq(false));
        EXPECT_THAT(foundOther, Eq(true));
    }

    // test removal of last element
    {
        set::remove(testContainer, otherFoo);
        EXPECT_THAT(testContainer.size(), Eq(0U));

        // need to check that the container is unchanged
        // look for both elements in the set in any order (we cannot assume the vector/list behaviour of the container)
        bool found{false};
        bool foundOther{false};
        for (auto& element : testContainer)
        {
            if (element == foo)
            {
                found = true;
            }
            else if (element == otherFoo)
            {
                foundOther = true;
            }
        }

        EXPECT_THAT(found, Eq(false));
        EXPECT_THAT(foundOther, Eq(false));
    }

    // test removal from empty container
    {
        set::remove(testContainer, foo);
        EXPECT_THAT(testContainer.size(), Eq(0U));
    }
}


TEST_F(CxxSet_test, hasElement)
{
    EXPECT_THAT(testContainer.size(), Eq(0U));

    Foo foo(73);
    Foo otherFoo(37);

    bool found = set::hasElement(testContainer, foo);
    EXPECT_THAT(found, Eq(false));
    found = set::hasElement(testContainer, otherFoo);
    EXPECT_THAT(found, Eq(false));

    set::add(testContainer, foo);
    EXPECT_THAT(testContainer.size(), Eq(1U));

    found = set::hasElement(testContainer, foo);
    EXPECT_THAT(found, Eq(true));
    found = set::hasElement(testContainer, otherFoo);
    EXPECT_THAT(found, Eq(false));

    set::add(testContainer, otherFoo);
    EXPECT_THAT(testContainer.size(), Eq(2U));

    found = set::hasElement(testContainer, foo);
    EXPECT_THAT(found, Eq(true));
    found = set::hasElement(testContainer, otherFoo);
    EXPECT_THAT(found, Eq(true));

    set::remove(testContainer, foo);
    EXPECT_THAT(testContainer.size(), Eq(1U));

    found = set::hasElement(testContainer, foo);
    EXPECT_THAT(found, Eq(false));
    found = set::hasElement(testContainer, otherFoo);
    EXPECT_THAT(found, Eq(true));

    set::remove(testContainer, otherFoo);

    EXPECT_THAT(testContainer.size(), Eq(0U));

    found = set::hasElement(testContainer, foo);
    EXPECT_THAT(found, Eq(false));
    found = set::hasElement(testContainer, otherFoo);
    EXPECT_THAT(found, Eq(false));
}
} // namespace
