// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_EXAMPLES_ICEPERF_UDS_HPP
#define IOX_EXAMPLES_ICEPERF_UDS_HPP

#include "base.hpp"

#include "iceoryx_utils/platform/fcntl.hpp"
#include "iceoryx_utils/platform/mqueue.hpp"
#include "iceoryx_utils/platform/socket.hpp"
#include "iceoryx_utils/platform/stat.hpp"
#include "iceoryx_utils/platform/un.hpp"
#include "iceoryx_utils/platform/unistd.hpp"

#include <string>

class UDS : public IcePerfBase
{
  public:
    /// @brief Max message size is on linux = 4096 and on mac os = 2048. To have
    ///  the same behavior on every platform we use 2048.
#ifdef __APPLE__
    static constexpr uint32_t MAX_MESSAGE_SIZE = 2 * IcePerfBase::ONE_KILOBYTE;
#else
    static constexpr uint32_t MAX_MESSAGE_SIZE = 4 * IcePerfBase::ONE_KILOBYTE;
#endif

    static constexpr int32_t ERROR_CODE = -1;
    static constexpr int32_t INVALID_FD = -1;

    UDS(const std::string& publisherName, const std::string& subscriberName) noexcept;
    /// @brief Cleans up outdated sockets, e.g. from a previous test
    /// @attention only leader is allowed to call this
    static void cleanupOutdatedResources(const std::string& publisherName, const std::string& subscriberName) noexcept;

    void initLeader() noexcept override;
    void initFollower() noexcept override;
    void shutdown() noexcept override;

  private:
    static constexpr const char* PREFIX{"/tmp/"};
    void init() noexcept;
    void send(const char* buffer, uint32_t length) noexcept;
    void receive(char* buffer) noexcept;
    void waitForLeader() noexcept;
    void waitForFollower() noexcept;
    void sendPerfTopic(const uint32_t payloadSizeInBytes, const RunFlag runFlag) noexcept override;
    PerfTopic receivePerfTopic() noexcept override;

    static void initSocketAddress(sockaddr_un& sockAddr, const std::string& socketName);

    const std::string m_publisherSocketName;
    const std::string m_subscriberSocketName;
    int m_sockfdPublisher{INVALID_FD};
    int m_sockfdSubscriber{INVALID_FD};
    struct sockaddr_un m_sockAddrPublisher;
    struct sockaddr_un m_sockAddrSubscriber;
    char m_message[MAX_MESSAGE_SIZE];
};

#endif // IOX_EXAMPLES_ICEPERF_UDS_HPP
