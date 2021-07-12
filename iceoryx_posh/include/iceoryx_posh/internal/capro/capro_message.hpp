// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_CAPRO_CAPRO_MESSAGE_HPP
#define IOX_POSH_CAPRO_CAPRO_MESSAGE_HPP

#include "iceoryx_posh/capro/service_description.hpp"

namespace iox
{
namespace log
{
class LogStream;
}
namespace capro
{
/// @brief Enum for service message types which are used in CaPro for
/// service-oriented communication
enum class CaproMessageType : uint8_t
{
    NOTYPE = 0,
    FIND,
    OFFER,
    STOP_OFFER,
    SUB,
    UNSUB,
    CONNECT,
    DISCONNECT,
    ACK,
    NACK,
    PUB,
    REQ,
    RES,
    PING,
    PONG,
    MESSGAGE_TYPE_END
};

/// @brief Converts the CaproMessageType to a string literal
/// @param[in] value to convert to a string literal
/// @return pointer to a string literal
inline constexpr const char* asStringLiteral(CaproMessageType value) noexcept;

/// @brief Convenience stream operator to easily use the `asStringLiteral` function with std::ostream
/// @param[in] stream sink to write the message to
/// @param[in] value to convert to a string literal
/// @return the reference to `stream` which was provided as input parameter
inline std::ostream& operator<<(std::ostream& stream, CaproMessageType value);

/// @brief Convenience stream operator to easily use the `asStringLiteral` function with iox::log::LogStream
/// @param[in] stream sink to write the message to
/// @param[in] value to convert to a string literal
/// @return the reference to `stream` which was provided as input parameter
inline log::LogStream& operator<<(log::LogStream& stream, CaproMessageType value);

enum class CaproMessageSubType : uint8_t
{
    NOSUBTYPE = 0,
    SERVICE,
    EVENT,
    FIELD
};

/// @brief C'tors for CaPro messages
class CaproMessage
{
  public:
    /// @brief default constructor
    CaproMessage() = default;

    /// @brief C'tor for CaPro Message with type, service description
    /// @param type                    Message type
    /// @param serviceDescription      Service Description
    /// @param subType                 Message sub type
    /// @param chunkQueueData(0)       No port
    /// @return                        Nothing
    CaproMessage(CaproMessageType type,
                 const ServiceDescription& serviceDescription,
                 CaproMessageSubType subType = CaproMessageSubType::NOSUBTYPE,
                 void* chunkQueueData = nullptr) noexcept;

    CaproMessageType m_type{CaproMessageType::NOTYPE};
    CaproMessageSubType m_subType{CaproMessageSubType::NOSUBTYPE};
    ServiceDescription m_serviceDescription;
    void* m_chunkQueueData{nullptr};
    uint64_t m_historyCapacity{0u};
};

} // namespace capro
} // namespace iox

#include "iceoryx_posh/internal/capro/capro_message.inl"

#endif // IOX_POSH_CAPRO_CAPRO_MESSAGE_HPP
