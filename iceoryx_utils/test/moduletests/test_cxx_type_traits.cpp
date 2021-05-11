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

#include "iceoryx_utils/cxx/type_traits.hpp"
#include "test.hpp"


namespace iox
{
namespace cxx
{
namespace test
{
template <typename, typename = void>
struct has_mytype_as_member : std::false_type
{
};

template <typename T>
struct has_mytype_as_member<T, void_t<typename T::myType>> : std::true_type
{
};
} // namespace test
} // namespace cxx
} // namespace iox

namespace
{
using namespace ::testing;
using namespace iox::cxx;

TEST(TypeTraitsTest, IsInvocableResolvesToTrue)
{
    auto lambda = [](int foo) -> void { foo++; };
    auto sut = is_invocable<decltype(lambda), int>::value;
    EXPECT_TRUE(sut);
}

TEST(TypeTraitsTest, IsInvocableResolvesToFalse)
{
    int beeblebrox{42};
    auto sut = is_invocable<decltype(beeblebrox), void>::value;
    EXPECT_FALSE(sut);
}

TEST(TypeTraitsTest, HasSignatureResolvesToTrue)
{
    auto lambda = [](int foo) -> int { return foo++; };
    auto sut = has_signature<decltype(lambda), int(int)>::value;
    EXPECT_TRUE(sut);
}

TEST(TypeTraitsTest, HasSignatureResolvesToFalse)
{
    auto lambda = [](float foo) -> float { return foo++; };
    auto sut = has_signature<decltype(lambda), void(void)>::value;
    EXPECT_FALSE(sut);
}

TEST(TypeTraitsTest, NoTypeAsMemberIsFalse)
{
    struct Sut
    {
    };

    EXPECT_FALSE(iox::cxx::test::has_mytype_as_member<Sut>::value);
}

TEST(TypeTraitsTest, MyTypeAsMemberIsTrue)
{
    struct Sut
    {
        using myType = int;
    };

    EXPECT_TRUE(iox::cxx::test::has_mytype_as_member<Sut>::value);
}

TEST(TypeTraitsTest, AddConstConditionallyAddsConstIfConditionTypeIsConst)
{
    using SutType = uint8_t;
    using ConditionType = bool;

    using SutTypeResult = iox::cxx::add_const_conditionally<SutType, const ConditionType>::type;

    EXPECT_TRUE(std::is_const<SutTypeResult>::value);
    // EXPECT_TRUE macro is broken when std::is_same is used directly
    auto typesIsNotChanged = std::is_same<SutType, std::remove_const_t<SutTypeResult>>::value;
    EXPECT_TRUE(typesIsNotChanged);
}

TEST(TypeTraitsTest, AddConstConditionallyDoesNotAddsConstIfConditionTypeIsNotConst)
{
    using SutType = uint8_t;
    using ConditionType = bool;

    using SutTypeResult = iox::cxx::add_const_conditionally<SutType, ConditionType>::type;

    EXPECT_FALSE(std::is_const<SutTypeResult>::value);
    // EXPECT_TRUE macro is broken when std::is_same is used directly
    auto typesIsNotChanged = std::is_same<SutType, SutTypeResult>::value;
    EXPECT_TRUE(typesIsNotChanged);
}

TEST(TypeTraitsTest, AddConstConditionallyTypeAliasWorks)
{
    using SutType = uint8_t;
    using ConditionType = bool;

    using SutTypeResult = iox::cxx::add_const_conditionally_t<SutType, const ConditionType>;

    EXPECT_TRUE(std::is_const<SutTypeResult>::value);
}
} // namespace
