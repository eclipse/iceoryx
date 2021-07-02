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

#ifndef IOX_POSH_FUZZTESTS_FUZZHELPER_HPP
#define IOX_POSH_FUZZTESTS_FUZZHELPER_HPP

#include "roudi_fuzz.hpp"
#include <memory>

/// @brief Class to implement some help methods for the fuzz wrapper
class FuzzHelper
{
  public:
    /// @brief Reads messages from stdin and writes them into a std::vector
    /// @param[in] message via stdin
    /// @param[out] std::vector containing std::strings of the messages from stdin. Each std::string in the vector
    /// is one line of stdin. This means that if there is one newline in stdin, there will be two std::strings, with two
    /// newlines, there will be three messages,...
    std::vector<std::string> getStdInMessages() const noexcept;

    /// @brief a shared Ptr to a RouDi thread which will be used to keep the thread alive until the message is
    /// processed by RouDi
    /// @param[out] a shated_ptr to RouDiFuzz which inherits from RouDi
    std::shared_ptr<RouDiFuzz> startRouDiThread() noexcept;

    /// @brief Splitted messages in allMessages are put together as one String. This is used for TOML parser for
    /// example because one message can contain newlines
    /// @param[in] std::vector containing several std::string messages which shall be sent to an interface
    /// @param[out] std::vector containing one std::string message
    std::vector<std::string> combineString(std::vector<std::string>& allMessages) noexcept;

    /// @brief A method to check if RouDi is alive. It checks if the UDS is available and then sends a default message
    /// to RouDi
    /// @param[out] Boolean value indicating if RouDi is available
    bool checkIsRouDiRunning() const noexcept;
};

#endif /*IOX_POSH_FUZZTESTS_FUZZHELPER_HPP*/
