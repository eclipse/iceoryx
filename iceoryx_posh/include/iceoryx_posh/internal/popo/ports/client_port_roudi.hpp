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
#ifndef IOX_POSH_POPO_PORTS_CLIENT_PORT_ROUDI_HPP
#define IOX_POSH_POPO_PORTS_CLIENT_PORT_ROUDI_HPP

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/client_port_data.hpp"

namespace iox
{
namespace popo
{
/// @brief The ClientPortRouDi provides the API for accessing a client port from the RouDi middleware daemon side.
/// The client port is divided in the three parts ClientPortData, ClientPortRouDi and ClientPortUser.
/// The ClientPortRouDi provides service discovery functionality that is based on CaPro messages. With this API the
/// dynamic connections between clients and servers ports can be established.
/// @note This class is not thread-safe and must be guarded by a mutex if used in a multithreaded context.
class ClientPortRouDi : public BasePort
{
  public:
    using MemberType_t = ClientPortData;

    /// @brief Creates a ClientPortRouDi from ClientPortData which are shared with ClientPortUser
    /// @param[in] clientPortData to be are accessed by the ClientPortRouDi interface
    explicit ClientPortRouDi(MemberType_t& clientPortData) noexcept;

    ClientPortRouDi(const ClientPortRouDi& other) = delete;
    ClientPortRouDi& operator=(const ClientPortRouDi&) = delete;
    ClientPortRouDi(ClientPortRouDi&& rhs) = default;
    ClientPortRouDi& operator=(ClientPortRouDi&& rhs) = default;
    ~ClientPortRouDi() = default;

    /// @brief Access to the configured responseQueueFullPolicy
    /// @return the configured responseQueueFullPolicy
    QueueFullPolicy2 getResponseQueueFullPolicy() const noexcept;

    /// @brief get an optional CaPro message that requests changes to the desired connection state of the client
    /// @return CaPro message with desired connection state, empty optional if no state change
    cxx::optional<capro::CaproMessage> tryGetCaProMessage() noexcept;

    /// @brief dispatch a CaPro message to the client for processing
    /// @param[in] caProMessage to process
    /// @return CaPro message with an immediate response the provided CaPro message, empty optional if no response
    cxx::optional<capro::CaproMessage>
    dispatchCaProMessageAndGetPossibleResponse(const capro::CaproMessage& caProMessage) noexcept;

    /// @brief cleanup the client and release all the chunks it currently holds
    /// @attention Contract is that user process is no more running when cleanup is called
    void releaseAllChunks() noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    void handleCaProProtocollViolation(iox::capro::CaproMessageType messageType) noexcept;

    cxx::optional<capro::CaproMessage>
    handleCaProMessageForStateNotConnected(const capro::CaproMessage& caProMessage) noexcept;
    cxx::optional<capro::CaproMessage>
    handleCaProMessageForStateConnectRequested(const capro::CaproMessage& caProMessage) noexcept;
    cxx::optional<capro::CaproMessage>
    handleCaProMessageForStateWaitForOffer(const capro::CaproMessage& caProMessage) noexcept;
    cxx::optional<capro::CaproMessage>
    handleCaProMessageForStateConnected(const capro::CaproMessage& caProMessage) noexcept;
    cxx::optional<capro::CaproMessage>
    handleCaProMessageForStateDisconnectRequesteded(const capro::CaproMessage& caProMessage) noexcept;

    ChunkSender<ClientChunkSenderData_t> m_chunkSender;
    ChunkReceiver<ClientChunkReceiverData_t> m_chunkReceiver;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_CLIENT_PORT_ROUDI_HPP
