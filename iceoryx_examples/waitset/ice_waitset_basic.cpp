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

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
#include "topic_data.hpp"

#include <atomic>
#include <iostream>

std::atomic_bool shutdown{false};
iox::cxx::optional<iox::popo::WaitSet<>> waitset;

static void sigHandler(int sig IOX_MAYBE_UNUSED)
{
    shutdown = true;
    if (waitset)
    {
        waitset->markForDestruction();
    }
}

int main()
{
    // initialize runtime
    iox::runtime::PoshRuntime::initRuntime("iox-cpp-waitset-basic");

    // create waitset inside of the optional
    waitset.emplace();

    // register signal handler to handle termination of the loop
    auto signalGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    // create subscriber
    iox::popo::Subscriber<CounterTopic> subscriber({"Radar", "FrontLeft", "Counter"});

    // attach subscriber to waitset
    waitset->attachState(subscriber, iox::popo::SubscriberState::HAS_DATA).or_else([](auto) {
        std::cerr << "failed to attach subscriber" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    while (!shutdown.load())
    {
        // We block and wait for samples to arrive.
        auto notificationVector = waitset->wait();

        for (auto& notification : notificationVector)
        {
            // We woke up and hence there must be at least one sample. When the sigHandler has called
            // markForDestruction the notificationVector is empty otherwise we know which subscriber received samples
            // since we only attached one.
            // Best practice is to always acquire the notificationVector and iterate over all elements and then react
            // accordingly. When this is not done and more elements are attached to the WaitSet it can cause
            // problems since we either miss events or handle events for objects which never occurred.
            if (notification->doesOriginateFrom(&subscriber))
            {
                // Consume a sample
                subscriber.take()
                    .and_then([](auto& sample) { std::cout << " got value: " << sample->counter << std::endl; })
                    .or_else([](auto& reason
                                    IOX_MAYBE_UNUSED) { /* we could check and handle the reason why there is no data */
                                                        std::cout << "got no data" << std::endl;
                    });
                // We could consume all samples but do not need to.
                // If there is more than one sample we will wake up again since the state of the subscriber is still
                // iox::popo::SubscriberState::HAS_DATA in this case.
            }
        }
    }

    std::cout << "shutting down" << std::endl;

    return (EXIT_SUCCESS);
}
