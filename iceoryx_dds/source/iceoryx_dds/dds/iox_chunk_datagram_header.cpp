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

#include "iceoryx_dds/dds/iox_chunk_datagram_header.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace dds
{
Endianess getEndianess()
{
    uint32_t endianDetector{0x01020304};
    switch (reinterpret_cast<uint8_t*>(&endianDetector)[0U])
    {
    case 1U:
        return Endianess::BIG;
    case 4U:
        return Endianess::LITTLE;
    case 2U:
        return Endianess::MIXED;
    default:
        return Endianess::UNDEFINED;
    }

    return Endianess::UNDEFINED;
}

IoxChunkDatagramHeader::Serialized_t IoxChunkDatagramHeader::serialize(const IoxChunkDatagramHeader& datagramHeader)
{
    Serialized_t serializedDatagram;

    serializedDatagram.emplace_back(datagramHeader.datagramVersion);
    serializedDatagram.emplace_back(static_cast<uint8_t>(datagramHeader.endianness));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userHeaderId >> 8U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>(datagramHeader.userHeaderId & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userHeaderSize >> 24U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userHeaderSize >> 16U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userHeaderSize >> 8U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>(datagramHeader.userHeaderSize & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadSize >> 24U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadSize >> 16U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadSize >> 8U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>(datagramHeader.userPayloadSize & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadAlignment >> 24U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadAlignment >> 16U) & 0xFF));
    serializedDatagram.emplace_back(static_cast<uint8_t>((datagramHeader.userPayloadAlignment >> 8U) & 0xFF));
    auto successfullyPushed =
        serializedDatagram.emplace_back(static_cast<uint8_t>(datagramHeader.userPayloadAlignment & 0xFF));

    iox::cxx::Ensures(successfullyPushed && "Expected to successfully serialize IoxChunkDatagramHeader!");

    return serializedDatagram;
}

IoxChunkDatagramHeader
IoxChunkDatagramHeader::deserialize(const IoxChunkDatagramHeader::Serialized_t& serializedDatagramHeader)
{
    iox::cxx::Expects(serializedDatagramHeader.size() == 16U && "Expects valid IoxChunkDatagramHeader serialization!");

    IoxChunkDatagramHeader datagramHeader;

    datagramHeader.datagramVersion = serializedDatagramHeader[0U];
    datagramHeader.endianness = static_cast<Endianess>(serializedDatagramHeader[1U]);
    datagramHeader.userHeaderId = static_cast<uint16_t>((static_cast<uint16_t>(serializedDatagramHeader[2U]) << 8U)
                                                        | static_cast<uint16_t>(serializedDatagramHeader[3U]));
    datagramHeader.userHeaderSize = (static_cast<uint32_t>(serializedDatagramHeader[4U]) << 24U)
                                    | (static_cast<uint32_t>(serializedDatagramHeader[5U]) << 16U)
                                    | (static_cast<uint32_t>(serializedDatagramHeader[6U]) << 8U)
                                    | static_cast<uint32_t>(serializedDatagramHeader[7U]);
    datagramHeader.userPayloadSize = (static_cast<uint32_t>(serializedDatagramHeader[8U]) << 24U)
                                     | (static_cast<uint32_t>(serializedDatagramHeader[9U]) << 16U)
                                     | (static_cast<uint32_t>(serializedDatagramHeader[10U]) << 8U)
                                     | static_cast<uint32_t>(serializedDatagramHeader[11U]);
    datagramHeader.userPayloadAlignment = (static_cast<uint32_t>(serializedDatagramHeader[12U]) << 24U)
                                          | (static_cast<uint32_t>(serializedDatagramHeader[13U]) << 16U)
                                          | (static_cast<uint32_t>(serializedDatagramHeader[14U]) << 8U)
                                          | static_cast<uint32_t>(serializedDatagramHeader[15U]);

    return datagramHeader;
}

} // namespace dds
} // namespace iox
