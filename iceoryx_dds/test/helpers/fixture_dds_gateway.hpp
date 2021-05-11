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

#ifndef TEST_HELPERS_FIXTURE_DDS_GATEWAY_H
#define TEST_HELPERS_FIXTURE_DDS_GATEWAY_H

#include "iceoryx_posh/gateway/channel.hpp"

#include "mocks/google_mocks.hpp"
#include "test.hpp"

template <typename IceoryxTerminal, typename DDSTerminal>
class DDSGatewayTestFixture : public Test
{
  public:
    // Holds mocks created by tests to be used by the channel factory.
    std::vector<std::shared_ptr<IceoryxTerminal>> m_stagedMockIceoryxTerminal;
    std::vector<std::shared_ptr<DDSTerminal>> m_stagedMockDDSTerminal;

    // Marks where in the array to look for a valid mock.
    // Indexes lower than the marker are assumed to have been moved out and thus undefined.
    size_t m_mockIceoryxTerminalCursor = 0;
    size_t m_mockDDSTerminalMarker = 0;

    void SetUp(){};
    void TearDown()
    {
        m_stagedMockIceoryxTerminal.clear();
        m_mockIceoryxTerminalCursor = 0;
        m_stagedMockDDSTerminal.clear();
        m_mockDDSTerminalMarker = 0;
    };

    // Create a new dds mock
    std::shared_ptr<DDSTerminal> createMockDDSTerminal(const iox::capro::ServiceDescription& sd)
    {
        return std::shared_ptr<DDSTerminal>(new DDSTerminal(sd));
    }
    // Stage the given mock to be used in the channel factory
    void stageMockDDSTerminal(std::shared_ptr<DDSTerminal>&& mock)
    {
        // Pass ownership - do not hold a reference here.
        m_stagedMockDDSTerminal.emplace_back(std::move(mock));
    };
    // Create a new iceoryx mock
    template <typename IceoryxPubSubOptions>
    std::shared_ptr<IceoryxTerminal> createMockIceoryxTerminal(const iox::capro::ServiceDescription& sd,
                                                               const IceoryxPubSubOptions& options)
    {
        return std::shared_ptr<IceoryxTerminal>(new IceoryxTerminal(sd, options));
    }
    // Stage the given mock to be used in the channel factory
    void stageMockIceoryxTerminal(std::shared_ptr<IceoryxTerminal>&& mock)
    {
        m_stagedMockIceoryxTerminal.emplace_back(std::move(mock));
    };

    // Creates channels to be used in tests.
    // Channels will contain staged mocks, or empty mocks if none are staged.
    // The factory method can be passed to test gateways, allowing injection of mocks.
    template <typename IceoryxPubSubOptions>
    iox::cxx::expected<iox::gw::Channel<IceoryxTerminal, DDSTerminal>, iox::gw::GatewayError>
    channelFactory(iox::capro::ServiceDescription sd, const IceoryxPubSubOptions& options) noexcept
    {
        // Get or create a mock iceoryx terminal
        std::shared_ptr<IceoryxTerminal> mockIceoryxTerminal;
        if (m_mockIceoryxTerminalCursor < m_stagedMockIceoryxTerminal.size())
        {
            // Important - must pass ownership to receiver so object is deleted when the receiver is done with it.
            mockIceoryxTerminal = std::move(m_stagedMockIceoryxTerminal.at(m_mockIceoryxTerminalCursor++));
        }
        else
        {
            mockIceoryxTerminal = createMockIceoryxTerminal(sd, options);
        }

        // Get or create a mock dds terminal
        std::shared_ptr<DDSTerminal> mockDataWriter;
        if (m_mockDDSTerminalMarker < m_stagedMockDDSTerminal.size())
        {
            // Important - must pass ownership to receiver so object is deleted when the receiver is done with it.
            mockDataWriter = std::move(m_stagedMockDDSTerminal.at(m_mockDDSTerminalMarker++));
        }
        else
        {
            mockDataWriter = createMockDDSTerminal(sd);
        }

        return iox::cxx::success<iox::gw::Channel<IceoryxTerminal, DDSTerminal>>(
            iox::gw::Channel<IceoryxTerminal, DDSTerminal>(
                sd, std::move(mockIceoryxTerminal), std::move(mockDataWriter)));
    }
};

#endif // TEST_HELPERS_FIXTURE_DDS_GATEWAY_H
