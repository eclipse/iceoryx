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

#ifndef IOX_DDS_IOX_TO_DDS_INL
#define IOX_DDS_IOX_TO_DDS_INL

#include "iceoryx_dds/dds/dds_config.hpp"
#include "iceoryx_dds/dds/iox_chunk_datagram_header.hpp"
#include "iceoryx_dds/internal/log/logging.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/gateway/gateway_config.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"

#include "iceoryx_dds/gateway/iox_to_dds.hpp"

namespace iox
{
namespace dds
{
// ======================================== Public ======================================== //
template <typename channel_t, typename gateway_t>
inline Iceoryx2DDSGateway<channel_t, gateway_t>::Iceoryx2DDSGateway() noexcept
    : gateway_t(capro::Interfaces::DDS, DISCOVERY_PERIOD, FORWARDING_PERIOD)
{
}

template <typename channel_t, typename gateway_t>
inline void Iceoryx2DDSGateway<channel_t, gateway_t>::loadConfiguration(const config::GatewayConfig& config) noexcept
{
    LogDebug() << "[Iceoryx2DDSGateway] Configuring gateway...";
    for (const auto& service : config.m_configuredServices)
    {
        if (!this->findChannel(service.m_serviceDescription).has_value())
        {
            auto serviceDescription = service.m_serviceDescription;
            LogDebug() << "[DDS2IceoryxGateway] Setting up channel for service: {"
                       << serviceDescription.getServiceIDString() << ", " << serviceDescription.getInstanceIDString()
                       << ", " << serviceDescription.getEventIDString() << "}";
            popo::SubscriberOptions options;
            options.queueCapacity = SUBSCRIBER_CACHE_SIZE;
            IOX_DISCARD_RESULT(setupChannel(serviceDescription, options));
        }
    }
}

template <typename channel_t, typename gateway_t>
inline void Iceoryx2DDSGateway<channel_t, gateway_t>::discover(const capro::CaproMessage& msg) noexcept
{
    LogDebug() << "[Iceoryx2DDSGateway] <CaproMessage> "
               << capro::CaproMessageTypeString[static_cast<uint8_t>(msg.m_type)]
               << " { Service: " << msg.m_serviceDescription.getServiceIDString()
               << ", Instance: " << msg.m_serviceDescription.getInstanceIDString()
               << ", Event: " << msg.m_serviceDescription.getEventIDString() << " }";

    if (msg.m_serviceDescription.getServiceIDString() == capro::IdString_t(roudi::INTROSPECTION_SERVICE_ID))
    {
        return;
    }
    if (msg.m_subType == capro::CaproMessageSubType::SERVICE)
    {
        return;
    }

    switch (msg.m_type)
    {
    case capro::CaproMessageType::OFFER:
    {
        if (!this->findChannel(msg.m_serviceDescription).has_value())
        {
            popo::SubscriberOptions options;
            options.queueCapacity = SUBSCRIBER_CACHE_SIZE;
            IOX_DISCARD_RESULT(setupChannel(msg.m_serviceDescription, options));
        }
        break;
    }
    case capro::CaproMessageType::STOP_OFFER:
    {
        if (this->findChannel(msg.m_serviceDescription).has_value())
        {
            IOX_DISCARD_RESULT(this->discardChannel(msg.m_serviceDescription));
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

template <typename channel_t, typename gateway_t>
inline void Iceoryx2DDSGateway<channel_t, gateway_t>::forward(const channel_t& channel) noexcept
{
    auto subscriber = channel.getIceoryxTerminal();
    while (subscriber->hasData())
    {
        subscriber->take().and_then([&](const void* userPayload) {
            auto dataWriter = channel.getExternalTerminal();
            auto chunkHeader = iox::mepoo::ChunkHeader::fromUserPayload(userPayload);
            iox::dds::IoxChunkDatagramHeader datagramHeader;
            datagramHeader.userHeaderId = chunkHeader->userHeaderId();
            datagramHeader.userHeaderSize = chunkHeader->userHeaderSize();
            datagramHeader.userPayloadSize = chunkHeader->userPayloadSize();
            datagramHeader.userPayloadAlignment = chunkHeader->userPayloadAlignment();
            dataWriter->write(datagramHeader,
                              static_cast<const uint8_t*>(chunkHeader->userHeader()),
                              static_cast<const uint8_t*>(chunkHeader->userPayload()));
            subscriber->release(userPayload);
        });
    }
}

// ======================================== Private ======================================== //

template <typename channel_t, typename gateway_t>
cxx::expected<channel_t, gw::GatewayError>
Iceoryx2DDSGateway<channel_t, gateway_t>::setupChannel(const capro::ServiceDescription& service,
                                                       const popo::SubscriberOptions& subscriberOptions) noexcept
{
    return this->addChannel(service, subscriberOptions).and_then([](auto channel) {
        auto subscriber = channel.getIceoryxTerminal();
        auto dataWriter = channel.getExternalTerminal();
        subscriber->subscribe();
        dataWriter->connect();
    });
}

} // namespace dds
} // namespace iox

#endif // IOX_DDS_IOX_TO_DDS_INL
