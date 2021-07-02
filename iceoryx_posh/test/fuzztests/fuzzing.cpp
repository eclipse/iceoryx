// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "fuzzing.hpp"
#include "cpptoml.h"
#include "iceoryx_hoofs/platform/socket.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"

#include <fstream>
#include <thread>

constexpr char UDS_NAME[] = "/tmp/";

void Fuzzing::fuzzingRouDiCom(const std::shared_ptr<RouDiFuzz> aRouDi, const std::string aMessage) noexcept
{
    if (aRouDi != nullptr)
    {
        aRouDi->processMessageFuzz(aMessage);
    }
    else
    {
        iox::LogDebug()
            << "Error, the Smart Pointer for RouDi which is used to call the method 'processMessage' is NULL";
    }
}

bool Fuzzing::fuzzingRouDiUDS(const std::string aMessage) noexcept
{
    bool conSuccess = false;

    auto socketCall = iox::posix::posixCall(socket)(AF_LOCAL, SOCK_DGRAM, 0)
                          .failureReturnValue(ERROR_CODE)
                          .evaluate()
                          .and_then([this](auto& r) { m_sockfd = r.value; });

    if (socketCall.has_error())
    {
        iox::LogError() << "A socket could not be created.";
        return conSuccess;
    }

    struct sockaddr aSockAddr;
    aSockAddr.sa_family = AF_LOCAL;
    std::string roudiName = std::string(UDS_NAME) + iox::roudi::IPC_CHANNEL_ROUDI_NAME;
    strncpy(aSockAddr.sa_data, roudiName.c_str(), sizeof(aSockAddr.sa_data));

    auto connectCall =
        iox::posix::posixCall(connect)(m_sockfd, reinterpret_cast<struct sockaddr*>(&aSockAddr), sizeof(aSockAddr))
            .failureReturnValue(ERROR_CODE)
            .suppressErrorMessagesForErrnos(ENOENT)
            .evaluate()
            .and_then([&](auto& r) { m_connectfd = r.value; });

    if (connectCall.has_error() || m_connectfd == -1)
    {
        iox::LogError() << "Connecting to RouDi via UDS was not successful.";
    }
    else
    {
        conSuccess = true;
        iox::posix::posixCall(sendto)(
            m_sockfd, aMessage.c_str(), aMessage.length() + 1, static_cast<int>(0), nullptr, static_cast<socklen_t>(0))
            .failureReturnValue(ERROR_CODE)
            .suppressErrorMessagesForErrnos(ENOENT)
            .evaluate()
            .and_then([](auto&) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(500)); // 0.5  second We need to wait otherwise it can happen that
                                                     // RouDi did not process the message
            })
            .or_else([&](auto&) {
                iox::LogError() << "Sending the message " << aMessage << " to RouDi via UDS was not successful.";
            });
    }

    iox::posix::posixCall(iox_close)(m_sockfd).failureReturnValue(ERROR_CODE).evaluate().or_else([](auto&) {
        iox::LogError() << "Socket could not be closed.";
    });

    return conSuccess;
}


bool Fuzzing::fuzzingTOMLParser(const std::string aMessage, const std::string tempFile) noexcept
{
    std::ofstream aTomlFile;
    aTomlFile.open(tempFile);
    iox::LogDebug() << "Sent to TOML: " << aMessage;
    if (aTomlFile.is_open())
    {
        aTomlFile << aMessage;
        cpptoml::parse_file(tempFile);
        aTomlFile.close();
        return true;
    }
    else
    {
        iox::LogDebug() << "Cannot open file to send it to TOML Parser: " << tempFile;
        return false;
    }
}
