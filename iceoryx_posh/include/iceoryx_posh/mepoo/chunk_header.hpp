// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_POSH_MEPOO_CHUNK_HEADER_HPP
#define IOX_POSH_MEPOO_CHUNK_HEADER_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/typed_unique_id.hpp"
#include "iceoryx_posh/mepoo/chunk_settings.hpp"

#include <cstdint>

namespace iox
{
namespace popo
{
template <typename T>
class ChunkSender;
}

namespace mepoo
{
/// @brief Helper struct to use as default template parameter when no user-header is used
struct NoUserHeader
{
};

struct ChunkHeader
{
    using UserPayloadOffset_t = uint32_t;

    /// @brief constructs and initializes a ChunkHeader
    /// @param[in] chunkSize is the size of the chunk the ChunkHeader is constructed
    /// @param[in] chunkSettings are the settings like user-payload size and user-header alignment
    ChunkHeader(const uint32_t chunkSize, const ChunkSettings& chunkSettings) noexcept;

    // copy/move ctors/assignment operators are deleted since the calculations for the user-header and user-payload
    // alignment are dependent on the address of the this pointer
    ChunkHeader(const ChunkHeader&) = delete;
    ChunkHeader(ChunkHeader&&) = delete;

    ChunkHeader& operator=(const ChunkHeader&) = delete;
    ChunkHeader& operator=(ChunkHeader&&) = delete;

    /// @brief From the 1.0 release onward, this must be incremented for each incompatible change, e.g.
    ///            - data width of members changes
    ///            - members are rearranged
    ///            - semantic meaning of a member changes
    static constexpr uint8_t CHUNK_HEADER_VERSION{1U};

    /// @brief User-Header id for no user-header
    static constexpr uint16_t NO_USER_HEADER{0x0000};
    /// @brief User-Header id for an unknown user-header
    static constexpr uint16_t UNKNOWN_USER_HEADER{0xFFFF};

    /// @brief The ChunkHeader version is used to detect incompatibilities for record&replay functionality
    /// @return the ChunkHeader version
    uint8_t chunkHeaderVersion() const noexcept;

    /// @brief The id of the user-header used by the chunk; if no user-header is used, this is set to NO_USER_HEADER
    /// @return the user-header id of the chunk
    uint16_t userHeaderId() const noexcept;

    /// @brief Get the pointer to the user-header
    /// @return the pointer to the user-header
    void* userHeader() noexcept;

    /// @brief Get the const pointer to the user-header
    /// @return the const pointer to the user-header
    const void* userHeader() const noexcept;

    /// @brief Get a pointer to the user-payload carried by the chunk
    /// @return the pointer to the user-payload
    void* userPayload() noexcept;

    /// @brief Get a const pointer to the user-payload carried by the chunk
    /// @return the const pointer to the user-payload
    const void* userPayload() const noexcept;

    /// @brief Get a pointer to the `ChunkHeader` associated to the user-payload of the chunk
    /// @param[in] userPayload is the pointer to the user-payload of the chunk
    /// @return the pointer to the `ChunkHeader` or a `nullptr` if `userPayload` is a `nullptr`
    static ChunkHeader* fromUserPayload(void* const userPayload) noexcept;

    /// @brief Get a const pointer to the `ChunkHeader` associated to the user-payload of the chunk
    /// @param[in] userPayload is the const pointer to the user-payload of the chunk
    /// @return the const pointer to the `ChunkHeader` or a `nullptr` if `userPayload` is a `nullptr`
    static const ChunkHeader* fromUserPayload(const void* const userPayload) noexcept;

    /// @brief Calculates the used size of the chunk with the ChunkHeader, user-heander and user-payload
    /// @return the used size of the chunk
    uint32_t usedSizeOfChunk() const noexcept;

    /// @brief The size of the whole chunk, including the header
    /// @return the chunk size
    uint32_t chunkSize() const noexcept;

    /// @brief The size of the chunk occupied by the user-header
    /// @return the user-header size
    uint32_t userHeaderSize() const noexcept;

    /// @brief The size of the chunk occupied by the user-payload
    /// @return the user-payload size
    uint32_t userPayloadSize() const noexcept;

    /// @brief The alignment of the chunk occupied by the user-payload
    /// @return the user-payload alignment
    uint32_t userPayloadAlignment() const noexcept;

    /// @brief The unique identifier of the publisher the chunk was sent from
    /// @return the id of the publisher the chunk was sent from
    UniquePortId originId() const noexcept;

    /// @brief A serial number for the sent chunks
    /// @brief the serquence number of the chunk
    uint64_t sequenceNumber() const noexcept;

  private:
    template <typename T>
    friend class popo::ChunkSender;

    void setOriginId(UniquePortId originId) noexcept;

    void setSequenceNumber(uint64_t sequenceNumber) noexcept;

    uint64_t overflowSafeUsedSizeOfChunk() const noexcept;

  private:
    // the order of these members must be changed carefully and if this happens, the m_chunkHeaderVersion
    // needs to be adapted in order to be able to detect incompatibilities between publisher/subscriber
    // or record&replay, m_chunkSize and m_chunkHeaderVersion should therefore neither changed the type,
    // nor the position

    // size of the whole chunk, including the header
    uint32_t m_chunkSize{0U};
    uint8_t m_chunkHeaderVersion{CHUNK_HEADER_VERSION};
    // reserved for future functionality and used to indicate the padding bytes; currently not used and set to `0`
    uint8_t m_reserved{0};
    // currently just a placeholder
    uint16_t m_userHeaderId{NO_USER_HEADER};
    UniquePortId m_originId{popo::InvalidId};
    uint64_t m_sequenceNumber{0U};
    uint32_t m_userHeaderSize{0U};
    uint32_t m_userPayloadSize{0U};
    uint32_t m_userPayloadAlignment{1U};
    UserPayloadOffset_t m_userPayloadOffset{sizeof(ChunkHeader)};
};

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_CHUNK_HEADER_HPP
