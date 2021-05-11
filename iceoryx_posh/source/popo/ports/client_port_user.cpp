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

#include "iceoryx_posh/internal/popo/ports/client_port_user.hpp"

namespace iox
{
namespace popo
{
ClientPortUser::ClientPortUser(cxx::not_null<MemberType_t* const> clientPortDataPtr) noexcept
    : BasePort(clientPortDataPtr)
    , m_chunkSender(&getMembers()->m_chunkSenderData)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)

{
}

const ClientPortUser::MemberType_t* ClientPortUser::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

ClientPortUser::MemberType_t* ClientPortUser::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

cxx::expected<RequestHeader*, AllocationError>
ClientPortUser::allocateRequest(const uint32_t /*userPayloadSize*/) noexcept
{
    return cxx::error<AllocationError>(AllocationError::RUNNING_OUT_OF_CHUNKS);
}

void ClientPortUser::freeRequest(RequestHeader* const /*requestHeader*/) noexcept
{
    /// @todo
}

void ClientPortUser::sendRequest(RequestHeader* const /*requestHeader*/) noexcept
{
    /// @todo
}

void ClientPortUser::connect() noexcept
{
    /// @todo
}

void ClientPortUser::disconnect() noexcept
{
    /// @todo
}

ConnectionState ClientPortUser::getConnectionState() const noexcept
{
    return getMembers()->m_connectionState;
}

cxx::expected<cxx::optional<const ResponseHeader*>, ChunkReceiveResult> ClientPortUser::getResponse() noexcept
{
    /// @todo
    return cxx::success<cxx::optional<const ResponseHeader*>>(cxx::nullopt_t());
}

void ClientPortUser::releaseResponse(const ResponseHeader* const /*responseHeader*/) noexcept
{
    /// @todo
}

bool ClientPortUser::hasNewResponses() const noexcept
{
    return !m_chunkReceiver.empty();
}

bool ClientPortUser::hasLostResponsesSinceLastCall() noexcept
{
    return m_chunkReceiver.hasLostChunks();
}

void ClientPortUser::setConditionVariable(ConditionVariableData& conditionVariableData,
                                          const uint64_t notificationIndex) noexcept
{
    m_chunkReceiver.setConditionVariable(conditionVariableData, notificationIndex);
}

void ClientPortUser::unsetConditionVariable() noexcept
{
    m_chunkReceiver.unsetConditionVariable();
}

bool ClientPortUser::isConditionVariableSet() const noexcept
{
    return m_chunkReceiver.isConditionVariableSet();
}

} // namespace popo
} // namespace iox
