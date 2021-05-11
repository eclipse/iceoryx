// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/popo/building_blocks/typed_unique_id.hpp"
#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "test.hpp"


using namespace ::testing;
using namespace iox::popo;
using namespace iox::cxx;

TEST(TypedUniqueId_RouDiId, SettingTheRouDiIdWorks)
{
    uint16_t someId = 1243u;
    iox::popo::internal::setUniqueRouDiId(someId);
    EXPECT_EQ(iox::popo::internal::getUniqueRouDiId(), someId);
    iox::popo::internal::unsetUniqueRouDiId();
}

TEST(TypedUniqueId_RouDiId, SettingTheRouDiIdTwiceFails)
{
    uint16_t someId = 1243u;
    bool errorHandlerCalled = false;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error error IOX_MAYBE_UNUSED,
                              const std::function<void()>,
                              const iox::ErrorLevel) { errorHandlerCalled = true; });

    iox::popo::internal::setUniqueRouDiId(someId);
    EXPECT_FALSE(errorHandlerCalled);
    iox::popo::internal::setUniqueRouDiId(someId);
    EXPECT_TRUE(errorHandlerCalled);
    iox::popo::internal::unsetUniqueRouDiId();
}

TEST(TypedUniqueId_RouDiId, GettingTheRouDiIdWithoutSettingFails)
{
    bool errorHandlerCalled = false;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error error IOX_MAYBE_UNUSED,
                              const std::function<void()>,
                              const iox::ErrorLevel) { errorHandlerCalled = true; });

    iox::popo::internal::getUniqueRouDiId();
    EXPECT_TRUE(errorHandlerCalled);
}

template <typename T>
class TypedUniqueId_test : public Test
{
  protected:
    using UniqueIDType = T;

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};
};

using Implementations = Types<TypedUniqueId<int>, TypedUniqueId<float>>;
/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(TypedUniqueId_test, Implementations);
#pragma GCC diagnostic pop

TYPED_TEST(TypedUniqueId_test, DefaultConstructorIncrementsID)
{
    typename TestFixture::UniqueIDType a, b;
    EXPECT_THAT(static_cast<uint64_t>(a) + 1, Eq(static_cast<uint64_t>(b)));
}

TYPED_TEST(TypedUniqueId_test, CopyConstructorSetsSameID)
{
    typename TestFixture::UniqueIDType a, b(a);
    EXPECT_THAT(static_cast<uint64_t>(a), Eq(static_cast<uint64_t>(b)));
}

TYPED_TEST(TypedUniqueId_test, CopyConstructorAssignmentSetsSameID)
{
    typename TestFixture::UniqueIDType a, b;
    a = b;
    EXPECT_THAT(a, Eq(b));
}

TYPED_TEST(TypedUniqueId_test, MoveConstructorSetsSameID)
{
    typename TestFixture::UniqueIDType a;
    auto id = static_cast<uint64_t>(a);
    decltype(a) b(std::move(a));
    EXPECT_THAT(static_cast<uint64_t>(b), Eq(id));
}

TYPED_TEST(TypedUniqueId_test, MoveAssignmentSetsSameID)
{
    typename TestFixture::UniqueIDType a, b;
    auto id = static_cast<uint64_t>(a);
    b = std::move(a);
    EXPECT_THAT(static_cast<uint64_t>(b), Eq(id));
}

TYPED_TEST(TypedUniqueId_test, SameIDsAreEqual)
{
    typename TestFixture::UniqueIDType a, b(a);
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a <= b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a >= b);
}

TYPED_TEST(TypedUniqueId_test, DifferentIDsAreNotEqual)
{
    typename TestFixture::UniqueIDType a, b;
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a >= b);
}

TYPED_TEST(TypedUniqueId_test, LatestIDIsGreatestID)
{
    typename TestFixture::UniqueIDType a, b;
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a <= b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
}

TYPED_TEST(TypedUniqueId_test, FirstIDIsSmallestID)
{
    typename TestFixture::UniqueIDType a, b;
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b <= a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
}

TYPED_TEST(TypedUniqueId_test, ConversionToUint64)
{
    typename TestFixture::UniqueIDType a, b;
    uint64_t id = static_cast<uint64_t>(a);
    b = a;
    EXPECT_EQ(id, static_cast<uint64_t>(b));
}

TYPED_TEST(TypedUniqueId_test, CreatingAnUniqueIdWithDefaultCTorIsValid)
{
    typename TestFixture::UniqueIDType a;
    EXPECT_TRUE(a.isValid());
}

TYPED_TEST(TypedUniqueId_test, InvalidIdIsInvalid)
{
    typename TestFixture::UniqueIDType a(InvalidId);
    EXPECT_FALSE(a.isValid());
}
