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

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
#include "topic_data.hpp"

#include <chrono>
#include <csignal>
#include <iostream>

bool keepRunning = true;
constexpr char APP_NAME[] = "iox-cpp-callbacks-publisher";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    keepRunning = false;
}

void sending()
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    iox::popo::Publisher<CounterTopic> myPublisherLeft({"Radar", "FrontLeft", "Counter"});
    iox::popo::Publisher<CounterTopic> myPublisherRight({"Radar", "FrontRight", "Counter"});

    for (uint32_t counter = 0U; keepRunning; ++counter)
    {
        if (counter % 3 == 0)
        {
            std::cout << "Radar.FrontLeft.Counter sending : " << counter << std::endl;
            myPublisherLeft.publishCopyOf(CounterTopic{counter}).or_else([](auto) {
                std::cerr << "Radar.FrontLeft.Counter send failed\n";
            });
        }
        else
        {
            std::cout << "Radar.FrontRight.Counter sending : " << counter * 2 << std::endl;
            myPublisherRight.publishCopyOf(CounterTopic{counter * 2}).or_else([](auto) {
                std::cerr << "Radar.FrontRight.Counter send failed\n";
            });
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    sending();

    return (EXIT_SUCCESS);
}
