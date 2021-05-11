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

#include "iceoryx_utils/cxx/vector.hpp"

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

std::atomic_bool killswitch{false};
constexpr char APP_NAME[] = "iox-cpp-publisher-vector";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

int main()
{
    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    // initialize runtime
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    // initialize publisher
    //! [create publisher]
    iox::popo::Publisher<iox::cxx::vector<double, 5>> publisher({"Radar", "FrontRight", "VectorData"});
    //! [create publisher]

    uint64_t ct = 0;
    // run until interrupted by Ctrl-C
    while (!killswitch)
    {
        publisher.loan()
            .and_then([&](auto& sample) {
                //! [vector emplace_back]
                for (uint64_t i = 0U; i < sample->capacity(); ++i)
                {
                    // we can omit the check of the return value since the loop doesn't exceed the capacity of the
                    // vector
                    sample->emplace_back(static_cast<double>(ct + i));
                }
                //! [vector emplace_back]

                sample.publish();
            })
            .or_else([](auto& error) {
                // do something with error
                std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
            });
        ++ct;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return (EXIT_SUCCESS);
}

