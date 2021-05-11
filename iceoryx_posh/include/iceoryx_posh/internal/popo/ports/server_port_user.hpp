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
#ifndef IOX_POSH_POPO_PORTS_SERVER_PORT_USER_HPP
#define IOX_POSH_POPO_PORTS_SERVER_PORT_USER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/server_port_data.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

namespace iox
{
namespace popo
{
/// @brief The ServerPortUser provides the API for accessing a server port from the user side. The server port
/// is divided in the three parts ServerPortData, ServerPortRouDi and ServerPortUser. The ServerPortUser
/// uses the functionality of a ChunkSender and ChunReceiver for receiving requests and sending responses.
/// Additionally it provides the offer / stopOffer API which controls whether the server is discoverable
/// for client ports
class ServerPortUser : public BasePort
{
  public:
    using MemberType_t = ServerPortData;

    explicit ServerPortUser(cxx::not_null<MemberType_t* const> serverPortDataPtr) noexcept;

    ServerPortUser(const ServerPortUser& other) = delete;
    ServerPortUser& operator=(const ServerPortUser&) = delete;
    ServerPortUser(ServerPortUser&& rhs) = default;
    ServerPortUser& operator=(ServerPortUser&& rhs) = default;
    ~ServerPortUser() = default;

    /// @brief Tries to get the next request from the queue. If there is a new one, the ChunkHeader of the oldest
    /// request in the queue is returned (FiFo queue)
    /// @return optional that has a new chunk header or no value if there are no new requests in the underlying queue,
    /// ChunkReceiveResult on error
    cxx::expected<cxx::optional<const RequestHeader*>, ChunkReceiveResult> getRequest() noexcept;

    /// @brief Release a request that was obtained with getRequest
    /// @param[in] chunkHeader, pointer to the ChunkHeader to release
    void releaseRequest(const RequestHeader* const requestHeader) noexcept;

    /// @brief check if there are requests in the queue
    /// @return if there are requests in the queue return true, otherwise false
    bool hasNewRequests() const noexcept;

    /// @brief check if there was a queue overflow since the last call of hasLostRequestsSinceLastCall
    /// @return true if the underlying queue overflowed since last call of this method, otherwise false
    bool hasLostRequestsSinceLastCall() noexcept;

    /// @brief Allocate a response, the ownerhip of the SharedChunk remains in the ServerPortUser for being able to
    /// cleanup if the user process disappears
    /// @param[in] userPayloadSize, size of the user user-paylaod without additional headers
    /// @return on success pointer to a ChunkHeader which can be used to access the chunk-header, user-header and
    /// user-payload fields, error if not
    cxx::expected<ResponseHeader*, AllocationError> allocateResponse(const uint32_t userPayloadSize) noexcept;

    /// @brief Free an allocated response without sending it
    /// @param[in] chunkHeader, pointer to the ChunkHeader to free
    void freeResponse(ResponseHeader* const responseHeader) noexcept;

    /// @brief Send an allocated request chunk to the server port
    /// @param[in] chunkHeader, pointer to the ChunkHeader to send
    void sendResponse(ResponseHeader* const responseHeader) noexcept;

    /// @brief offer this server port in the system
    void offer() noexcept;

    /// @brief stop offering this server port, all clients will be disconnected from this server
    void stopOffer() noexcept;

    /// @brief Checks whether the server port is currently offered
    /// @return true if currently offered otherwise false
    bool isOffered() const noexcept;

    /// @brief Checks whether there are currently clients connected to this server
    /// @return true if there are clients otherwise false
    bool hasClients() const noexcept;

    /// @brief set a condition variable (via its pointer) to the client
    /// @return true if attachment worked, otherwise false
    void setConditionVariable(ConditionVariableData& conditionVariableData, const uint64_t notificationIndex) noexcept;

    /// @brief unset a condition variable from the client
    void unsetConditionVariable() noexcept;

    /// @brief check if there's a condition variable set
    /// @return true if a condition variable attached, otherwise false
    bool isConditionVariableSet() const noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    ChunkSender<ServerChunkSenderData_t> m_chunkSender;
    ChunkReceiver<ServerChunkReceiverData_t> m_chunkReceiver;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_PUBLISHER_PORT_USER_HPP
