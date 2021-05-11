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

#include "topic_data.hpp"

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <iostream>

bool killswitch = false;
constexpr char APP_NAME[] = "iox-cpp-publisher-helloworld";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

int main()
{
    // Register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    iox::popo::Publisher<RadarObject> publisher({"Radar", "FrontLeft", "Object"});

    double ct = 0.0;
    while (!killswitch)
    {
        ++ct;

        // Retrieve a sample from shared memory
        auto loanResult = publisher.loan();
        if (!loanResult.has_error())
        {
            auto& sample = loanResult.value();
            // Sample can be held until ready to publish
            sample->x = ct;
            sample->y = ct;
            sample->z = ct;
            sample.publish();
        }
        else
        {
            auto error = loanResult.get_error();
            // Do something with error
            std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
        }

        std::cout << APP_NAME << " sent value: " << ct << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
