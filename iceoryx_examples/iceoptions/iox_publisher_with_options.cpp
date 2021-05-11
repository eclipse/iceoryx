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

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <iostream>

bool killswitch = false;
constexpr char APP_NAME[] = "iox-cpp-publisher-with-options";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
    // this is optional, but since the iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER option is used,
    // a slow subscriber might block the shutdown and this call unblocks the publisher
    iox::runtime::PoshRuntime::getInstance().shutdown();
}

int main()
{
    // Register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    // create publisher with some options set
    iox::popo::PublisherOptions publisherOptions;

    // the publishers stores the last 10 samples for possible late joiners
    publisherOptions.historyCapacity = 10U;

    // when the publisher is created, it is not yet visible
    publisherOptions.offerOnCreate = false;

    // grouping of publishers and subscribers within a process
    publisherOptions.nodeName = "Pub_Node_With_Options";

    //  we allow the subscribers to block the publisher if they want to ensure that no samples are lost
    publisherOptions.subscriberTooSlowPolicy = iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER;

    iox::popo::Publisher<RadarObject> publisher({"Radar", "FrontLeft", "Object"}, publisherOptions);

    // we have to explicitely offer the publisher for making it visible to subscribers
    publisher.offer();

    double ct = 0.0;
    while (!killswitch)
    {
        ++ct;

        // Retrieve a sample, construct it with the given arguments and publish it via a lambda.
        publisher.loan(ct, ct, ct).and_then([](auto& sample) { sample.publish(); });

        std::cout << APP_NAME << " sent value: " << ct << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    return 0;
}
