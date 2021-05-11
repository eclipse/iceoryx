// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "helpers/fixture_dds_gateway.hpp"

#include "iceoryx_dds/dds/data_writer.hpp"
#include "iceoryx_dds/gateway/iox_to_dds.hpp"
#include "iceoryx_posh/gateway/channel.hpp"
#include "iceoryx_posh/gateway/gateway_config.hpp"
#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "mocks/google_mocks.hpp"
#include "test.hpp"

#include <limits>

using namespace ::testing;
using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgPointee;

// ======================================== Helpers ======================================== //
using TestChannel = iox::gw::Channel<MockSubscriber, MockDataWriter>;
using TestGateway =
    iox::dds::Iceoryx2DDSGateway<TestChannel, MockGenericGateway<TestChannel, iox::popo::SubscriberOptions>>;

// ======================================== Fixture ======================================== //
class Iceoryx2DDSGatewayTest : public DDSGatewayTestFixture<MockSubscriber, MockDataWriter>
{
};

// ======================================== Tests ======================================== //
TEST_F(Iceoryx2DDSGatewayTest, ChannelsAreCreatedForConfiguredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    TestGateway gw{};
    EXPECT_CALL(gw, findChannel(_)).WillOnce(Return(iox::cxx::nullopt_t()));
    EXPECT_CALL(gw, addChannel(_, _)).WillOnce(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.loadConfiguration(config);
}

TEST_F(Iceoryx2DDSGatewayTest, ImmediatelySubscribesToDataFromConfiguredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    auto mockSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    EXPECT_CALL(*mockSubscriber, subscribe()).Times(1);
    stageMockIceoryxTerminal(std::move(mockSubscriber));

    TestGateway gw{};
    ON_CALL(gw, findChannel(_)).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(_, _)).WillByDefault(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.loadConfiguration(config);
}

TEST_F(Iceoryx2DDSGatewayTest, ImmediatelyConnectsCreatedDataWritersForConfiguredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    auto mockWriter = createMockDDSTerminal(testService);
    EXPECT_CALL(*mockWriter, connect()).Times(1);
    stageMockDDSTerminal(std::move(mockWriter));

    TestGateway gw{};
    ON_CALL(gw, findChannel(_)).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(_, _)).WillByDefault(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.loadConfiguration(config);
}

TEST_F(Iceoryx2DDSGatewayTest, IgnoresIntrospectionPorts)
{
    // === Setup
    TestGateway gw{};
    auto msg = iox::capro::CaproMessage(iox::capro::CaproMessageType::OFFER,
                                        {"Introspection", iox::capro::AnyInstanceString, iox::capro::AnyEventString});
    msg.m_subType = iox::capro::CaproMessageSubType::EVENT;

    EXPECT_CALL(gw, addChannel(_, _)).Times(0);

    // === Test
    gw.discover(msg);
}

TEST_F(Iceoryx2DDSGatewayTest, IgnoresServiceMessages)
{
    // === Setup
    TestGateway gw{};
    auto msg = iox::capro::CaproMessage(
        iox::capro::CaproMessageType::OFFER,
        {iox::capro::AnyServiceString, iox::capro::AnyInstanceString, iox::capro::AnyEventString});
    msg.m_subType = iox::capro::CaproMessageSubType::SERVICE;

    EXPECT_CALL(gw, addChannel(_, _)).Times(0);

    // === Test
    gw.discover(msg);
}

TEST_F(Iceoryx2DDSGatewayTest, ChannelsAreCreatedForDiscoveredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    TestGateway gw{};
    auto msg = iox::capro::CaproMessage(iox::capro::CaproMessageType::OFFER, testService);
    msg.m_subType = iox::capro::CaproMessageSubType::EVENT;

    EXPECT_CALL(gw, findChannel(_)).WillOnce(Return(iox::cxx::nullopt_t()));
    EXPECT_CALL(gw, addChannel(_, _)).WillOnce(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.discover(msg);
}

TEST_F(Iceoryx2DDSGatewayTest, ImmediatelySubscribesToDataFromDiscoveredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    auto mockSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    EXPECT_CALL(*mockSubscriber, subscribe()).Times(1);
    stageMockIceoryxTerminal(std::move(mockSubscriber));

    TestGateway gw{};
    auto msg = iox::capro::CaproMessage(iox::capro::CaproMessageType::OFFER, testService);
    msg.m_subType = iox::capro::CaproMessageSubType::EVENT;

    // Mock methods of the mock generic dds gateway base class
    ON_CALL(gw, findChannel(_)).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(_, _)).WillByDefault(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.discover(msg);
}

TEST_F(Iceoryx2DDSGatewayTest, ImmediatelyConnectsCreatedDataWritersForDiscoveredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    auto mockWriter = createMockDDSTerminal(testService);
    EXPECT_CALL(*mockWriter, connect()).Times(1);
    stageMockDDSTerminal(std::move(mockWriter));

    TestGateway gw{};
    auto msg = iox::capro::CaproMessage(iox::capro::CaproMessageType::OFFER, testService);
    msg.m_subType = iox::capro::CaproMessageSubType::EVENT;

    // Mock methods of the mock generic dds gateway base class
    ON_CALL(gw, findChannel(_)).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(_, _)).WillByDefault(Return(channelFactory(testService, iox::popo::SubscriberOptions())));

    // === Test
    gw.discover(msg);
}

/// @ todo #376
#if 0
TEST_F(Iceoryx2DDSGatewayTest, ForwardsChunkFromSubscriberToDataWriter)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    // Prepare a mock mempool chunk
    ChunkMock<int> mockChunk{42};

    // Set up subscriber to provide the chunk
    auto mockSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    EXPECT_CALL(*mockSubscriber, hasNewChunks()).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSubscriber, getChunk(_)).WillOnce(DoAll(SetArgPointee<0>(mockChunk.chunkHeader()),
    Return(true))); stageMockIceoryxTerminal(std::move(mockSubscriber));

    // Verify expected write to the data writer
    auto mockWriter = createMockDDSTerminal(testService);
    EXPECT_CALL(*mockWriter,
                write(SafeMatcherCast<uint8_t*>(Pointee(Eq(42))), mockChunk.chunkHeader()->m_payloadSize))
        .Times(1);
    stageMockDDSTerminal(std::move(mockWriter));

    auto testChannel = channelFactory(testService, iox::popo::SubscriberOptions());
    TestGateway gw{};

    // === Test
    gw.forward(testChannel.value());
}

