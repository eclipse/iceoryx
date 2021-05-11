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

#include "iceoryx_utils/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/platform/socket.hpp"
#include "iceoryx_utils/platform/unistd.hpp"

#include <chrono>
#include <cstdlib>
#include <string>

namespace iox
{
namespace posix
{
constexpr char UnixDomainSocket::PATH_PREFIX[];

UnixDomainSocket::UnixDomainSocket() noexcept
{
    this->m_isInitialized = false;
    this->m_errorValue = IpcChannelError::NOT_INITIALIZED;
}

UnixDomainSocket::UnixDomainSocket(const IpcChannelName_t& name,
                                   const IpcChannelMode mode,
                                   const IpcChannelSide channelSide,
                                   const size_t maxMsgSize,
                                   const uint64_t maxMsgNumber) noexcept
    : UnixDomainSocket(
        NoPathPrefix,
        [&]() -> UdsName_t {
            /// invalid names will be forwarded and handled by the other constructor
            /// separately
            if (!isNameValid(name))
            {
                return name;
            }
            return UdsName_t(PATH_PREFIX).append(iox::cxx::TruncateToCapacity, name);
        }(),
        mode,
        channelSide,
        maxMsgSize,
        maxMsgNumber)
{
}

UnixDomainSocket::UnixDomainSocket(const NoPathPrefix_t,
                                   const UdsName_t& name,
                                   const IpcChannelMode mode,
                                   const IpcChannelSide channelSide,
                                   const size_t maxMsgSize,
                                   const uint64_t maxMsgNumber IOX_MAYBE_UNUSED) noexcept
    : m_name(name)
    , m_channelSide(channelSide)
{
    if (!isNameValid(name))
    {
        this->m_isInitialized = false;
        this->m_errorValue = IpcChannelError::INVALID_CHANNEL_NAME;
        return;
    }

    if (maxMsgSize > MAX_MESSAGE_SIZE)
    {
        this->m_isInitialized = false;
        this->m_errorValue = IpcChannelError::MAX_MESSAGE_SIZE_EXCEEDED;
    }
    else
    {
        m_maxMessageSize = maxMsgSize;
        initalizeSocket(mode)
            .and_then([this]() { this->m_isInitialized = true; })
            .or_else([this](IpcChannelError& error) {
                this->m_isInitialized = false;
                this->m_errorValue = error;
            });
    }
}

UnixDomainSocket::UnixDomainSocket(UnixDomainSocket&& other) noexcept
{
    *this = std::move(other);
}

UnixDomainSocket::~UnixDomainSocket() noexcept
{
    if (destroy().has_error())
    {
        std::cerr << "unable to cleanup unix domain socket \"" << m_name << "\" in the destructor" << std::endl;
    }
}

UnixDomainSocket& UnixDomainSocket::operator=(UnixDomainSocket&& other) noexcept
{
    if (this != &other)
    {
        if (destroy().has_error())
        {
            std::cerr << "Unable to cleanup unix domain socket \"" << m_name
                      << "\" in the move constructor/move assingment operator" << std::endl;
        }

        CreationPattern_t::operator=(std::move(other));

        m_name = std::move(other.m_name);
        m_channelSide = std::move(other.m_channelSide);
        m_sockfd = std::move(other.m_sockfd);
        m_sockAddr = std::move(other.m_sockAddr);
        m_maxMessageSize = std::move(other.m_maxMessageSize);

        other.m_sockfd = INVALID_FD;
    }

    return *this;
}

cxx::expected<bool, IpcChannelError> UnixDomainSocket::unlinkIfExists(const UdsName_t& name) noexcept
{
    return unlinkIfExists(NoPathPrefix, UdsName_t(PATH_PREFIX).append(iox::cxx::TruncateToCapacity, name));
}

cxx::expected<bool, IpcChannelError> UnixDomainSocket::unlinkIfExists(const NoPathPrefix_t,
                                                                      const UdsName_t& name) noexcept
{
    if (!isNameValid(name))
    {
        return cxx::error<IpcChannelError>(IpcChannelError::INVALID_CHANNEL_NAME);
    }

    auto unlinkCall =
        cxx::makeSmartC(unlink, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {ENOENT}, name.c_str());

    if (!unlinkCall.hasErrors())
    {
        // ENOENT is set if this socket is not known
        return cxx::success<bool>(unlinkCall.getErrNum() != ENOENT);
    }
    else
    {
        return cxx::error<IpcChannelError>(IpcChannelError::INTERNAL_LOGIC_ERROR);
    }
}

cxx::expected<IpcChannelError> UnixDomainSocket::closeFileDescriptor() noexcept
{
    if (m_sockfd != INVALID_FD)
    {
        auto closeCall =
            cxx::makeSmartC(iox_close, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, m_sockfd);

        if (!closeCall.hasErrors())
        {
            if (IpcChannelSide::SERVER == m_channelSide)
            {
                unlink(m_sockAddr.sun_path);
            }

            m_sockfd = INVALID_FD;
            m_isInitialized = false;

            return cxx::success<void>();
        }
        else
        {
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(closeCall.getErrNum()));
        }
    }
    return cxx::success<>();
}

cxx::expected<IpcChannelError> UnixDomainSocket::destroy() noexcept
{
    if (m_isInitialized)
    {
        return closeFileDescriptor();
    }

    return cxx::success<void>();
}

cxx::expected<IpcChannelError> UnixDomainSocket::send(const std::string& msg) const noexcept
{
    // we also support timedSend. The setsockopt call sets the timeout for all further sendto calls, so we must set
    // it to 0 to turn the timeout off
    return timedSend(msg, units::Duration::fromSeconds(0ULL));
}

cxx::expected<IpcChannelError> UnixDomainSocket::timedSend(const std::string& msg, const units::Duration& timeout) const
    noexcept
{
    if (msg.size() >= m_maxMessageSize) // message sizes with null termination must be smaller than m_maxMessageSize
    {
        return cxx::error<IpcChannelError>(IpcChannelError::MESSAGE_TOO_LONG);
    }

    if (IpcChannelSide::SERVER == m_channelSide)
    {
        std::cerr << "sending on server side not supported for unix domain socket \"" << m_name << "\"" << std::endl;
        return cxx::error<IpcChannelError>(IpcChannelError::INTERNAL_LOGIC_ERROR);
    }

    struct timeval tv = timeout;
#if defined(__APPLE__)
    if (tv.tv_sec != 0 || tv.tv_usec != 0)
    {
        std::cerr
            << "socket: \"" << m_name
            << "\", timedSend with a timeout != 0 is not supported on MacOS. timedSend will behave like send instead."
            << std::endl;
    }
#endif

    auto setsockoptCall = cxx::makeSmartC(setsockopt,
                                          cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                          {static_cast<ssize_t>(ERROR_CODE)},
                                          {EWOULDBLOCK},
                                          m_sockfd,
                                          SOL_SOCKET,
                                          SO_SNDTIMEO,
                                          reinterpret_cast<const char*>(&tv),
                                          static_cast<socklen_t>(sizeof(tv)));

    if (setsockoptCall.hasErrors())
    {
        return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(setsockoptCall.getErrNum()));
    }
    else
    {
        auto sendCall = cxx::makeSmartC(sendto,
                                        cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                        {static_cast<ssize_t>(ERROR_CODE)},
                                        {},
                                        m_sockfd,
                                        msg.c_str(),
                                        static_cast<size_t>(msg.size() + 1), // +1 for the \0 at the end
                                        static_cast<int>(0),
                                        nullptr, // socket address not used for a connected SOCK_DGRAM
                                        static_cast<socklen_t>(0));

        if (sendCall.hasErrors())
        {
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(sendCall.getErrNum()));
        }
        else
        {
            return cxx::success<void>();
        }
    }
}

