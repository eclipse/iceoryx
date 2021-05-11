// Copyright (c) 2020 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_GW_GATEWAY_GENERIC_INL
#define IOX_POSH_GW_GATEWAY_GENERIC_INL

#include "iceoryx_posh/gateway/gateway_generic.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_utils/internal/file_reader/file_reader.hpp"

// ================================================== Public ================================================== //

namespace iox
{
namespace gw
{
template <typename channel_t, typename gateway_t>
inline GatewayGeneric<channel_t, gateway_t>::~GatewayGeneric() noexcept
{
    shutdown();
}

template <typename channel_t, typename gateway_t>
inline void GatewayGeneric<channel_t, gateway_t>::runMultithreaded() noexcept
{
    m_discoveryThread = std::thread([this] { this->discoveryLoop(); });
    m_forwardingThread = std::thread([this] { this->forwardingLoop(); });
    m_isRunning.store(true, std::memory_order_relaxed);
}

template <typename channel_t, typename gateway_t>
inline void GatewayGeneric<channel_t, gateway_t>::shutdown() noexcept
{
    if (m_isRunning.load(std::memory_order_relaxed))
    {
        m_isRunning.store(false, std::memory_order_relaxed);

        m_discoveryThread.join();
        m_forwardingThread.join();
    }
}

template <typename channel_t, typename gateway_t>
inline uint64_t GatewayGeneric<channel_t, gateway_t>::getNumberOfChannels() const noexcept
{
    return m_channels->size();
}

// ================================================== Protected ================================================== //

template <typename channel_t, typename gateway_t>
inline GatewayGeneric<channel_t, gateway_t>::GatewayGeneric(capro::Interfaces interface,
                                                            units::Duration discoveryPeriod,
                                                            units::Duration forwardingPeriod) noexcept
    : gateway_t(interface)
    , m_discoveryPeriod(discoveryPeriod)
    , m_forwardingPeriod(forwardingPeriod)
{
}

template <typename channel_t, typename gateway_t>
template <typename IceoryxPubSubOptions>
inline cxx::expected<channel_t, GatewayError>
GatewayGeneric<channel_t, gateway_t>::addChannel(const capro::ServiceDescription& service,
                                                 const IceoryxPubSubOptions& options) noexcept
{
    // Filter out wildcard services
    if (service.getServiceID() == capro::AnyService || service.getInstanceID() == capro::AnyInstance
        || service.getEventID() == capro::AnyEvent)
    {
        return cxx::error<GatewayError>(GatewayError::UNSUPPORTED_SERVICE_TYPE);
    }

    // Return existing channel if one for the service already exists, otherwise create a new one
    auto existingChannel = findChannel(service);
    if (existingChannel.has_value())
    {
        return cxx::success<channel_t>(existingChannel.value());
    }
    else
    {
        auto result = channel_t::create(service, options);
        if (result.has_error())
        {
            return cxx::error<GatewayError>(GatewayError::UNSUCCESSFUL_CHANNEL_CREATION);
        }
        else
        {
            auto channel = result.value();
            m_channels->push_back(channel);
            return cxx::success<channel_t>(channel);
        }
    }
}

template <typename channel_t, typename gateway_t>
inline cxx::optional<channel_t>
GatewayGeneric<channel_t, gateway_t>::findChannel(const iox::capro::ServiceDescription& service) const noexcept
{
    auto guardedVector = this->m_channels.getScopeGuard();
    auto channel = std::find_if(guardedVector->begin(), guardedVector->end(), [&service](const channel_t& channel) {
        return channel.getServiceDescription() == service;
    });
    if (channel == guardedVector->end())
    {
        return cxx::nullopt_t();
    }
    else
    {
        return cxx::make_optional<channel_t>(*channel);
    }
}

template <typename channel_t, typename gateway_t>
inline void
GatewayGeneric<channel_t, gateway_t>::forEachChannel(const cxx::function_ref<void(channel_t&)> f) const noexcept
{
    auto guardedVector = m_channels.getScopeGuard();
    for (auto channel = guardedVector->begin(); channel != guardedVector->end(); ++channel)
    {
        f(*channel);
    }
}

template <typename channel_t, typename gateway_t>
inline cxx::expected<GatewayError>
GatewayGeneric<channel_t, gateway_t>::discardChannel(const capro::ServiceDescription& service) noexcept
{
    auto guardedVector = this->m_channels.getScopeGuard();
    auto channel = std::find_if(guardedVector->begin(), guardedVector->end(), [&service](const channel_t& channel) {
        return channel.getServiceDescription() == service;
    });
    if (channel != guardedVector->end())
    {
        guardedVector->erase(channel);
        return cxx::success<void>();
    }
    else
    {
        return cxx::error<GatewayError>(GatewayError::NONEXISTANT_CHANNEL);
    }
}

// ================================================== Private ================================================== //

template <typename channel_t, typename gateway_t>
inline void GatewayGeneric<channel_t, gateway_t>::discoveryLoop() noexcept
{
    while (m_isRunning.load(std::memory_order_relaxed))
    {
        auto startTime = std::chrono::steady_clock::now();
        capro::CaproMessage msg;
        while (this->getCaProMessage(msg))
        {
            discover(msg);
        }
        std::this_thread::sleep_until(startTime + std::chrono::milliseconds(m_discoveryPeriod.toMilliseconds()));
    }
}

template <typename channel_t, typename gateway_t>
inline void GatewayGeneric<channel_t, gateway_t>::forwardingLoop() noexcept
{
    while (m_isRunning.load(std::memory_order_relaxed))
    {
        auto startTime = std::chrono::steady_clock::now();
        forEachChannel([this](channel_t channel) { this->forward(channel); });
        std::this_thread::sleep_until(startTime + std::chrono::milliseconds(m_forwardingPeriod.toMilliseconds()));
    };
}

} // namespace gw
} // namespace iox

#endif // IOX_POSH_GW_GATEWAY_GENERIC_INL
