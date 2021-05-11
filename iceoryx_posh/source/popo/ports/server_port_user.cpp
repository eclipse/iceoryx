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

#include "iceoryx_posh/internal/popo/ports/server_port_user.hpp"

namespace iox
{
namespace popo
{
ServerPortUser::ServerPortUser(cxx::not_null<MemberType_t* const> serverPortDataPtr) noexcept
    : BasePort(serverPortDataPtr)
    , m_chunkSender(&getMembers()->m_chunkSenderData)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)

{
}

const ServerPortUser::MemberType_t* ServerPortUser::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

ServerPortUser::MemberType_t* ServerPortUser::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}


cxx::expected<cxx::optional<const RequestHeader*>, ChunkReceiveResult> ServerPortUser::getRequest() noexcept
{
    return cxx::success<cxx::optional<const RequestHeader*>>(cxx::nullopt_t());
}

void ServerPortUser::releaseRequest(const RequestHeader* const /*requestHeader*/) noexcept
{
    /// @todo
}

bool ServerPortUser::hasNewRequests() const noexcept
{
    return !m_chunkReceiver.empty();
}

bool ServerPortUser::hasLostRequestsSinceLastCall() noexcept
{
    return m_chunkReceiver.hasLostChunks();
}

cxx::expected<ResponseHeader*, AllocationError>
ServerPortUser::allocateResponse(const uint32_t /*userPayloadSize*/) noexcept
{
    /// @todo
    return cxx::error<AllocationError>(AllocationError::RUNNING_OUT_OF_CHUNKS);
}

void ServerPortUser::freeResponse(ResponseHeader* const /*responseHeader*/) noexcept
{
    /// @todo
}

void ServerPortUser::sendResponse(ResponseHeader* const /*responseHeader*/) noexcept
{
    /// @todo
}

void ServerPortUser::offer() noexcept
{
    if (!getMembers()->m_offeringRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_offeringRequested.store(true, std::memory_order_relaxed);
    }
}

void ServerPortUser::stopOffer() noexcept
{
    if (getMembers()->m_offeringRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_offeringRequested.store(false, std::memory_order_relaxed);
    }
}

bool ServerPortUser::isOffered() const noexcept
{
    return getMembers()->m_offeringRequested.load(std::memory_order_relaxed);
}

bool ServerPortUser::hasClients() const noexcept
{
    return m_chunkSender.hasStoredQueues();
}

void ServerPortUser::setConditionVariable(ConditionVariableData& conditionVariableData,
                                          const uint64_t notificationIndex) noexcept
{
    m_chunkReceiver.setConditionVariable(conditionVariableData, notificationIndex);
}

void ServerPortUser::unsetConditionVariable() noexcept
{
    m_chunkReceiver.unsetConditionVariable();
}

bool ServerPortUser::isConditionVariableSet() const noexcept
{
    return m_chunkReceiver.isConditionVariableSet();
}

} // namespace popo
} // namespace iox