cxx::expected<std::string, IpcChannelError> UnixDomainSocket::receive() const noexcept
{
    // we also support timedReceive. The setsockopt call sets the timeout for all further recvfrom calls, so we must set
    // it to 0 to turn the timeout off
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    return timedReceive(units::Duration(tv));
}


cxx::expected<std::string, IpcChannelError> UnixDomainSocket::timedReceive(const units::Duration& timeout) const
    noexcept
{
    if (IpcChannelSide::CLIENT == m_channelSide)
    {
        std::cerr << "receiving on client side not supported for unix domain socket \"" << m_name << "\"" << std::endl;
        return cxx::error<IpcChannelError>(IpcChannelError::INTERNAL_LOGIC_ERROR);
    }

    struct timeval tv = timeout;

    auto setsockoptCall = cxx::makeSmartC(setsockopt,
                                          cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                          {static_cast<ssize_t>(ERROR_CODE)},
                                          {EWOULDBLOCK},
                                          m_sockfd,
                                          SOL_SOCKET,
                                          SO_RCVTIMEO,
                                          reinterpret_cast<const char*>(&tv),
                                          static_cast<socklen_t>(sizeof(tv)));

    if (setsockoptCall.hasErrors())
    {
        return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(setsockoptCall.getErrNum()));
    }
    else
    {
        char message[MAX_MESSAGE_SIZE + 1];
        auto recvCall = cxx::makeSmartC(recvfrom,
                                        cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                        {static_cast<ssize_t>(ERROR_CODE)},
                                        {EAGAIN},
                                        m_sockfd,
                                        &(message[0]),
                                        MAX_MESSAGE_SIZE,
                                        0,
                                        nullptr,
                                        nullptr);
        message[MAX_MESSAGE_SIZE] = 0;

        if (recvCall.hasErrors())
        {
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(recvCall.getErrNum()));
        }
        /// we have to handle the timeout separately since it is not actual an
        /// error, it is expected behavior. but we have to still inform the user
        else if (recvCall.getErrNum() == EAGAIN)
        {
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(recvCall.getErrNum()));
        }
        else
        {
            return cxx::success<std::string>(std::string(message));
        }
    }
}


