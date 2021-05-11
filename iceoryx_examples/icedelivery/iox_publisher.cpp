// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
constexpr char APP_NAME[] = "iox-cpp-publisher";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

void getRadarObject(RadarObject* const object, const double& val) noexcept
{
    *object = RadarObject(val, val, val);
}

int main()
{
    // Register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    iox::popo::Publisher<RadarObject> publisher({"Radar", "FrontLeft", "Object"});

    double ct = 0.0;
    while (!killswitch)
    {
        ++ct;

        // API Usage #1
        //  * Retrieve a typed sample from shared memory.
        //  * Sample can be held until ready to publish.
        //  * Data is default constructed during loan
        publisher.loan()
            .and_then([&](auto& sample) {
                sample->x = ct + 89;
                sample->y = ct + 89;
                sample->z = ct + 89;
                sample.publish();
            })
            .or_else([](auto& error) {
                // Do something with error
                std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
            });


        // API Usage #2
        //  * Retrieve a typed sample from shared memory and construct data in-place
        //  * Sample can be held until ready to publish.
        //  * Data is constructed with the aruments provided.
        publisher.loan(ct + 144, ct + 144, ct + 144).and_then([](auto& sample) { sample.publish(); }).or_else([](auto& error) {
            // Do something with error
            std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
        });

        // API Usage #3
        //  * Basic copy-and-publish. Useful for smaller data types.
        //
        auto object = RadarObject(ct + 233, ct + 233, ct + 233);
        publisher.publishCopyOf(object).or_else([](auto& error) {
            // Do something with error.
            std::cerr << "Unable to publishCopyOf, error code: " << static_cast<uint64_t>(error) << std::endl;
        });

        // API Usage #4
        //  * Provide a callable that will be used to populate the loaned sample.
        //  * The first argument of the callable must be T* and is the location that the callable should
        //      write its result to.
        //
        publisher.publishResultOf(getRadarObject, ct).or_else([](auto& error) {
            // Do something with error.
            std::cerr << "Unable to publishResultOf, error code: " << static_cast<uint64_t>(error) << std::endl;
        });
        publisher.publishResultOf([&ct](RadarObject* object) { *object = RadarObject(ct + 377, ct + 377, ct + 377); })
            .or_else([](auto& error) {
                // Do something with error.
                std::cerr << "Unable to publishResultOf, error code: " << static_cast<uint64_t>(error) << std::endl;
            });


        std::cout << APP_NAME << " sent five times value: " << ct << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
