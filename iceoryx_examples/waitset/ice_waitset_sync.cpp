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

class SomeClass
{
  public:
    static void cyclicRun(iox::popo::UserTrigger*)
    {
        std::cout << "activation callback\n";
    }
};

int main()
{
    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime("iox-cpp-waitset-sync");
    std::atomic_bool keepRunning{true};

    iox::popo::WaitSet<> waitset;

    // attach shutdownTrigger to handle CTRL+C
    waitset.attachEvent(shutdownTrigger).or_else([](auto) {
        std::cerr << "failed to attach shutdown trigger" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // create and attach the cyclicTrigger with a callback to
    // SomeClass::myCyclicRun
    iox::popo::UserTrigger cyclicTrigger;
    waitset.attachEvent(cyclicTrigger, 0U, createNotificationCallback(SomeClass::cyclicRun)).or_else([](auto) {
        std::cerr << "failed to attach cyclic trigger" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // start a thread which triggers cyclicTrigger every second
    std::thread cyclicTriggerThread([&] {
        while (keepRunning.load())
        {
            cyclicTrigger.trigger();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // event loop
    while (keepRunning.load())
    {
        auto notificationVector = waitset.wait();

        for (auto& notification : notificationVector)
        {
            if (notification->doesOriginateFrom(&shutdownTrigger))
            {
                // CTRL+c was pressed -> exit
                keepRunning.store(false);
            }
            else
            {
                // call SomeClass::myCyclicRun
                (*notification)();
            }
        }

        std::cout << std::endl;
    }

    cyclicTriggerThread.join();
    return (EXIT_SUCCESS);
}