cxx::expected<IpcChannelError> UnixDomainSocket::initalizeSocket(const IpcChannelMode mode) noexcept
{
    // initialize the sockAddr data structure with the provided name
    memset(&m_sockAddr, 0, sizeof(m_sockAddr));
    m_sockAddr.sun_family = AF_LOCAL;
    if (m_name.size() > LONGEST_VALID_NAME)
    {
        return cxx::error<IpcChannelError>(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    strncpy(m_sockAddr.sun_path, m_name.c_str(), m_name.size());

    // we currently don't support a IpcChannelMode::NON_BLOCKING, for send and receive timouts can be used, the other
    // calls are blocking
    if (IpcChannelMode::NON_BLOCKING == mode)
    {
        return cxx::error<IpcChannelError>(IpcChannelError::INVALID_ARGUMENTS);
    }

    // the mask will be applied to the permissions, we only allow users and group members to have read and write access
    // the system call always succeeds, no need to check for errors
    mode_t umaskSaved = umask(S_IXUSR | S_IXGRP | S_IRWXO);
    // Reset to old umask when going out of scope
    cxx::GenericRAII umaskGuard([&] { umask(umaskSaved); });

    auto socketCall =
        cxx::makeSmartC(socket, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, AF_LOCAL, SOCK_DGRAM, 0);

    if (socketCall.hasErrors())
    {
        return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(socketCall.getErrNum()));
    }

    m_sockfd = socketCall.getReturnValue();

    if (IpcChannelSide::SERVER == m_channelSide)
    {
        unlink(m_sockAddr.sun_path);

        auto bindCall = cxx::makeSmartC(bind,
                                        cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                        {ERROR_CODE},
                                        {},
                                        m_sockfd,
                                        reinterpret_cast<struct sockaddr*>(&m_sockAddr),
                                        static_cast<socklen_t>(sizeof(m_sockAddr)));

        if (!bindCall.hasErrors())
        {
            return cxx::success<>();
        }
        else
        {
            closeFileDescriptor().or_else([](auto) {
                std::cerr << "Unable to close socket file descriptor in error related cleanup during initialization."
                          << std::endl;
            });
            // possible errors in closeFileDescriptor() are masked and we inform the user about the actual error
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(bindCall.getErrNum()));
        }
    }
    else
    {
        // we use a connected socket, this leads to a behavior closer to the message queue (e.g. error if client
        // is created and server not present)
        auto connectCall = cxx::makeSmartC(connect,
                                           cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                           {ERROR_CODE},
                                           {ENOENT},
                                           m_sockfd,
                                           (struct sockaddr*)&m_sockAddr,
                                           static_cast<socklen_t>(sizeof(m_sockAddr)));

        if (connectCall.hasErrors())
        {
            closeFileDescriptor().or_else([](auto) {
                std::cerr << "Unable to close socket file descriptor in error related cleanup during initialization."
                          << std::endl;
            });
            // possible errors in closeFileDescriptor() are masked and we inform the user about the actual error
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(connectCall.getErrNum()));
        }
        else if (connectCall.getErrNum() == ENOENT)
        {
            closeFileDescriptor().or_else([](auto) {
                std::cerr << "Unable to close socket file descriptor in error related cleanup during initialization."
                          << std::endl;
            });
            // possible errors in closeFileDescriptor() are masked and we inform the user about the actual error
            return cxx::error<IpcChannelError>(convertErrnoToIpcChannelError(connectCall.getErrNum()));
        }
        else
        {
            return cxx::success<>();
        }
    }
}

