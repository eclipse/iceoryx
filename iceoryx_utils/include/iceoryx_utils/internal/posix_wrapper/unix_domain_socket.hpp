// Copyright (c) 2020 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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
#ifndef IOX_UTILS_POSIX_WRAPPER_UNIX_DOMAIN_SOCKET_HPP
#define IOX_UTILS_POSIX_WRAPPER_UNIX_DOMAIN_SOCKET_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/design_pattern/creation.hpp"
#include "iceoryx_utils/internal/posix_wrapper/ipc_channel.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_utils/platform/fcntl.hpp"
#include "iceoryx_utils/platform/mqueue.hpp"
#include "iceoryx_utils/platform/stat.hpp"
#include "iceoryx_utils/platform/un.hpp"

namespace iox
{
namespace posix
{
/// @brief Wrapper class for unix domain socket
class UnixDomainSocket : public DesignPattern::Creation<UnixDomainSocket, IpcChannelError>
{
  public:
    struct NoPathPrefix_t
    {
    };
    static constexpr NoPathPrefix_t NoPathPrefix{};
    static constexpr char PATH_PREFIX[] = "/tmp/";

    /// @brief Max message size is on linux = 4096 and on mac os = 2048. To have
    ///  the same behavior on every platform we use 2048.
    static constexpr size_t MAX_MESSAGE_SIZE = 2048U;
    static constexpr size_t SHORTEST_VALID_NAME = 2U;
    static constexpr size_t NULL_TERMINATOR_SIZE = 1;
    /// @brief The name length is limited by the size of the sockaddr_un::sun_path buffer and the path prefix
    static constexpr size_t LONGEST_VALID_NAME = sizeof(sockaddr_un::sun_path) - 1;
    static constexpr int32_t ERROR_CODE = -1;
    static constexpr int32_t INVALID_FD = -1;

    using UdsName_t = cxx::string<LONGEST_VALID_NAME>;

    /// @brief for calling private constructor in create method
    friend class DesignPattern::Creation<UnixDomainSocket, IpcChannelError>;

    /// @brief default constructor. The result is an invalid UnixDomainSocket object which can be reassigned later by
    /// using the
    /// move constructor.
    UnixDomainSocket() noexcept;

    UnixDomainSocket(const UnixDomainSocket& other) = delete;
    UnixDomainSocket(UnixDomainSocket&& other) noexcept;
    UnixDomainSocket& operator=(const UnixDomainSocket& other) = delete;
    UnixDomainSocket& operator=(UnixDomainSocket&& other) noexcept;

    ~UnixDomainSocket() noexcept;

    /// @brief unlink the provided unix domain socket
    /// @param name of the unix domain socket to unlink
    /// @return true if the unix domain socket could be unlinked, false otherwise, IpcChannelError if error occured
    static cxx::expected<bool, IpcChannelError> unlinkIfExists(const UdsName_t& name) noexcept;

    /// @brief unlink the provided unix domain socket
    /// @param NoPathPrefix signalling that this method does not add a path prefix
    /// @param name of the unix domain socket to unlink
    /// @return true if the unix domain socket could be unlinked, false otherwise, IpcChannelError if error occured
    static cxx::expected<bool, IpcChannelError> unlinkIfExists(const NoPathPrefix_t, const UdsName_t& name) noexcept;

    /// @brief close the unix domain socket.
    cxx::expected<IpcChannelError> destroy() noexcept;

    /// @brief send a message using std::string.
    /// @param msg to send
    /// @return IpcChannelError if error occured
    cxx::expected<IpcChannelError> send(const std::string& msg) const noexcept;

    /// @brief try to send a message for a given timeout duration using std::string
    /// @param msg to send
    /// @param timout for the send operation
    /// @return IpcChannelError if error occured
    cxx::expected<IpcChannelError> timedSend(const std::string& msg, const units::Duration& timeout) const noexcept;

    /// @brief receive message using std::string.
    /// @return received message. In case of an error, IpcChannelError is returned and msg is empty.
    cxx::expected<std::string, IpcChannelError> receive() const noexcept;

    /// @brief try to receive message for a given timeout duration using std::string.
    /// @param timout for the receive operation
    /// @return received message. In case of an error, IpcChannelError is returned and msg is empty.
    cxx::expected<std::string, IpcChannelError> timedReceive(const units::Duration& timeout) const noexcept;

    /// @brief checks whether the unix domain socket is outdated
    /// @return true if the unix domain socket is outdated, false otherwise, IpcChannelError if error occured
    cxx::expected<bool, IpcChannelError> isOutdated() noexcept;

  private:
    /// @brief c'tor
    /// @param name for the unix domain socket
    /// @param mode blocking or non_blocking
    /// @param channel side client or server
    /// @param maxMsgSize max message size that can be transmitted
    /// @param maxMsgNumber max messages that can be queued
    UnixDomainSocket(const IpcChannelName_t& name,
                     const IpcChannelMode mode,
                     const IpcChannelSide channelSide,
                     const size_t maxMsgSize = MAX_MESSAGE_SIZE,
                     const uint64_t maxMsgNumber = 10U) noexcept;

    /// @brief c'tor
    /// @param NoPathPrefix signalling that this constructor does not add a path prefix
    /// @param name for the unix domain socket
    /// @param mode blocking or non_blocking
    /// @param channel side client or server
    /// @param maxMsgSize max message size that can be transmitted
    /// @param maxMsgNumber max messages that can be queued
    UnixDomainSocket(const NoPathPrefix_t,
                     const UdsName_t& name,
                     const IpcChannelMode mode,
                     const IpcChannelSide channelSide,
                     const size_t maxMsgSize = MAX_MESSAGE_SIZE,
                     const uint64_t maxMsgNumber = 10U) noexcept;


    /// @brief initializes the unix domain socket
    /// @param mode blocking or non_blocking
    /// @return IpcChannelError if error occured
    cxx::expected<IpcChannelError> initalizeSocket(const IpcChannelMode mode) noexcept;

    /// @brief create an IpcChannelError from the provides error code
    /// @return IpcChannelError if error occured
    IpcChannelError convertErrnoToIpcChannelError(const int32_t errnum) const noexcept;

    static bool isNameValid(const UdsName_t& name) noexcept;

    /// @brief Tries to close the file descriptor
    /// @return IpcChannelError if error occured
    cxx::expected<IpcChannelError> closeFileDescriptor() noexcept;

  private:
    UdsName_t m_name;
    IpcChannelSide m_channelSide;
    int32_t m_sockfd{INVALID_FD};
    struct sockaddr_un m_sockAddr;
    size_t m_maxMessageSize{MAX_MESSAGE_SIZE};
};
} // namespace posix
} // namespace iox

#endif // IOX_UTILS_POSIX_WRAPPER_UNIX_DOMAIN_SOCKET_HPP
