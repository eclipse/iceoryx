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

std::atomic_bool keepRunning{true};
constexpr char APP_NAME[] = "iox-cpp-callbacks-listener-as-class-member";

iox::posix::Semaphore shutdownSemaphore =
    iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    shutdownSemaphore.post().or_else([](auto) {
        std::cerr << "unable to call post on shutdownSemaphore - semaphore corrupt?" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    keepRunning = false;
}

class CounterService
{
  public:
    CounterService()
        : m_subscriberLeft({"Radar", "FrontLeft", "Counter"})
        , m_subscriberRight({"Radar", "FrontRight", "Counter"})
    {
        /// Attach the static method onSampleReceivedCallback and provide this as additional argument
        /// to the callback to gain access to the object whenever the callback is called.
        /// It is not possible to use a lambda with capturing here since they are not convertable to
        /// a C function pointer.
        /// important: the user has to ensure that the contextData (*this) lives as long as
        ///            the subscriber with its callback is attached to the listener
        m_listener
            .attachEvent(m_subscriberLeft,
                         iox::popo::SubscriberEvent::DATA_RECEIVED,
                         iox::popo::createNotificationCallback(onSampleReceivedCallback, *this))
            .or_else([](auto) {
                std::cerr << "unable to attach subscriberLeft" << std::endl;
                std::exit(EXIT_FAILURE);
            });
        m_listener
            .attachEvent(m_subscriberRight,
                         iox::popo::SubscriberEvent::DATA_RECEIVED,
                         iox::popo::createNotificationCallback(onSampleReceivedCallback, *this))
            .or_else([](auto) {
                std::cerr << "unable to attach subscriberRight" << std::endl;
                std::exit(EXIT_FAILURE);
            });
    }

    void waitForShutdown() noexcept
    {
        shutdownSemaphore.wait().or_else(
            [](auto) { std::cerr << "unable to call wait on shutdownSemaphore - semaphore corrupt?" << std::endl; });
    }

  private:
    /// This method has to be static since only c functions are allowed as callback.
    /// To gain access to the members and methods of CounterClass we provide as an additional argument the this pointer
    /// which is stored in self
    static void onSampleReceivedCallback(iox::popo::Subscriber<CounterTopic>* subscriber, CounterService* self)
    {
        subscriber->take().and_then([subscriber, self](auto& sample) {
            auto instanceString = subscriber->getServiceDescription().getInstanceIDString();

            // store the sample in the corresponding cache
            if (instanceString == iox::capro::IdString_t("FrontLeft"))
            {
                self->m_leftCache.emplace(*sample);
            }
            else if (instanceString == iox::capro::IdString_t("FrontRight"))
            {
                self->m_rightCache.emplace(*sample);
            }

            std::cout << "received: " << sample->counter << std::endl;
        });

        // if both caches are filled we can process them
        if (self->m_leftCache && self->m_rightCache)
        {
            std::cout << "Received samples from FrontLeft and FrontRight. Sum of " << self->m_leftCache->counter
                      << " + " << self->m_rightCache->counter << " = "
                      << self->m_leftCache->counter + self->m_rightCache->counter << std::endl;
            self->m_leftCache.reset();
            self->m_rightCache.reset();
        }
    }

    iox::popo::Subscriber<CounterTopic> m_subscriberLeft;
    iox::popo::Subscriber<CounterTopic> m_subscriberRight;
    iox::cxx::optional<CounterTopic> m_leftCache;
    iox::cxx::optional<CounterTopic> m_rightCache;
    iox::popo::Listener m_listener;
};

int main()
{
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    CounterService counterService;

    counterService.waitForShutdown();

    return (EXIT_SUCCESS);
}
