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

#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
#include "topic_data.hpp"

#include <chrono>
#include <csignal>
#include <iostream>

iox::posix::Semaphore shutdownSemaphore =
    iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();

std::atomic_bool keepRunning{true};
constexpr char APP_NAME[] = "iox-cpp-callbacks-subscriber";

iox::cxx::optional<CounterTopic> leftCache;
iox::cxx::optional<CounterTopic> rightCache;

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    shutdownSemaphore.post().or_else([](auto) {
        std::cerr << "unable to call post on shutdownSemaphore - semaphore corrupt?" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    keepRunning = false;
}

void heartbeatCallback(iox::popo::UserTrigger*)
{
    std::cout << "heartbeat received " << std::endl;
}

void onSampleReceivedCallback(iox::popo::Subscriber<CounterTopic>* subscriber)
{
    subscriber->take().and_then([subscriber](auto& sample) {
        auto instanceString = subscriber->getServiceDescription().getInstanceIDString();

        // store the sample in the corresponding cache
        if (instanceString == iox::capro::IdString_t("FrontLeft"))
        {
            leftCache.emplace(*sample);
        }
        else if (instanceString == iox::capro::IdString_t("FrontRight"))
        {
            rightCache.emplace(*sample);
        }

        std::cout << "received: " << sample->counter << std::endl;
    });

    // if both caches are filled we can process them
    if (leftCache && rightCache)
    {
        std::cout << "Received samples from FrontLeft and FrontRight. Sum of " << leftCache->counter << " + "
                  << rightCache->counter << " = " << leftCache->counter + rightCache->counter << std::endl;
        leftCache.reset();
        rightCache.reset();
    }
}

int main()
{
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    // the listener starts a background thread and the callbacks of the attached events
    // will be called in this background thread when they are triggered
    iox::popo::Listener listener;

    iox::popo::UserTrigger heartbeat;
    iox::popo::Subscriber<CounterTopic> subscriberLeft({"Radar", "FrontLeft", "Counter"});
    iox::popo::Subscriber<CounterTopic> subscriberRight({"Radar", "FrontRight", "Counter"});

    // send a heartbeat every 4 seconds
    std::thread heartbeatThread([&] {
        while (keepRunning)
        {
            heartbeat.trigger();
            std::this_thread::sleep_for(std::chrono::seconds(4));
        }
    });

    // attach everything to the listener, from here on the callbacks are called when the corresponding event is occuring
    listener.attachEvent(heartbeat, iox::popo::createNotificationCallback(heartbeatCallback)).or_else([](auto) {
        std::cerr << "unable to attach heartbeat event" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    // It is possible to attach any c function here with a signature of void(iox::popo::Subscriber<CounterTopic> *).
    // But please be aware that the listener does not take ownership of the callback, therefore it has to exist as
    // long as the event is attached. Furthermore, it excludes lambdas which are capturing data since they are not
    // convertable to a c function pointer.
    // to simplify the example we attach the same callback onSampleReceivedCallback again
    listener
        .attachEvent(subscriberLeft,
                     iox::popo::SubscriberEvent::DATA_RECEIVED,
                     iox::popo::createNotificationCallback(onSampleReceivedCallback))
        .or_else([](auto) {
            std::cerr << "unable to attach subscriberLeft" << std::endl;
            std::exit(EXIT_FAILURE);
        });
    listener
        .attachEvent(subscriberRight,
                     iox::popo::SubscriberEvent::DATA_RECEIVED,
                     iox::popo::createNotificationCallback(onSampleReceivedCallback))
        .or_else([](auto) {
            std::cerr << "unable to attach subscriberRight" << std::endl;
            std::exit(EXIT_FAILURE);
        });

    // wait until someone presses CTRL+c
    shutdownSemaphore.wait().or_else(
        [](auto) { std::cerr << "unable to call wait on shutdownSemaphore - semaphore corrupt?" << std::endl; });

    // optional detachEvent, but not required.
    //   when the listener goes out of scope it will detach all events and when a
    //   subscriber goes out of scope it will detach itself from the listener
    listener.detachEvent(heartbeat);
    listener.detachEvent(subscriberLeft, iox::popo::SubscriberEvent::DATA_RECEIVED);
    listener.detachEvent(subscriberRight, iox::popo::SubscriberEvent::DATA_RECEIVED);

    heartbeatThread.join();

    return (EXIT_SUCCESS);
}
