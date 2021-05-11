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

#ifndef IOX_DDS_DDS_IOX_CHUNK_DATAGRAM_HEADER_HPP
#define IOX_DDS_DDS_IOX_CHUNK_DATAGRAM_HEADER_HPP

#include "iceoryx_utils/cxx/vector.hpp"

#include <cstdint>

namespace iox
{
namespace dds
{
/// @brief the endianess of the serialized data
enum class Endianess : uint8_t
{
    UNDEFINED,
    LITTLE,
    BIG,
    MIXED,
};

constexpr const char* EndianessString[] = {"UNDEFINED", "LITTLE", "BIG", "MIXED"};

/// @brief Detects the endianness of the system
Endianess getEndianess();

/// @brief The datagram header with chunk metadata for user-header and user-payload
struct IoxChunkDatagramHeader
{
    using Serialized_t = iox::cxx::vector<uint8_t, 16U>;

    /// @brief Serializes a IoxChunkDatagramHeader into a vector of uint8_t
    /// @param[in] datagramHeader to serialize
    /// @return the serialized IoxChunkDatagramHeader
    static Serialized_t serialize(const IoxChunkDatagramHeader& datagramHeader);

    /// @brief Deserializes a vector of uint8_t into a IoxChunkDatagramHeader
    /// @param[in] serializedDatagram is the serialized IoxChunkDatagramHeader
    /// @return the deserialized IoxChunkDatagramHeader
    static IoxChunkDatagramHeader deserialize(const Serialized_t& serializedDatagramHeader);

    /// @brief From the 1.0 release onward, this must be incremented for each incompatible change, e.g.
    ///            - data width of members changes
    ///            - members are rearranged
    ///            - semantic meaning of a member changes
    static constexpr uint8_t DATAGRAM_VERSION{1U};

    /// @note This must always be the first member and always 1 bytes in order to prevent issues with endianess when
    /// deserialized or incorrectly detected versions due to different size
    uint8_t datagramVersion{DATAGRAM_VERSION};
    /// @note This must always be 1 byte in order to prevent issues with endianess when deserialized
    Endianess endianness{Endianess::UNDEFINED};
    uint16_t userHeaderId{0xFFFF};
    uint32_t userHeaderSize{0U};
    uint32_t userPayloadSize{0U};
    uint32_t userPayloadAlignment{0U};
};

} // namespace dds
} // namespace iox

#endif // IOX_DDS_DDS_IOX_CHUNK_DATAGRAM_HEADER_HPP