cxx::expected<bool, IpcChannelError> UnixDomainSocket::isOutdated() noexcept
{
    // This is for being API compatible with the message queue, but has no equivalent for socket.
    // We return false to say that the socket is not outdated. If there is a problem,
    // we rely on the other calls and their error returns

    return cxx::success<bool>(false);
}


IpcChannelError UnixDomainSocket::convertErrnoToIpcChannelError(const int32_t errnum) const noexcept
{
    switch (errnum)
    {
    case EACCES:
    {
        std::cerr << "permission to create unix domain socket denied \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::ACCESS_DENIED);
    }
    case EAFNOSUPPORT:
    {
        std::cerr << "address family not supported for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_ARGUMENTS);
    }
    case EINVAL:
    {
        std::cerr << "provided invalid arguments for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_ARGUMENTS);
    }
    case EMFILE:
    {
        std::cerr << "process limit reached for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::PROCESS_LIMIT);
    }
    case ENFILE:
    {
        std::cerr << "system limit reached for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::SYSTEM_LIMIT);
    }
    case ENOBUFS:
    {
        std::cerr << "out of memory for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::OUT_OF_MEMORY);
    }
    case ENOMEM:
    {
        std::cerr << "out of memory for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::OUT_OF_MEMORY);
    }
    case EPROTONOSUPPORT:
    {
        std::cerr << "protocol type not supported for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_ARGUMENTS);
    }
    case EADDRINUSE:
    {
        std::cerr << "unix domain socket already in use \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::CHANNEL_ALREADY_EXISTS);
    }
    case EBADF:
    {
        std::cerr << "invalid file descriptor for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_FILE_DESCRIPTOR);
    }
    case ENOTSOCK:
    {
        std::cerr << "invalid file descriptor for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_FILE_DESCRIPTOR);
    }
    case EADDRNOTAVAIL:
    {
        std::cerr << "interface or address error for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case EFAULT:
    {
        std::cerr << "outside address space error for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case ELOOP:
    {
        std::cerr << "too many symbolic links for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case ENAMETOOLONG:
    {
        std::cerr << "name too long for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case ENOTDIR:
    {
        std::cerr << "not a directory error for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case ENOENT:
    {
        // no error message needed since this is a normal use case
        return IpcChannelError(IpcChannelError::NO_SUCH_CHANNEL);
    }
    case EROFS:
    {
        std::cerr << "read only error for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_CHANNEL_NAME);
    }
    case EIO:
    {
        std::cerr << "I/O for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::I_O_ERROR);
    }
    case ENOPROTOOPT:
    {
        std::cerr << "invalid option for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::INVALID_ARGUMENTS);
    }
    case ECONNREFUSED:
    {
        std::cerr << "No server for unix domain socket \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::NO_SUCH_CHANNEL);
    }
    case ECONNRESET:
    {
        std::cerr << "connection was reset by peer for \"" << m_name << "\"" << std::endl;
        return IpcChannelError(IpcChannelError::CONNECTION_RESET_BY_PEER);
    }
    case EWOULDBLOCK:
    {
        // no error message needed since this is a normal use case
        return IpcChannelError(IpcChannelError::TIMEOUT);
    }
    default:
    {
        std::cerr << "internal logic error in unix domain socket \"" << m_name << "\" occurred" << std::endl;
        return IpcChannelError(IpcChannelError::INTERNAL_LOGIC_ERROR);
    }
    }
}

bool UnixDomainSocket::isNameValid(const UdsName_t& name) noexcept
{
    return !(name.empty() || name.size() < SHORTEST_VALID_NAME || name.size() > LONGEST_VALID_NAME);
}


} // namespace posix
} // namespace iox
