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

#ifndef IOX_DDS_DDS_DATA_READER_HPP
#define IOX_DDS_DDS_DATA_READER_HPP

#include "iceoryx_dds/dds/iox_chunk_datagram_header.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

namespace iox
{
namespace dds
{
enum class DataReaderError : uint8_t
{
    INVALID_STATE,
    NOT_CONNECTED,
    INVALID_DATAGRAM_HEADER_SIZE,
    INVALID_BUFFER_PARAMETER_FOR_USER_HEADER,
    INVALID_BUFFER_PARAMETER_FOR_USER_PAYLOAD,
    INVALID_DATA,
    BUFFER_SIZE_MISMATCH
};

constexpr const char* DataReaderErrorString[] = {"INVALID_STATE",
                                                 "NOT_CONNECTED",
                                                 "INVALID_DATAGRAM_HEADER_SIZE",
                                                 "INVALID_BUFFER_PARAMETER_FOR_USER_HEADER",
                                                 "INVALID_BUFFER_PARAMETER_FOR_USER_PAYLOAD",
                                                 "INVALID_DATA",
                                                 "BUFFER_SIZE_MISMATCH"};

class DataReader
{
  public:
    ///
    /// @brief connect Connect the DataReader to the underlying DDS network.
    ///
    virtual void connect() noexcept = 0;

    ///
    /// @brief peekNextIoxChunkDatagramHeader Get the IoxChunkDatagramHeader of the next sample if one is available.
    /// @return The IoxChunkDatagramHeader of the next sample if one is available.
    ///
    virtual iox::cxx::optional<IoxChunkDatagramHeader> peekNextIoxChunkDatagramHeader() noexcept = 0;

    ///
    /// @brief hasSamples Checks if new samples ready to take.
    /// @return True if new samples available.
    ///
    virtual bool hasSamples() noexcept = 0;

    ///
    /// @brief take Take the next available sample from the DDS data space.
    /// @param datagramHeader with size information
    /// @param userHeaderBuffer buffer for the user-header
    /// @param userPayloadBuffer buffer for the user-payload
    /// @return Error if unsuccessful.
    ///
    virtual iox::cxx::expected<DataReaderError> takeNext(const IoxChunkDatagramHeader datagramHeader,
                                                         uint8_t* const userHeaderBuffer,
                                                         uint8_t* const userPayloadBuffer) noexcept = 0;

    ///
    /// @brief getServiceId
    /// @return The ID of the service producing the bytes
    ///
    virtual capro::IdString_t getServiceId() const noexcept = 0;

    ///
    /// @brief getInstanceId
    /// @return The ID of the instance of the service producing the bytes
    ///
    virtual capro::IdString_t getInstanceId() const noexcept = 0;

    ///
    /// @brief getEventId
    /// @return The ID of the event producing the data
    ///
    virtual capro::IdString_t getEventId() const noexcept = 0;

  protected:
    DataReader() noexcept = default;
};
} // namespace dds
} // namespace iox

#endif // IOX_DDS_DDS_DATA_READER_HPP
