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

#include "fuzz_helper.hpp"
#include "fuzzing.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"


std::vector<std::string> FuzzHelper::getStdInMessages() const noexcept
{
    std::vector<std::string> stdInMessages;
    for (std::string line; std::getline(std::cin, line);)
    {
        stdInMessages.push_back(line);
    }
    return stdInMessages;
}

std::shared_ptr<RouDiFuzz> FuzzHelper::startRouDiThread() noexcept
{
    static iox::roudi::IceOryxRouDiComponents m_rouDiComponents(iox::RouDiConfig_t().setDefaults());
    static iox::RouDiConfig_t m_config = iox::RouDiConfig_t().setDefaults();
    std::shared_ptr<RouDiFuzz> aRouDi(
        new RouDiFuzz(m_rouDiComponents.rouDiMemoryManager, m_rouDiComponents.portManager));
    return aRouDi;
}

bool FuzzHelper::checkIsRouDiRunning() const noexcept
{
    return Fuzzing().fuzzingRouDiUDS("Hello Roudi!");
}

std::vector<std::string> FuzzHelper::combineString(std::vector<std::string>& allMessages) noexcept
{
    std::string tempString = "";
    for (std::string aMessage : allMessages)
    {
        tempString += aMessage + "\n";
    }
    allMessages.clear();
    allMessages.emplace_back(tempString);
    return allMessages;
}
