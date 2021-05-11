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

#include "iceoryx_dds/gateway/dds_to_iox.hpp"
#include "iceoryx_posh/gateway/channel.hpp"

#include "mocks/google_mocks.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::_;

// ======================================== Helpers ======================================== //
using TestChannel = iox::gw::Channel<MockPublisher, MockDataReader>;
using TestGateway =
    iox::dds::DDS2IceoryxGateway<TestChannel, MockGenericGateway<TestChannel, iox::popo::PublisherOptions>>;

// ======================================== Fixture ======================================== //
class DDS2IceoryxGatewayTest : public DDSGatewayTestFixture<MockPublisher, MockDataReader>
{
};

// ======================================== Tests ======================================== //
TEST_F(DDS2IceoryxGatewayTest, ChannelsAreCreatedForConfiguredServices)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});
    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    TestGateway gw{};
    EXPECT_CALL(gw, findChannel).WillOnce(Return(iox::cxx::nullopt_t()));
    EXPECT_CALL(gw, addChannel(testService, _))
        .WillOnce(Return(channelFactory(testService, iox::popo::PublisherOptions())));

    // === Test
    gw.loadConfiguration(config);
}

TEST_F(DDS2IceoryxGatewayTest, ImmediatelyOffersConfiguredPublishers)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    auto mockPublisher = createMockIceoryxTerminal(testService, iox::popo::PublisherOptions());
    EXPECT_CALL(*mockPublisher, offer).Times(1);
    stageMockIceoryxTerminal(std::move(mockPublisher));

    TestGateway gw{};
    ON_CALL(gw, findChannel).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(testService, _))
        .WillByDefault(Return(channelFactory(testService, iox::popo::PublisherOptions())));

    // === Test
    gw.loadConfiguration(config);
}

TEST_F(DDS2IceoryxGatewayTest, ImmediatelyConnectsConfiguredDataReaders)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    iox::config::GatewayConfig config{};
    config.m_configuredServices.push_back(iox::config::GatewayConfig::ServiceEntry{testService});

    auto mockDataReader = createMockDDSTerminal(testService);
    EXPECT_CALL(*mockDataReader, connect).Times(1);
    stageMockDDSTerminal(std::move(mockDataReader));

    TestGateway gw{};
    ON_CALL(gw, findChannel).WillByDefault(Return(iox::cxx::nullopt_t()));
    ON_CALL(gw, addChannel(testService, _))
        .WillByDefault(Return(channelFactory(testService, iox::popo::PublisherOptions())));

    // === Test
    gw.loadConfiguration(config);
}

/// @ todo #376
#if 0
TEST_F(DDS2IceoryxGatewayTest, PublishesMemoryChunksContainingSamplesToNetwork)
{
    // === Setup
    auto testService = iox::capro::ServiceDescription({"Radar", "Front-Right", "Reflections"});

    // Setup data reader to provide a sample
    auto mockDataReader = createMockDDSTerminal(testService);
    auto mockPublisher = createMockIceoryxTerminal(testService);

    ON_CALL(*mockDataReader, peekNextSize)
        .WillByDefault(Return(ByMove(iox::cxx::make_optional<uint64_t>(static_cast<uint64_t>(8u)))));
    ON_CALL(*mockDataReader, takeNext).WillByDefault(Return(ByMove(iox::cxx::success<>())));
    EXPECT_CALL(*mockPublisher, sendChunk).Times(1);

    stageMockDDSTerminal(std::move(mockDataReader));
    stageMockIceoryxTerminal(std::move(mockPublisher));

    TestGateway gw{};

    // === Test
    auto testChannel = channelFactory(testService, iox::popo::PublisherOptions()).value();
    gw.forward(testChannel);
}
#endif
