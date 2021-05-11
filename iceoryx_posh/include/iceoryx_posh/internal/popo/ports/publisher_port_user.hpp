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
#ifndef IOX_POSH_POPO_PORTS_PUBLISHER_PORT_USER_HPP
#define IOX_POSH_POPO_PORTS_PUBLISHER_PORT_USER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_data.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

namespace iox
{
namespace popo
{
/// @brief The PublisherPortUser provides the API for accessing a publisher port from the user side. The publisher port
/// is divided in the three parts PublisherPortData, PublisherPortRouDi and PublisherPortUser. The PublisherPortUser
/// uses the functionality of a ChunkSender for sending shared memory chunks. Additionally it provides the offer /
/// stopOffer API which controls whether the publisher port is discoverable for subscriber ports
class PublisherPortUser : public BasePort
{
  public:
    using MemberType_t = PublisherPortData;

    explicit PublisherPortUser(cxx::not_null<MemberType_t* const> publisherPortDataPtr) noexcept;

    PublisherPortUser(const PublisherPortUser& other) = delete;
    PublisherPortUser& operator=(const PublisherPortUser&) = delete;
    PublisherPortUser(PublisherPortUser&& rhs) = default;
    PublisherPortUser& operator=(PublisherPortUser&& rhs) = default;
    ~PublisherPortUser() = default;

    /// @brief Allocate a chunk, the ownership of the SharedChunk remains in the PublisherPortUser for being able to
    /// cleanup if the user process disappears
    /// @param[in] userPayloadSize, size of the user-payload without additional headers
    /// @param[in] userPayloadAlignment, alignment of the user-payload
    /// @param[in] userHeaderSize, size of the user-header; use iox::CHUNK_NO_USER_HEADER_SIZE to omit a user-header
    /// @param[in] userHeaderAlignment, alignment of the user-header; use iox::CHUNK_NO_USER_HEADER_ALIGNMENT
    /// to omit a user-header
    /// @return on success pointer to a ChunkHeader which can be used to access the chunk-header, user-header and
    /// user-payload fields, error if not
    cxx::expected<mepoo::ChunkHeader*, AllocationError>
    tryAllocateChunk(const uint32_t userPayloadSize,
                     const uint32_t userPayloadAlignment,
                     const uint32_t userHeaderSize = 0U,
                     const uint32_t userHeaderAlignment = 1U) noexcept;

    /// @brief Free an allocated chunk without sending it
    /// @param[in] chunkHeader, pointer to the ChunkHeader to free
    void releaseChunk(mepoo::ChunkHeader* const chunkHeader) noexcept;

    /// @brief Send an allocated chunk to all connected subscriber ports
    /// @param[in] chunkHeader, pointer to the ChunkHeader to send
    void sendChunk(mepoo::ChunkHeader* const chunkHeader) noexcept;

    /// @brief Returns the last sent chunk if there is one
    /// @return pointer to the ChunkHeader of the last sent Chunk if there is one, empty optional if not
    cxx::optional<const mepoo::ChunkHeader*> tryGetPreviousChunk() const noexcept;

    /// @brief offer this publiher port in the system
    void offer() noexcept;

    /// @brief stop offering this publisher port, all subscribers will be removed from this publisher
    void stopOffer() noexcept;

    /// @brief Checks whether the publisher port is currently offered
    /// @return true if currently offered otherwise false
    bool isOffered() const noexcept;

    /// @brief Checks whether there are currently subscribers connected to this publisher
    /// @return true if there are subscribers otherwise false
    bool hasSubscribers() const noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    ChunkSender<PublisherPortData::ChunkSenderData_t> m_chunkSender;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_PUBLISHER_PORT_USER_HPP
