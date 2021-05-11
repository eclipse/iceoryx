// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/gateway/channel.hpp"
#include "iceoryx_posh/gateway/gateway_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"

#include "test.hpp"

#include "stubs/stub_gateway_generic.hpp"

using namespace ::testing;
using namespace iox::units::duration_literals;
using ::testing::_;

// ======================================== Helpers ======================================== //

using iox::capro::IdString_t;

// We do not need real channel terminals to test the base class.
struct StubbedIceoryxTerminal
{
    struct Options
    {
    };
    StubbedIceoryxTerminal(const iox::capro::ServiceDescription&, const Options&){};
};

struct StubbedExternalTerminal
{
    StubbedExternalTerminal(IdString_t, IdString_t, IdString_t){};
};

using TestChannel = iox::gw::Channel<StubbedIceoryxTerminal, StubbedExternalTerminal>;
using TestGatewayGeneric = iox::gw::StubbedGatewayGeneric<TestChannel>;

// ======================================== Fixture ======================================== //
class GatewayGenericTest : public Test
{
  public:
    void SetUp(){};
    void TearDown(){};
};

// ======================================== Tests ======================================== //
TEST_F(GatewayGenericTest, AddedChannelsAreStored)
{
    // ===== Setup
    auto testService = iox::capro::ServiceDescription("service", "instance", "event");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testService, StubbedIceoryxTerminal::Options()).has_error());

    EXPECT_EQ(1U, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, DoesNotAddDuplicateChannels)
{
    // ===== Setup
    auto testService = iox::capro::ServiceDescription("service", "instance", "event");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testService, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(testService, StubbedIceoryxTerminal::Options()).has_error());

    EXPECT_EQ(1U, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, IgnoresWildcardServices)
{
    // ===== Setup
    auto completeWildcardService = iox::capro::ServiceDescription(
        iox::capro::AnyServiceString, iox::capro::AnyInstanceString, iox::capro::AnyEventString);
    auto wildcardServiceService = iox::capro::ServiceDescription(iox::capro::AnyServiceString, "instance", "event");
    auto wildcardInstanceService = iox::capro::ServiceDescription("service", iox::capro::AnyInstanceString, "event");
    auto wildcardEventService = iox::capro::ServiceDescription("service", "instance", iox::capro::AnyEventString);

    TestGatewayGeneric gw{};

    // ===== Test
    auto resultOne = gw.addChannel(completeWildcardService, StubbedIceoryxTerminal::Options());
    auto resultTwo = gw.addChannel(wildcardServiceService, StubbedIceoryxTerminal::Options());
    auto resultThree = gw.addChannel(wildcardInstanceService, StubbedIceoryxTerminal::Options());
    auto resultFour = gw.addChannel(wildcardEventService, StubbedIceoryxTerminal::Options());

    EXPECT_EQ(iox::gw::GatewayError::UNSUPPORTED_SERVICE_TYPE, resultOne.get_error());
    EXPECT_EQ(iox::gw::GatewayError::UNSUPPORTED_SERVICE_TYPE, resultTwo.get_error());
    EXPECT_EQ(iox::gw::GatewayError::UNSUPPORTED_SERVICE_TYPE, resultThree.get_error());
    EXPECT_EQ(iox::gw::GatewayError::UNSUPPORTED_SERVICE_TYPE, resultFour.get_error());

    EXPECT_EQ(0U, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, ProperlyManagesMultipleChannels)
{
    // ===== Setup
    auto serviceOne = iox::capro::ServiceDescription("serviceOne", "instanceOne", "eventOne");
    auto serviceTwo = iox::capro::ServiceDescription("serviceTwo", "instanceTwo", "eventTwo");
    auto serviceThree = iox::capro::ServiceDescription("serviceThree", "instanceThree", "eventThree");
    auto serviceFour = iox::capro::ServiceDescription("serviceFour", "instanceFour", "eventFour");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(serviceOne, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(serviceTwo, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(serviceThree, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(serviceFour, StubbedIceoryxTerminal::Options()).has_error());


    EXPECT_EQ(4U, gw.getNumberOfChannels());
    EXPECT_EQ(true, gw.findChannel(serviceOne).has_value());
    EXPECT_EQ(true, gw.findChannel(serviceTwo).has_value());
    EXPECT_EQ(true, gw.findChannel(serviceThree).has_value());
    EXPECT_EQ(true, gw.findChannel(serviceFour).has_value());
}

TEST_F(GatewayGenericTest, HandlesMaxmimumChannelCapacity)
{
    // ===== Setup
    TestGatewayGeneric gw{};

    // ===== Test
    for (auto i = 0U; i < iox::MAX_CHANNEL_NUMBER; i++)
    {
        auto result = gw.addChannel(
            iox::capro::ServiceDescription(iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i)),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i)),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i))),
            StubbedIceoryxTerminal::Options());
        EXPECT_EQ(false, result.has_error());
    }

    EXPECT_EQ(iox::MAX_CHANNEL_NUMBER, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, ThrowsErrorWhenExceedingMaximumChannelCapaicity)
{
    // ===== Setup
    TestGatewayGeneric gw{};

    // ===== Test
    for (auto i = 0U; i < iox::MAX_CHANNEL_NUMBER; i++)
    {
        auto result = gw.addChannel(
            iox::capro::ServiceDescription(iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i)),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i)),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::to_string(i))),
            StubbedIceoryxTerminal::Options());
        EXPECT_EQ(false, result.has_error());
    }

    auto result = gw.addChannel({"oneTooMany", "oneTooMany", "oneTooMany"}, StubbedIceoryxTerminal::Options());
    EXPECT_EQ(true, result.has_error());
    EXPECT_EQ(iox::gw::GatewayError::UNSUCCESSFUL_CHANNEL_CREATION, result.get_error());
}

