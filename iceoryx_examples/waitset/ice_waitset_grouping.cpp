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

#include "iceoryx_posh/popo/untyped_subscriber.hpp"
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
    constexpr uint64_t NUMBER_OF_SUBSCRIBERS = 4U;
    constexpr uint64_t ONE_SHUTDOWN_TRIGGER = 1U;

    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime("iox-cpp-waitset-grouping");
    iox::popo::WaitSet<NUMBER_OF_SUBSCRIBERS + ONE_SHUTDOWN_TRIGGER> waitset;

    // attach shutdownTrigger to handle CTRL+C
    waitset.attachEvent(shutdownTrigger).or_else([](auto) {
        std::cerr << "failed to attach shutdown trigger" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // create subscriber and subscribe them to our service
    iox::cxx::vector<iox::popo::UntypedSubscriber, NUMBER_OF_SUBSCRIBERS> subscriberVector;
    for (auto i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
    {
        subscriberVector.emplace_back(iox::capro::ServiceDescription{"Radar", "FrontLeft", "Counter"});
        auto& subscriber = subscriberVector.back();
        // Ignore unused variable warning
        (void)subscriber;
    }

    constexpr uint64_t FIRST_GROUP_ID = 123U;
    constexpr uint64_t SECOND_GROUP_ID = 456U;

    // attach the first two subscribers to waitset with a id of FIRST_GROUP_ID
    for (auto i = 0U; i < NUMBER_OF_SUBSCRIBERS / 2; ++i)
    {
        waitset.attachState(subscriberVector[i], iox::popo::SubscriberState::HAS_DATA, FIRST_GROUP_ID)
            .or_else([&](auto) {
                std::cerr << "failed to attach subscriber" << i << std::endl;
                std::exit(EXIT_FAILURE);
            });
    }

    // attach the remaining subscribers to waitset with a id of SECOND_GROUP_ID
    for (auto i = NUMBER_OF_SUBSCRIBERS / 2; i < NUMBER_OF_SUBSCRIBERS; ++i)
    {
        waitset.attachState(subscriberVector[i], iox::popo::SubscriberState::HAS_DATA, SECOND_GROUP_ID)
            .or_else([&](auto) {
                std::cerr << "failed to attach subscriber" << i << std::endl;
                std::exit(EXIT_FAILURE);
            });
    }

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
            // we print the received data for the first group
            else if (notification->getNotificationId() == FIRST_GROUP_ID)
            {
                auto subscriber = notification->getOrigin<iox::popo::UntypedSubscriber>();
                subscriber->take().and_then([&](auto& userPayload) {
                    const CounterTopic* data = static_cast<const CounterTopic*>(userPayload);
                    std::cout << "received: " << std::dec << data->counter << std::endl;
                    subscriber->release(userPayload);
                });
            }
            // dismiss the received data for the second group
            else if (notification->getNotificationId() == SECOND_GROUP_ID)
            {
                std::cout << "dismiss data\n";
                auto subscriber = notification->getOrigin<iox::popo::UntypedSubscriber>();
                // We need to release the data to reset the trigger hasData
                // otherwise the WaitSet would notify us in `waitset.wait()` again
                // instantly.
                subscriber->releaseQueuedData();
            }
        }

        std::cout << std::endl;
    }

    return (EXIT_SUCCESS);
}