TEST_F(Iceoryx2DDSGatewayTest, IgnoresMemoryChunksWithNoPayload)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    // Prepare a mock mempool chunk
    ChunkMock<int> mockChunk{0};
    mockChunk.chunkHeader()->m_info.m_payloadSize = 0;

    // Set up subscriber to provide the chunk
    auto mockSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    EXPECT_CALL(*mockSubscriber, hasNewChunks()).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSubscriber, getChunk(_)).WillOnce(DoAll(SetArgPointee<0>(mockChunk.chunkHeader()),
    Return(true))); stageMockIceoryxTerminal(std::move(mockSubscriber));

    // Verify expected write to the data writer
    auto mockWriter = createMockDDSTerminal(testService);
    EXPECT_CALL(*mockWriter, write(_, _)).Times(Exactly(0));
    stageMockDDSTerminal(std::move(mockWriter));

    auto testChannel = channelFactory(testService, iox::popo::SubscriberOptions());
    TestGateway gw{};

    // === Test
    gw.forward(testChannel.value());
}

TEST_F(Iceoryx2DDSGatewayTest, ReleasesReferenceToMemoryChunkAfterSend)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    // Prepare a mock mempool chunk
    ChunkMock<int> mockChunk{42};

    // Set up expect sequence of interactions with subscriber and data writer
    auto mockSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    auto mockWriter = createMockDDSTerminal(testService);
    {
        InSequence seq;
        EXPECT_CALL(*mockSubscriber, hasNewChunks()).WillOnce(Return(true));
        EXPECT_CALL(*mockSubscriber, getChunk(_))
            .WillOnce(DoAll(SetArgPointee<0>(mockChunk.chunkHeader()), Return(true)));
        EXPECT_CALL(*mockWriter, write(_, _)).Times(1);
        EXPECT_CALL(*mockSubscriber, releaseChunk(_)).Times(1);
        EXPECT_CALL(*mockSubscriber, hasNewChunks())
            .WillRepeatedly(Return(false)); // No more chunks after the first one
    }

    stageMockIceoryxTerminal(std::move(mockSubscriber));
    stageMockDDSTerminal(std::move(mockWriter));

    auto testChannel = channelFactory(testService, iox::popo::SubscriberOptions());
    TestGateway gw{};

    // === Test
    gw.forward(testChannel.value());
}

TEST_F(Iceoryx2DDSGatewayTest, DestroysCorrespondingSubscriberWhenAPublisherStopsOffering)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    // Subscribers
    auto firstCreatedSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    auto secondCreatedSubscriber = createMockIceoryxTerminal(testService, iox::popo::SubscriberOptions());
    {
        InSequence seq;
        EXPECT_CALL(*firstCreatedSubscriber, subscribe()).Times(1);
        EXPECT_CALL(*secondCreatedSubscriber, subscribe()).Times(1);
    }

    stageMockIceoryxTerminal(std::move(firstCreatedSubscriber));
    stageMockIceoryxTerminal(std::move(secondCreatedSubscriber));

    // Messages
    auto offerMsg = iox::capro::CaproMessage(iox::capro::CaproMessageType::OFFER, testService);
    offerMsg.m_subType = iox::capro::CaproMessageSubType::EVENT;
    auto stopOfferMsg = iox::capro::CaproMessage(iox::capro::CaproMessageType::STOP_OFFER, testService);
    stopOfferMsg.m_subType = iox::capro::CaproMessageSubType::EVENT;

    // Get the test channels here as we need to use them in expectations
    auto testChannelOne = channelFactory(testService, iox::popo::SubscriberOptions());
    auto testChannelTwo = channelFactory(testService, iox::popo::SubscriberOptions());

    TestGateway gw{};
    EXPECT_CALL(gw, findChannel(_))
        .WillOnce(Return(iox::cxx::nullopt_t()))
        .WillOnce(
            Return(iox::cxx::make_optional<iox::gw::Channel<MockSubscriber,
            MockDataWriter>>(testChannelOne.value())))
        .WillOnce(Return(iox::cxx::nullopt_t()));
    EXPECT_CALL(gw, addChannel(_, _)).WillOnce(Return(testChannelOne)).WillOnce(Return(testChannelTwo));
    EXPECT_CALL(gw, discardChannel(_)).WillOnce(Return(iox::cxx::success<>()));

    // === Test
    gw.discover(offerMsg);
    gw.discover(stopOfferMsg); // first subscriber must be deleted here
    gw.discover(offerMsg);
}
#endif
