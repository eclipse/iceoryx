// Copyright (c) 2020 by Robert Bosch GmbH All rights reserved.
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

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"
#include "iceoryx_posh/runtime/posh_runtime_single_process.hpp"
#include "iceoryx_utils/log/logmanager.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>

std::atomic_bool keepRunning{true};

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    keepRunning = false;
}

struct TransmissionData_t
{
    uint64_t counter;
};

void consoleOutput(const std::string& output)
{
    static std::mutex consoleOutputMutex;

    std::lock_guard<std::mutex> lock(consoleOutputMutex);
    std::cout << output << std::endl;
}

void publisher()
{
    iox::popo::PublisherOptions publisherOptions;
    publisherOptions.historyCapacity = 10U;
    iox::popo::Publisher<TransmissionData_t> publisher({"Single", "Process", "Demo"}, publisherOptions);

    uint64_t counter{0};
    std::string greenRightArrow("\033[32m->\033[m ");
    while (keepRunning.load())
    {
        publisher.loan().and_then([&](auto& sample) {
            sample->counter = counter++;
            consoleOutput(std::string("Sending   " + greenRightArrow + std::to_string(sample->counter)));
            sample.publish();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void subscriber()
{
    iox::popo::SubscriberOptions options;
    options.queueCapacity = 10U;
    options.historyRequest = 5U;
    iox::popo::Subscriber<TransmissionData_t> subscriber({"Single", "Process", "Demo"}, options);

    std::string orangeLeftArrow("\033[33m<-\033[m ");
    while (keepRunning.load())
    {
        if (iox::SubscribeState::SUBSCRIBED == subscriber.getSubscriptionState())
        {
            bool hasMoreSamples{true};

            do
            {
                subscriber.take()
                    .and_then([&](auto& sample) {
                        consoleOutput(std::string("Receiving " + orangeLeftArrow + std::to_string(sample->counter)));
                    })
                    .or_else([&](auto& result) {
                        hasMoreSamples = false;
                        if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                        {
                            std::cout << "Error receiving chunk." << std::endl;
                        }
                    });
            } while (hasMoreSamples);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    // Register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    // set the log level to error to see the essence of the example
    iox::log::LogManager::GetLogManager().SetDefaultLogLevel(iox::log::LogLevel::kError);

    iox::RouDiConfig_t defaultRouDiConfig = iox::RouDiConfig_t().setDefaults();
    iox::roudi::IceOryxRouDiComponents roudiComponents(defaultRouDiConfig);

    iox::roudi::RouDi roudi(roudiComponents.rouDiMemoryManager,
                            roudiComponents.portManager,
                            iox::roudi::RouDi::RoudiStartupParameters{iox::roudi::MonitoringMode::OFF, false});

    // create a single process runtime for inter thread communication
    iox::runtime::PoshRuntimeSingleProcess runtime("singleProcessDemo");

    std::thread publisherThread(publisher), subscriberThread(subscriber);

    // communicate for 2 seconds and then stop the example
    std::this_thread::sleep_for(std::chrono::seconds(2));
    keepRunning.store(false);

    publisherThread.join();
    subscriberThread.join();

    std::cout << "Finished" << std::endl;
}
