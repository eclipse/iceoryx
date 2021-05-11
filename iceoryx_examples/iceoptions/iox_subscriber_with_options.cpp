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

#include "topic_data.hpp"

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <iostream>

bool killswitch = false;
constexpr char APP_NAME[] = "iox-cpp-subscriber-with-options";

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

    // create subscriber with some options set
    iox::popo::SubscriberOptions subscriberOptions;

    // the queue can hold 10 samples, on overflow the oldest sample will be replaced with the new arriving one
    subscriberOptions.queueCapacity = 10U;

    // When starting the subscriber late it will miss the first samples which the
    // publisher has send. The history request ensures that we at least get the last 5
    // samples sent by the publisher when we subscribe (if at least 5 were already sent
    // and the publisher has history enabled).
    subscriberOptions.historyRequest = 5U;

    // when the subscriber is created, no attempts are made to connect to any publishers that may exist
    subscriberOptions.subscribeOnCreate = false;

    // grouping of publishers and subscribers within a process
    subscriberOptions.nodeName = "Sub_Node_With_Options";

    // we request the publisher to wait for space in the queue if it is full. The publisher will be blocked then
    subscriberOptions.queueFullPolicy = iox::popo::QueueFullPolicy::BLOCK_PUBLISHER;

    iox::popo::Subscriber<RadarObject> subscriber({"Radar", "FrontLeft", "Object"}, subscriberOptions);

    // We have to explicitly call subscribe() otherwise the subscriber will not try to connect to publishers
    subscriber.subscribe();

    // run until interrupted by Ctrl-C
    while (!killswitch)
    {
        subscriber.take().and_then(
            [](auto& sample) { std::cout << APP_NAME << " got value: " << sample->x << std::endl; });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    subscriber.unsubscribe();

    return (EXIT_SUCCESS);
}
