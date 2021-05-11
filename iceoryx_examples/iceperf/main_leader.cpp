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

#include "example_common.hpp"
#include "iceperf_leader.hpp"

#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/convert.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/platform/getopt.hpp"

#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
    PerfSettings settings;

    constexpr option longOptions[] = {{"help", no_argument, nullptr, 'h'},
                                      {"benchmark", required_argument, nullptr, 'b'},
                                      {"technology", required_argument, nullptr, 't'},
                                      {"number-of-samples", required_argument, nullptr, 't'},
                                      {nullptr, 0, nullptr, 0}};

    // colon after shortOption means it requires an argument, two colons mean optional argument
    constexpr const char* shortOptions = "hb:t:n:";
    int32_t index{0};
    int32_t opt{-1};
    while ((opt = getopt_long(argc, argv, shortOptions, longOptions, &index), opt != -1))
    {
        switch (opt)
        {
        case 'h':
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "-h, --help                        Display help" << std::endl;
            std::cout << "-b, --benchmark <TYPE>            Selects the type of benchmark to run" << std::endl;
            std::cout << "                                  <TYPE> {all, latency, throughput}" << std::endl;
            std::cout << "                                  default = 'all'" << std::endl;
            std::cout << "-t, --technology <TYPE>           Selects the type of technology to benchmark" << std::endl;
            std::cout << "                                  <TYPE> {all," << std::endl;
            std::cout << "                                          iceoryx-cpp-api," << std::endl;
            std::cout << "                                          iceoryx-c-api," << std::endl;
            std::cout << "                                          posix-message-queue," << std::endl;
            std::cout << "                                          unix-domain-sockets}" << std::endl;
            std::cout << "                                  default = 'all'" << std::endl;
            std::cout << "-n, --number-of-samples <N>       Set the number of samples sent in a benchmark round"
                      << std::endl;
            std::cout << "                                  default = '10000'" << std::endl;

            return EXIT_SUCCESS;
        case 'b':
            if (strcmp(optarg, "all") == 0)
            {
                settings.benchmark = Benchmark::ALL;
            }
            else if (strcmp(optarg, "latency") == 0)
            {
                settings.benchmark = Benchmark::LATENCY;
            }
            else if (strcmp(optarg, "throughput") == 0)
            {
                settings.benchmark = Benchmark::THROUGHPUT;
            }
            else
            {
                std::cerr << "Options for 'benchmark' are 'all', 'latency' and 'off'!" << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case 't':
            if (strcmp(optarg, "all") == 0)
            {
                settings.technology = Technology::ALL;
            }
            else if (strcmp(optarg, "iceoryx-cpp-api") == 0)
            {
                settings.technology = Technology::ICEORYX_CPP_API;
            }
            else if (strcmp(optarg, "iceoryx-c-api") == 0)
            {
                settings.technology = Technology::ICEORYX_C_API;
            }
            else if (strcmp(optarg, "posix-message-queue") == 0)
            {
                settings.technology = Technology::POSIX_MESSAGE_QUEUE;
            }
            else if (strcmp(optarg, "unix-domain-sockets") == 0)
            {
                settings.technology = Technology::UNIX_DOMAIN_SOCKET;
            }
            else
            {
                std::cerr << "Options for 'technology' are 'all', 'iceoryx-cpp-api', 'iceoryx-c-api', "
                             "'posix-message-queue' and 'unix-domain-sockets'!"
                          << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case 'n':
            if (!iox::cxx::convert::fromString(optarg, settings.numberOfSamples))
            {
                std::cerr << "Could not parse 'number-of-samples' paramater!" << std::endl;
                return EXIT_FAILURE;
            }
            break;
        default:
            return EXIT_FAILURE;
        };
    }

    IcePerfLeader app(settings);
    return app.run();
}
