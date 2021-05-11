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

//! [iceoryx includes]
#include "user_header_and_payload_types.hpp"

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
//! [iceoryx includes]

#include <atomic>
#include <iostream>

//! [signal handling]
std::atomic<bool> killswitch{false};

static void sigHandler(int sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}
//! [signal handling]

int main()
{
    //! [register sigHandler]
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);
    //! [register sigHandler]

    //! [initialize runtime]
    constexpr char APP_NAME[] = "iox-cpp-user-header-subscriber";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create subscriber]
    // for the 1.0 release, the Subscriber alias for the SubscriberImpl does not have the second parameter for the
    // Header, therefore the SubscriberImpl must be used directly
    iox::popo::Subscriber<Data, Header> subscriber({"Example", "User-Header", "Timestamp"});
    //! [create subscriber]

    //! [poll subscriber for samples in a loop]
    while (!killswitch)
    {
        //! [take sample]
        subscriber.take().and_then([&](auto& sample) {
            std::cout << APP_NAME << " got value: " << sample->fibonacci << " with timestamp "
                      << sample.getUserHeader().publisherTimestamp << "ms" << std::endl;
        });
        //! [take sample]

        constexpr std::chrono::milliseconds SLEEP_TIME{100U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    //! [poll subscriber for samples in a loop]

    return EXIT_SUCCESS;
}
