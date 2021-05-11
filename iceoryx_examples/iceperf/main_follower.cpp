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

#include "iceperf_follower.hpp"

#include "iceoryx_utils/cxx/convert.hpp"
#include "iceoryx_utils/platform/getopt.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    constexpr option longOptions[] = {
        {"help", no_argument, nullptr, 'h'}, {"moo", required_argument, nullptr, 'm'}, {nullptr, 0, nullptr, 0}};

    // colon after shortOption means it requires an argument, two colons mean optional argument
    constexpr const char* shortOptions = "hm:";
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
            std::cout << "-m, --moo <intensity>             Prints 'Moo!' with the specified intensity" << std::endl;
            std::cout << "                                  range = '0' to '100'" << std::endl;
            std::cout << "                                  default = '0'" << std::endl;

            return EXIT_SUCCESS;
        case 'm':
        {
            constexpr decltype(EXIT_SUCCESS) MOO{EXIT_SUCCESS};

            uint64_t intensity{0U};
            if (!iox::cxx::convert::fromString(optarg, intensity))
            {
                std::cerr << "Could not parse 'intensity' paramater!" << std::endl;
                return EXIT_FAILURE;
            }
            if (intensity > 100)
            {
                std::cerr << "Too high moo 'intensity'!" << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "Moo";
            for (uint64_t i = 0; i < intensity; ++i)
            {
                std::cout << "o";
            }
            std::cout << "!" << std::endl;

            return MOO;
        }
        default:
            return EXIT_FAILURE;
        }
    }

    IcePerfFollower app;
    return app.run();
}
