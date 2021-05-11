// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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
#include <iostream>

bool killswitch = false;

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    killswitch = true;
}

void sending()
{
    iox::runtime::PoshRuntime::initRuntime("iox-cpp-publisher-waitset");
    iox::popo::Publisher<CounterTopic> myPublisher({"Radar", "FrontLeft", "Counter"});

    for (uint32_t counter = 0U; !killswitch; ++counter)
    {
        myPublisher.publishCopyOf(CounterTopic{counter})
            .and_then([&] { std::cout << "Sending: " << counter << std::endl; })
            .or_else([&](auto) { std::cout << "Failed sending: " << counter << std::endl; });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    myPublisher.stopOffer();
}

int main()
{
    auto signalGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    std::thread tx(sending);
    tx.join();

    return (EXIT_SUCCESS);
}
