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

void shutdownCallback(iox::popo::UserTrigger*)
{
    std::cout << "CTRL+c pressed - exiting now" << std::endl;
}

// The callback of the event. Every callback must have an argument which is
// a pointer to the origin of the Trigger. In our case the event origin is
// the untyped subscriber.
void subscriberCallback(iox::popo::UntypedSubscriber* const subscriber, uint64_t* const sumOfAllSamples)
{
    while (subscriber->hasData())
    {
        subscriber->take().and_then([&](auto& userPayload) {
            auto chunkHeader = iox::mepoo::ChunkHeader::fromUserPayload(userPayload);
            std::cout << "subscriber: " << std::hex << subscriber << " length: " << std::dec
                      << chunkHeader->userPayloadSize() << " ptr: " << std::hex << chunkHeader->userPayload()
                      << std::endl;
        });
        // no nullptr check required since it is guaranteed != nullptr
        ++(*sumOfAllSamples);
    }
}

int main()
{
    constexpr uint64_t NUMBER_OF_SUBSCRIBERS = 2U;
    constexpr uint64_t ONE_SHUTDOWN_TRIGGER = 1U;

    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime("iox-cpp-waitset-gateway");

    iox::popo::WaitSet<NUMBER_OF_SUBSCRIBERS + ONE_SHUTDOWN_TRIGGER> waitset;

    // attach shutdownTrigger to handle CTRL+C
    waitset.attachEvent(shutdownTrigger, iox::popo::createNotificationCallback(shutdownCallback)).or_else([](auto) {
        std::cerr << "failed to attach shutdown trigger" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    uint64_t sumOfAllSamples = 0U;

    // create subscriber and subscribe them to our service
    iox::cxx::vector<iox::popo::UntypedSubscriber, NUMBER_OF_SUBSCRIBERS> subscriberVector;
    for (auto i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
    {
        subscriberVector.emplace_back(iox::capro::ServiceDescription{"Radar", "FrontLeft", "Counter"});
        auto& subscriber = subscriberVector.back();

        /// important: the user has to ensure that the contextData (sumOfAllSamples) lives as long as
        ///            the subscriber with its callback is attached to the listener
        waitset
            .attachEvent(subscriber,
                         iox::popo::SubscriberEvent::DATA_RECEIVED,
                         0,
                         createNotificationCallback(subscriberCallback, sumOfAllSamples))
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
            else
            {
                // call the callback which was assigned to the notification
                (*notification)();
            }
        }

        std::cout << "sum of all samples: " << std::dec << sumOfAllSamples << std::endl;
    }

    return (EXIT_SUCCESS);
}
