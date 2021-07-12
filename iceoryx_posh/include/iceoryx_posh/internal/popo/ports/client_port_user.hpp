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
#ifndef IOX_POSH_POPO_PORTS_CLIENT_PORT_USER_HPP
#define IOX_POSH_POPO_PORTS_CLIENT_PORT_USER_HPP

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iceoryx_hoofs/cxx/helplets.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/error_handling/error_handling.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/client_port_data.hpp"
#include "iceoryx_posh/popo/rpc_header.hpp"

namespace iox
{
namespace popo
{
/// @brief The ClientPortUser provides the API for accessing a client port from the user side. The client port
/// is divided in the three parts ClientPortData, ClientPortRouDi and ClientPortUser. The ClientPortUser
/// uses the functionality of a ChunkSender and ChunReceiver for sending requests and receiving responses.
/// Additionally it provides the connect / disconnect API which controls whether the client port shall connect to the
/// server.
/// @note This class is not thread-safe and must be guarded by a mutex if used in a multithreaded context.
class ClientPortUser : public BasePort
{
  public:
    using MemberType_t = ClientPortData;

    /// @brief Creates a ClientPortUser from ClientPortData which are shared with ClientPortRouDi
    /// @param[in] clientPortData to be are accessed by the ClientPortUser interface
    explicit ClientPortUser(MemberType_t& clientPortData) noexcept;

    ClientPortUser(const ClientPortUser& other) = delete;
    ClientPortUser& operator=(const ClientPortUser&) = delete;
    ClientPortUser(ClientPortUser&& rhs) = default;
    ClientPortUser& operator=(ClientPortUser&& rhs) = default;
    ~ClientPortUser() = default;

    /// @brief Allocate a chunk, the ownerhip of the SharedChunk remains in the ClientPortUser for being able to
    /// cleanup if the user process disappears
    /// @param[in] userPayloadSize, size of the user-paylaod without additional headers
    /// @param[in] userPayloadAlignment, alignment of the user-paylaod without additional headers
    /// @return on success pointer to a RequestHeader which can be used to access the chunk-header, user-header and
    /// user-payload fields, error if not
    cxx::expected<RequestHeader*, AllocationError> allocateRequest(const uint32_t userPayloadSize,
                                                                   const uint32_t userPayloadAlignment) noexcept;

    /// @brief Free an allocated request without sending it
    /// @param[in] requestHeader, pointer to the RequestHeader to free
    void freeRequest(RequestHeader* const requestHeader) noexcept;

    /// @brief Send an allocated request chunk to the server port
    /// @param[in] requestHeader, pointer to the RequestHeader to send
    void sendRequest(RequestHeader* const requestHeader) noexcept;

    /// @brief try to connect to the server Caution: There can be delays between calling connect and a change
    /// in the connection state
    /// @code
    ///   myPort.connect();
    ///   while(myPort.getConnectionState() != ConnectionState::CONNECTED)
    ///       sleep(1_s);
    /// @endcode
    void connect() noexcept;

    /// @brief disconnect from the server
    void disconnect() noexcept;

    /// @brief get the current connection state. Caution: There can be delays between calling connect and a change
    /// in the connection state. The connection state can also change without user interaction if the server comes
    /// and goes
    /// @return ConnectionState
    ConnectionState getConnectionState() const noexcept;

    /// @brief Tries to get the next response from the queue. If there is a new one, the ResponseHeader of the oldest
    /// response in the queue is returned (FiFo queue)
    /// @return cxx::expected that has a new ResponseHeader if there are new responses in the underlying queue,
    /// ChunkReceiveResult on error
    cxx::expected<const ResponseHeader*, ChunkReceiveResult> getResponse() noexcept;

    /// @brief Release a response that was obtained with getResponseChunk
    /// @param[in] requestHeader, pointer to the ResponseHeader to release
    void releaseResponse(const ResponseHeader* const responseHeader) noexcept;

    /// @brief check if there are responses in the queue
    /// @return if there are responses in the queue return true, otherwise false
    bool hasNewResponses() const noexcept;

    /// @brief check if there was a queue overflow since the last call of hasLostResponseChunks
    /// @return true if the underlying queue overflowed since last call of this method, otherwise false
    bool hasLostResponsesSinceLastCall() noexcept;

    /// @brief set a condition variable (via its pointer) to the client
    void setConditionVariable(ConditionVariableData& conditionVariableData, const uint64_t notificationIndex) noexcept;

    /// @brief unset a condition variable from the client
    void unsetConditionVariable() noexcept;

    /// @brief check if there's a condition variable set
    /// @return true if a condition variable attached, otherwise false
    bool isConditionVariableSet() const noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    ChunkSender<ClientChunkSenderData_t> m_chunkSender;
    ChunkReceiver<ClientChunkReceiverData_t> m_chunkReceiver;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_CLIENT_PORT_USER_HPP
