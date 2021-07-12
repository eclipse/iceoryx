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

#include "iceoryx_posh/internal/popo/ports/client_port_roudi.hpp"
#include "iceoryx_hoofs/cxx/helplets.hpp"

namespace iox
{
namespace popo
{
ClientPortRouDi::ClientPortRouDi(MemberType_t& clientPortData) noexcept
    : BasePort(&clientPortData)
    , m_chunkSender(&getMembers()->m_chunkSenderData)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)

{
}

const ClientPortRouDi::MemberType_t* ClientPortRouDi::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

ClientPortRouDi::MemberType_t* ClientPortRouDi::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

QueueFullPolicy2 ClientPortRouDi::getResponseQueueFullPolicy() const noexcept
{
    return static_cast<QueueFullPolicy2>(getMembers()->m_chunkReceiverData.m_queueFullPolicy);
}

cxx::optional<capro::CaproMessage> ClientPortRouDi::tryGetCaProMessage() noexcept
{
    // get subscribe request from user side
    const auto currentConnectRequest = getMembers()->m_connectRequested.load(std::memory_order_relaxed);

    const auto currentConnectionState = getMembers()->m_connectionState.load(std::memory_order_relaxed);

    switch (currentConnectionState)
    {
    case ConnectionState::NOT_CONNECTED:
        if (currentConnectRequest)
        {
            capro::CaproMessage caproMessage(capro::CaproMessageType::CONNECT,
                                             BasePort::getMembers()->m_serviceDescription);
            return dispatchCaProMessageAndGetPossibleResponse(caproMessage);
        }
        break;
    case ConnectionState::WAIT_FOR_OFFER:
        IOX_FALLTHROUGH;
    case ConnectionState::CONNECTED:
        if (!currentConnectRequest)
        {
            capro::CaproMessage caproMessage(capro::CaproMessageType::DISCONNECT,
                                             BasePort::getMembers()->m_serviceDescription);
            return dispatchCaProMessageAndGetPossibleResponse(caproMessage);
        }
        break;
    default:
        // do nothing
        break;
    }

    // nothing to change
    return cxx::nullopt_t();
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::dispatchCaProMessageAndGetPossibleResponse(const capro::CaproMessage& caProMessage) noexcept
{
    const auto currentConnectionState = getMembers()->m_connectionState.load(std::memory_order_relaxed);

    switch (currentConnectionState)
    {
    case ConnectionState::NOT_CONNECTED:
        return handleCaProMessageForStateNotConnected(caProMessage);
    case ConnectionState::CONNECT_REQUESTED:
        return handleCaProMessageForStateConnectRequested(caProMessage);
    case ConnectionState::WAIT_FOR_OFFER:
        return handleCaProMessageForStateWaitForOffer(caProMessage);
    case ConnectionState::CONNECTED:
        return handleCaProMessageForStateConnected(caProMessage);
    case ConnectionState::DISCONNECT_REQUESTED:
        return handleCaProMessageForStateDisconnectRequesteded(caProMessage);
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

void ClientPortRouDi::handleCaProProtocollViolation(iox::capro::CaproMessageType messageType) noexcept
{
    // this shouldn't be reached
    LogFatal() << "CaPro Protocol Violation! Got '" << messageType << "' in `"
               << getMembers()->m_connectionState.load(std::memory_order_relaxed) << "'";
    errorHandler(Error::kPOPO__CAPRO_PROTOCOL_ERROR, nullptr, ErrorLevel::SEVERE);
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::handleCaProMessageForStateNotConnected(const capro::CaproMessage& caProMessage) noexcept
{
    switch (caProMessage.m_type)
    {
    case capro::CaproMessageType::CONNECT:
    {
        getMembers()->m_connectionState.store(ConnectionState::CONNECT_REQUESTED, std::memory_order_relaxed);

        capro::CaproMessage caproMessage(capro::CaproMessageType::CONNECT,
                                         BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);
        caproMessage.m_historyCapacity = 0;

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    case capro::CaproMessageType::OFFER:
        return cxx::nullopt_t();
    default:
        break;
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::handleCaProMessageForStateConnectRequested(const capro::CaproMessage& caProMessage) noexcept
{
    switch (caProMessage.m_type)
    {
    case capro::CaproMessageType::ACK:
        cxx::Expects(caProMessage.m_chunkQueueData != nullptr && "Invalid request queue passed to client");
        cxx::Expects(!m_chunkSender
                          .tryAddQueue(static_cast<ServerChunkQueueData_t*>(caProMessage.m_chunkQueueData),
                                       caProMessage.m_historyCapacity)
                          .has_error());

        getMembers()->m_connectionState.store(ConnectionState::CONNECTED, std::memory_order_relaxed);
        return cxx::nullopt_t();
    case capro::CaproMessageType::NACK:
        getMembers()->m_connectionState.store(ConnectionState::WAIT_FOR_OFFER, std::memory_order_relaxed);
        return cxx::nullopt_t();
    default:
        break;
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::handleCaProMessageForStateWaitForOffer(const capro::CaproMessage& caProMessage) noexcept
{
    switch (caProMessage.m_type)
    {
    case capro::CaproMessageType::OFFER:
    {
        getMembers()->m_connectionState.store(ConnectionState::CONNECT_REQUESTED, std::memory_order_relaxed);

        capro::CaproMessage caproMessage(capro::CaproMessageType::CONNECT,
                                         BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);
        caproMessage.m_historyCapacity = 0;

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    case capro::CaproMessageType::DISCONNECT:
        getMembers()->m_connectionState.store(ConnectionState::NOT_CONNECTED, std::memory_order_relaxed);
        return cxx::nullopt_t();
    default:
        break;
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::handleCaProMessageForStateConnected(const capro::CaproMessage& caProMessage) noexcept
{
    switch (caProMessage.m_type)
    {
    case capro::CaproMessageType::STOP_OFFER:
        getMembers()->m_connectionState.store(ConnectionState::WAIT_FOR_OFFER, std::memory_order_relaxed);
        m_chunkSender.removeAllQueues();
        return cxx::nullopt_t();
    case capro::CaproMessageType::DISCONNECT:
    {
        getMembers()->m_connectionState.store(ConnectionState::DISCONNECT_REQUESTED, std::memory_order_relaxed);
        m_chunkSender.removeAllQueues();

        capro::CaproMessage caproMessage(capro::CaproMessageType::DISCONNECT,
                                         BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    default:
        break;
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::handleCaProMessageForStateDisconnectRequesteded(const capro::CaproMessage& caProMessage) noexcept
{
    switch (caProMessage.m_type)
    {
    case capro::CaproMessageType::ACK:
        IOX_FALLTHROUGH;
    case capro::CaproMessageType::NACK:
        getMembers()->m_connectionState.store(ConnectionState::NOT_CONNECTED, std::memory_order_relaxed);
        return cxx::nullopt_t();
    default:
        break;
    }

    handleCaProProtocollViolation(caProMessage.m_type);
    return cxx::nullopt;
}

void ClientPortRouDi::releaseAllChunks() noexcept
{
    m_chunkSender.releaseAll();
    m_chunkReceiver.releaseAll();
}

} // namespace popo
} // namespace iox
