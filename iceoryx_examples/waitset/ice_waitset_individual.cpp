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

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
#include "topic_data.hpp"

#include <chrono>
#include <iostream>

iox::popo::UserTrigger shutdownTrigger;

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    shutdownTrigger.trigger();
}

int main()
{
    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime("iox-cpp-waitset-individual");

    iox::popo::WaitSet<> waitset;

    // attach shutdownTrigger to handle CTRL+C
    waitset.attachEvent(shutdownTrigger).or_else([](auto) {
        std::cerr << "failed to attach shutdown trigger" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // create two subscribers, subscribe to the service and attach them to the waitset
    iox::popo::Subscriber<CounterTopic> subscriber1({"Radar", "FrontLeft", "Counter"});
    iox::popo::Subscriber<CounterTopic> subscriber2({"Radar", "FrontLeft", "Counter"});

    waitset.attachState(subscriber1, iox::popo::SubscriberState::HAS_DATA).or_else([](auto) {
        std::cerr << "failed to attach subscriber1" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    waitset.attachState(subscriber2, iox::popo::SubscriberState::HAS_DATA).or_else([](auto) {
        std::cerr << "failed to attach subscriber2" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // event loop
    while (true)
    {
        auto notificationVector = waitset.wait();

        for (auto& notification : notificationVector)
        {
            if (notification->doesOriginateFrom(&shutdownTrigger))
            {
                // CTRL+c was pressed -> exit
                return (EXIT_SUCCESS);
            }
            // process sample received by subscriber1
            else if (notification->doesOriginateFrom(&subscriber1))
            {
                subscriber1.take().and_then(
                    [&](auto& sample) { std::cout << " subscriber 1 received: " << sample->counter << std::endl; });
            }
            // dismiss sample received by subscriber2
            if (notification->doesOriginateFrom(&subscriber2))
            {
                // We need to release the samples to reset the trigger hasSamples
                // otherwise the WaitSet would notify us in `waitset.wait()` again
                // instantly.
                subscriber2.releaseQueuedData();
                std::cout << "subscriber 2 received something - dont care\n";
            }
        }

        std::cout << std::endl;
    }

    return (EXIT_SUCCESS);
}