TEST_F(GatewayGenericTest, ThrowsErrorWhenAttemptingToRemoveNonexistantChannel)
{
    // ===== Setup
    auto testServiceA = iox::capro::ServiceDescription("serviceA", "instanceA", "eventA");
    auto testServiceB = iox::capro::ServiceDescription("serviceB", "instanceB", "eventB");
    auto testServiceC = iox::capro::ServiceDescription("serviceC", "instanceC", "eventC");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testServiceA, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(testServiceB, StubbedIceoryxTerminal::Options()).has_error());
    EXPECT_EQ(2U, gw.getNumberOfChannels());

    auto result = gw.discardChannel(testServiceC);
    EXPECT_EQ(true, result.has_error());
    EXPECT_EQ(2U, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, DiscardedChannelsAreNotStored)
{
    // ===== Setup
    auto testService = iox::capro::ServiceDescription("service", "instance", "event");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testService, StubbedIceoryxTerminal::Options()).has_error());
    EXPECT_EQ(1U, gw.getNumberOfChannels());
    auto result = gw.discardChannel(testService);
    EXPECT_EQ(false, result.has_error());
    EXPECT_EQ(0U, gw.getNumberOfChannels());
}

TEST_F(GatewayGenericTest, FindChannelReturnsCopyOfFoundChannel)
{
    // ===== Setup
    auto testService = iox::capro::ServiceDescription("service", "instance", "event");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testService, StubbedIceoryxTerminal::Options()).has_error());
    auto foundChannel = gw.findChannel(testService);
    EXPECT_EQ(true, foundChannel.has_value());
    if (foundChannel.has_value())
    {
        EXPECT_EQ(testService, foundChannel.value().getServiceDescription());
    }
}

TEST_F(GatewayGenericTest, FindChannelGivesEmptyOptionalIfNoneFound)
{
    // ===== Setup
    auto storedChannelService = iox::capro::ServiceDescription("service", "instance", "event");
    auto notStoredChannelService = iox::capro::ServiceDescription("otherService", "otherInstance", "otherEvent");

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(storedChannelService, StubbedIceoryxTerminal::Options()).has_error());
    auto foundChannel = gw.findChannel(notStoredChannelService);
    EXPECT_EQ(false, foundChannel.has_value());
}

TEST_F(GatewayGenericTest, ForEachChannelExecutesGivenFunctionForAllStoredChannels)
{
    // ===== Setup
    auto testServiceA = iox::capro::ServiceDescription("serviceA", "instanceA", "eventA");
    auto testServiceB = iox::capro::ServiceDescription("serviceB", "instanceB", "eventB");
    auto testServiceC = iox::capro::ServiceDescription("serviceC", "instanceC", "eventC");

    auto count = 0U;
    auto f = [&count](TestChannel&) { count++; };

    TestGatewayGeneric gw{};

    // ===== Test
    ASSERT_FALSE(gw.addChannel(testServiceA, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(testServiceB, StubbedIceoryxTerminal::Options()).has_error());
    ASSERT_FALSE(gw.addChannel(testServiceC, StubbedIceoryxTerminal::Options()).has_error());
    gw.forEachChannel(f);

    EXPECT_EQ(3U, count);
}
