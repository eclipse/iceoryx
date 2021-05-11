// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex AI Inc. All rights reserved.
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
#include "test.hpp"

#include <type_traits>

namespace
{
using namespace ::testing;
using namespace iox::cxx;

namespace
{
struct Bar
{
    alignas(8) uint8_t m_dummy[73];
};
struct Foo
{
    uint8_t m_dummy[73];
};
struct FooBar
{
    alignas(32) uint8_t m_dummy[73];
};
struct FuBar
{
    alignas(32) uint8_t m_dummy[73];
};
} // namespace

class Helplets_test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(Helplets_test, maxSize)
{
    EXPECT_THAT(iox::cxx::maxSize<Foo>(), Eq(sizeof(Foo)));

    EXPECT_THAT(sizeof(Bar), Ne(sizeof(Foo)));
    EXPECT_THAT((iox::cxx::maxSize<Bar, Foo>()), Eq(sizeof(Bar)));

    EXPECT_THAT(sizeof(Bar), Ne(sizeof(FooBar)));
    EXPECT_THAT(sizeof(Foo), Ne(sizeof(FooBar)));
    EXPECT_THAT((iox::cxx::maxSize<Bar, Foo, FooBar>()), Eq(sizeof(FooBar)));

    EXPECT_THAT(sizeof(FooBar), Eq(sizeof(FuBar)));
    EXPECT_THAT((iox::cxx::maxSize<FooBar, FuBar>()), Eq(sizeof(FooBar)));
}

TEST_F(Helplets_test, maxAlignment)
{
    EXPECT_THAT(iox::cxx::maxAlignment<Foo>(), Eq(alignof(Foo)));

    EXPECT_THAT(alignof(Bar), Ne(alignof(Foo)));
    EXPECT_THAT((iox::cxx::maxAlignment<Bar, Foo>()), Eq(alignof(Bar)));

    EXPECT_THAT(alignof(Bar), Ne(alignof(FooBar)));
    EXPECT_THAT(alignof(Foo), Ne(alignof(FooBar)));
    EXPECT_THAT((iox::cxx::maxAlignment<Bar, Foo, FooBar>()), Eq(alignof(FooBar)));

    EXPECT_THAT(alignof(FooBar), Eq(alignof(FuBar)));
    EXPECT_THAT((iox::cxx::maxAlignment<FooBar, FuBar>()), Eq(alignof(FooBar)));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint8WhenValueSmaller256)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<123U>, uint8_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint8WhenValueEqualTo255)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<255U>, uint8_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint16WhenValueEqualTo256)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<256U>, uint16_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint16WhenValueBetween256And65535)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<8172U>, uint16_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint16WhenValueEqualTo65535)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<65535U>, uint16_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint32WhenValueEqualTo65536)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<65536U>, uint32_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint32WhenValueBetween2p16And2p32)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<81721U>, uint32_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint32WhenValueEqualTo4294967295)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<4294967295U>, uint32_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint64WhenValueEqualTo4294967296)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<4294967296U>, uint64_t>::value));
}

TEST_F(Helplets_test, bestFittingTypeUsesUint32WhenValueGreater2p32)
{
    EXPECT_TRUE((std::is_same<BestFittingType_t<42949672961U>, uint64_t>::value));
}

template <class T>
class Helplets_test_isPowerOfTwo : public Helplets_test
{
  public:
    using CurrentType = T;

    static constexpr T MAX = std::numeric_limits<T>::max();
    static constexpr T MAX_POWER_OF_TWO = MAX / 2U + 1U;
};

using HelpletsIsPowerOfTwoTypes = Types<uint8_t, uint16_t, uint32_t, uint64_t, size_t>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(Helplets_test_isPowerOfTwo, HelpletsIsPowerOfTwoTypes);
#pragma GCC diagnostic pop

TYPED_TEST(Helplets_test_isPowerOfTwo, OneIsPowerOfTwo)
{
    EXPECT_TRUE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(1)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, TwoIsPowerOfTwo)
{
    EXPECT_TRUE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(2)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, FourIsPowerOfTwo)
{
    EXPECT_TRUE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(4)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, MaxPossiblePowerOfTwoForTypeIsPowerOfTwo)
{
    EXPECT_TRUE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(TestFixture::MAX_POWER_OF_TWO)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, ZeroIsNotPowerOfTwo)
{
    EXPECT_FALSE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(0)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, FourtyTwoIsNotPowerOfTwo)
{
    EXPECT_FALSE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(42)));
}

TYPED_TEST(Helplets_test_isPowerOfTwo, MaxValueForTypeIsNotPowerOfTwo)
{
    EXPECT_FALSE(isPowerOfTwo(static_cast<typename TestFixture::CurrentType>(TestFixture::MAX)));
}
} // namespace
