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
ClientPortUser::ClientPortUser(MemberType_t& clientPortData) noexcept
    : BasePort(&clientPortData)
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
ClientPortUser::allocateRequest(const uint32_t userPayloadSize, const uint32_t userPayloadAlignment) noexcept
{
    auto allocateResult = m_chunkSender.tryAllocate(
        getUniqueID(), userPayloadSize, userPayloadAlignment, sizeof(RequestHeader), alignof(RequestHeader));

    if (allocateResult.has_error())
    {
        return cxx::error<AllocationError>(allocateResult.get_error());
    }

    auto requestHeader = new (allocateResult.value()->userHeader())
        RequestHeader(getMembers()->m_uniqueId, RpcBaseHeader::UNKNOWN_CLIENT_QUEUE_INDEX);

    return cxx::success<RequestHeader*>(requestHeader);
}

void ClientPortUser::freeRequest(RequestHeader* const requestHeader) noexcept
{
    if (requestHeader)
    {
        m_chunkSender.release(requestHeader->getChunkHeader());
    }
    else
    {
        errorHandler(Error::kPOPO__CLIENT_PORT_INVALID_REQUEST_TO_FREE_FROM_USER, nullptr, ErrorLevel::SEVERE);
    }
}

void ClientPortUser::sendRequest(RequestHeader* const requestHeader) noexcept
{
    const auto connectRequested = getMembers()->m_connectRequested.load(std::memory_order_relaxed);

    if (connectRequested)
    {
        if (requestHeader)
        {
            m_chunkSender.send(requestHeader->getChunkHeader());
        }
        else
        {
            errorHandler(Error::kPOPO__CLIENT_PORT_INVALID_REQUEST_TO_SEND_FROM_USER, nullptr, ErrorLevel::SEVERE);
        }
    }
    else
    {
        LogWarn() << "Try to send request without being connected!";
    }
}

void ClientPortUser::connect() noexcept
{
    if (!getMembers()->m_connectRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_connectRequested.store(true, std::memory_order_relaxed);
    }
}

void ClientPortUser::disconnect() noexcept
{
    if (getMembers()->m_connectRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_connectRequested.store(false, std::memory_order_relaxed);
    }
}

ConnectionState ClientPortUser::getConnectionState() const noexcept
{
    return getMembers()->m_connectionState;
}

cxx::expected<const ResponseHeader*, ChunkReceiveResult> ClientPortUser::getResponse() noexcept
{
    auto getChunkResult = m_chunkReceiver.tryGet();

    if (getChunkResult.has_error())
    {
        return cxx::error<ChunkReceiveResult>(getChunkResult.get_error());
    }

    return cxx::success<const ResponseHeader*>(
        static_cast<const ResponseHeader*>(getChunkResult.value()->userHeader()));
}

void ClientPortUser::releaseResponse(const ResponseHeader* const responseHeader) noexcept
{
    if (responseHeader)
    {
        m_chunkReceiver.release(responseHeader->getChunkHeader());
    }
    else
    {
        errorHandler(Error::kPOPO__CLIENT_PORT_INVALID_RESPONSE_TO_RELEASE_FROM_USER, nullptr, ErrorLevel::SEVERE);
    }
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
