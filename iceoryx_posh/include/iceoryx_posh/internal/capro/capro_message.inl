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


namespace iox
{
namespace capro
{
inline constexpr const char* asStringLiteral(CaproMessageType value) noexcept
{
    switch (value)
    {
    case CaproMessageType::NOTYPE:
        return "CaproMessageType::NOTYPE";
    case CaproMessageType::FIND:
        return "CaproMessageType::FIND";
    case CaproMessageType::OFFER:
        return "CaproMessageType::OFFER";
    case CaproMessageType::STOP_OFFER:
        return "CaproMessageType::STOP_OFFER";
    case CaproMessageType::SUB:
        return "CaproMessageType::SUB";
    case CaproMessageType::UNSUB:
        return "CaproMessageType::UNSUB";
    case CaproMessageType::CONNECT:
        return "CaproMessageType::CONNECT";
    case CaproMessageType::DISCONNECT:
        return "CaproMessageType::DISCONNECT";
    case CaproMessageType::ACK:
        return "CaproMessageType::ACK";
    case CaproMessageType::NACK:
        return "CaproMessageType::NACK";
    case CaproMessageType::PUB:
        return "CaproMessageType::PUB";
    case CaproMessageType::REQ:
        return "CaproMessageType::REQ";
    case CaproMessageType::RES:
        return "CaproMessageType::RES";
    case CaproMessageType::PING:
        return "CaproMessageType::PING";
    case CaproMessageType::PONG:
        return "CaproMessageType::PONG";
    case CaproMessageType::MESSGAGE_TYPE_END:
        return "CaproMessageType::MESSGAGE_TYPE_END";
    }

    return "[Undefined CaproMessageType]";
}

inline std::ostream& operator<<(std::ostream& stream, CaproMessageType value)
{
    stream << asStringLiteral(value);
    return stream;
}

inline log::LogStream& operator<<(log::LogStream& stream, CaproMessageType value)
{
    stream << asStringLiteral(value);
    return stream;
}
} // namespace capro
} // namespace iox
