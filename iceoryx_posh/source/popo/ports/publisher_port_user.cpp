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

#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"

namespace iox
{
namespace popo
{
PublisherPortUser::PublisherPortUser(cxx::not_null<MemberType_t* const> publisherPortDataPtr) noexcept
    : BasePort(publisherPortDataPtr)
    , m_chunkSender(&getMembers()->m_chunkSenderData)

{
}

const PublisherPortUser::MemberType_t* PublisherPortUser::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

PublisherPortUser::MemberType_t* PublisherPortUser::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

cxx::expected<mepoo::ChunkHeader*, AllocationError>
PublisherPortUser::tryAllocateChunk(const uint32_t userPayloadSize,
                                    const uint32_t userPayloadAlignment,
                                    const uint32_t userHeaderSize,
                                    const uint32_t userHeaderAlignment) noexcept
{
    return m_chunkSender.tryAllocate(
        getUniqueID(), userPayloadSize, userPayloadAlignment, userHeaderSize, userHeaderAlignment);
}

void PublisherPortUser::releaseChunk(mepoo::ChunkHeader* const chunkHeader) noexcept
{
    m_chunkSender.release(chunkHeader);
}

void PublisherPortUser::sendChunk(mepoo::ChunkHeader* const chunkHeader) noexcept
{
    const auto offerRequested = getMembers()->m_offeringRequested.load(std::memory_order_relaxed);

    if (offerRequested)
    {
        m_chunkSender.send(chunkHeader);
    }
    else
    {
        // if the publisher port is not offered, we do not send the chunk but we put them in the history
        // this is needed e.g. for AUTOSAR Adaptive fields
        // just always calling send and relying that there are no subscribers if not offered does not work, as the list
        // of subscribers is updated asynchronously by RouDi (only RouDi has write access to the list of subscribers)
        m_chunkSender.pushToHistory(chunkHeader);
    }
}

cxx::optional<const mepoo::ChunkHeader*> PublisherPortUser::tryGetPreviousChunk() const noexcept
{
    return m_chunkSender.tryGetPreviousChunk();
}

void PublisherPortUser::offer() noexcept
{
    if (!getMembers()->m_offeringRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_offeringRequested.store(true, std::memory_order_relaxed);
    }
}

void PublisherPortUser::stopOffer() noexcept
{
    if (getMembers()->m_offeringRequested.load(std::memory_order_relaxed))
    {
        getMembers()->m_offeringRequested.store(false, std::memory_order_relaxed);
    }
}

bool PublisherPortUser::isOffered() const noexcept
{
    return getMembers()->m_offeringRequested.load(std::memory_order_relaxed);
}

bool PublisherPortUser::hasSubscribers() const noexcept
{
    return m_chunkSender.hasStoredQueues();
}

} // namespace popo
} // namespace iox
