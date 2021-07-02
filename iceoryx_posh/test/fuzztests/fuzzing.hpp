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

#ifndef IOX_POSH_FUZZTESTS_FUZZING_HPP
#define IOX_POSH_FUZZTESTS_FUZZING_HPP

#include "roudi_fuzz.hpp"
#include <memory>
#include <string>

static constexpr int32_t ERROR_CODE = -1;

/// @brief Fuzzing is a class which is used to send a message to different interfaces which shall be fuzzed.
class Fuzzing
{
  public:
    /// @brief fuzzingRouDiCom is a method to test the processMessage method of RouDi. It shall be independent from
    /// the underlying protocol such as uds. It should also be slightly faster since some functions are not invoked
    /// compared to uds fuzzing. However, a RouDi thread is also started with this approach because otherwise it was not
    /// be possible to invoke the processMessage method within RouDi without directly modifying the code in RouDi.
    /// @param[in] shared_ptr to RouDiFuzz such that processMessage can be invoked
    /// @param[in] std::string containing a message which shall be sent to the interface
    void fuzzingRouDiCom(const std::shared_ptr<RouDiFuzz> aRouDi, const std::string aMessage) noexcept;

    /// @brief fuzzingRouDiUDS is a method to test the Unix Domain Socket interface of RouDi. It connects to
    /// RouDi's uds and sends the message given as input to RouDi
    /// @param[in] std::string containing a message which shall be sent to the interface
    /// @param[out] bool as result of connect(). If the result is false it means that it was not possible to
    /// connect to the socket
    bool fuzzingRouDiUDS(const std::string aMessage) noexcept;

    /// @brief fuzzingTOMLParser is a method to send a message to the TOML parser.
    /// @param[in] std::string containing a message which shall be sent to the interface
    /// @param[in] std::string containing a valid path to an empty file which will be used to write the tomlFile into.
    /// This is necessary because the TOML parser expects a path to a file as input
    /// @param[out] bool as result if the TOML file could be sent to the parser
    bool fuzzingTOMLParser(const std::string tomlFile, const std::string tempFile) noexcept;

  private:
    int32_t m_sockfd{iox::posix::UnixDomainSocket::INVALID_FD};
    int32_t m_connectfd{iox::posix::UnixDomainSocket::INVALID_FD};
};
#endif /* IOX_POSH_FUZZTESTS_FUZZING_HPP */
