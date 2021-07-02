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

#include "cmdlineparserfuzzing.hpp"
#include "fuzz_helper.hpp"
#include "fuzzing.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"

#include <fstream>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) noexcept
{
    constexpr uint8_t MAX_RETRIES = 50;
    constexpr uint64_t WAIT_RETRY_IN_MS = 100;
    CmdLineParserFuzzing cmd;
    std::vector<std::string> allMessages = cmd.parseCmd(argc, argv);

    if (cmd.getHelpFlag())
    {
        return EXIT_SUCCESS;
    }

    if (cmd.getErrorFlag())
    {
        iox::LogError() << "No or wrong command lines were specified. Please use --help!";
        return EXIT_FAILURE;
    }

    if (allMessages.empty())
    {
        iox::LogError()
            << "Please use -m [cl, stdin] to enter the input you want to send to the executable. If you use -m "
               "cl, then you also need use -i [INPUT_MESSAGE] or -c [PATH_To_File] to specify the message.";
        return EXIT_FAILURE;
    }

    if (cmd.getInputMode() == InputMode::NONE)
    {
        iox::LogError() << "Please use -m to specify the input. Please use --help to get more information.";
        return EXIT_FAILURE;
    }

    if ((cmd.getInputMode() == InputMode::CL) && (!cmd.getCmdLineFlag()))
    {
        iox::LogError()
            << "Please use -i [INPUT_MESSAGE] or -c [PATH_To_File] to enter a String which you want to send to "
               "the interface. It is also possible to use -m stdin instead.";
        return EXIT_FAILURE;
    }

    FuzzHelper aFuzzHelper;
    std::shared_ptr<RouDiFuzz> aRouDi;

    if (cmd.getFuzzingAPI() == FuzzingApi::TOML)
    {
        if (!cmd.getTomlFileFlag())
        {
            iox::LogError()
                << "Please use -t [PATH_To_File] to specify a file where the messages are written to which are "
                   "sent to the TOML configuration parser.";
            return EXIT_FAILURE;
        }
        else
        {
            allMessages = aFuzzHelper.combineString(allMessages);
        }
    }

    if ((cmd.getFuzzingAPI() == FuzzingApi::UDS) || (cmd.getFuzzingAPI() == FuzzingApi::COM)) // Start RouDi
    {
        aRouDi = aFuzzHelper.startRouDiThread();
        if ((cmd.getFuzzingAPI() == FuzzingApi::UDS))
        {
            uint8_t retryCounter{0};
            while (!aFuzzHelper.checkIsRouDiRunning())
            {
                if (retryCounter >= MAX_RETRIES)
                {
                    iox::LogError() << "RouDi could not be started, program terminates!";
                    return EXIT_FAILURE;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RETRY_IN_MS));
                retryCounter++;
            }
        }
    }

    if ((cmd.getInputMode() == InputMode::CL) || (cmd.getInputMode() == InputMode::STDIN))
    {
        Fuzzing aFuzzer;
        for (std::string aMessage : allMessages)
        {
            switch (cmd.getFuzzingAPI())
            {
            case FuzzingApi::COM:
            {
                iox::LogDebug() << "Messages sent to RouDi: " << aMessage;
                aFuzzer.fuzzingRouDiCom(aRouDi, aMessage);
                break;
            }

            case FuzzingApi::UDS:
            {
                bool hasConnect = aFuzzer.fuzzingRouDiUDS(aMessage);
                if (!hasConnect)
                {
                    iox::LogError() << "Could not connect to the UDS socket";
                }
                iox::LogDebug() << "Messages sent to RouDi: " << aMessage;
                break;
            }

            case FuzzingApi::TOML:
            {
                aFuzzer.fuzzingTOMLParser(aMessage, cmd.getTomlFile());
                iox::LogDebug() << "Messages sent to TOML Parser: " << aMessage;
                break;
            }

            default:
            {
                iox::LogError() << "Error: Unkown Fuzzing API parameter";
                return EXIT_FAILURE;
            }
            };
        }
    }
    else
    {
        iox::LogError()
            << "Error: Only stdin and command line are allowed to enter an input. Please use --help to get more "
               "information.";
        return EXIT_FAILURE;
    }
}
