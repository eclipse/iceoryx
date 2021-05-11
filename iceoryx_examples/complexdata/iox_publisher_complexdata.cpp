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

#include "topic_data.hpp"

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

std::atomic_bool killswitch{false};
constexpr char APP_NAME[] = "iox-cpp-publisher-complexdata";

static void sigHandler(int f_sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

// push_front (list), push (stack), emplace_back (vector) return a bool - true if the insertion succeeded, false
// otherwise
// to keep the example clear, this helper function handles the return values
//! [handle return val]
void handleInsertionReturnVal(const bool success)
{
    if (!success)
    {
        std::cerr << "Failed to insert element." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
//! [handle return val]

int main()
{
    // register sigHandler
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

    // initialize runtime
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    // initialize publisher
    iox::popo::Publisher<ComplexDataType> publisher({"Group", "Instance", "ComplexDataTopic"});

    uint64_t ct = 0;
    // run until interrupted by Ctrl-C
    while (!killswitch)
    {
        ++ct;
        publisher.loan()
            .and_then([&](auto& sample) {
                //! [fill lists]
                // forward_list<string<10>, 5>
                handleInsertionReturnVal(sample->stringForwardList.push_front("world"));
                handleInsertionReturnVal(sample->stringForwardList.push_front("hello"));
                // list<uint64_t, 10>;
                handleInsertionReturnVal(sample->integerList.push_front(ct));
                handleInsertionReturnVal(sample->integerList.push_front(ct * 2));
                handleInsertionReturnVal(sample->integerList.push_front(ct + 4));
                // list<optional<int32_t>, 15>
                handleInsertionReturnVal(sample->optionalList.push_front(42));
                handleInsertionReturnVal(sample->optionalList.push_front(nullopt));
                //! [fill lists]

                // stack<float, 5>
                //! [fill stack]
                for (uint64_t i = 0U; i < sample->floatStack.capacity(); ++i)
                {
                    handleInsertionReturnVal(sample->floatStack.push(static_cast<float>(ct * i)));
                }
                //! [fill stack]

                // string<20>
                //! [assign string]
                sample->someString = "hello iceoryx";
                //! [assign string]

                // vector<double, 5>
                //! [fill vectors]
                for (uint64_t i = 0U; i < sample->doubleVector.capacity(); ++i)
                {
                    handleInsertionReturnVal(sample->doubleVector.emplace_back(static_cast<double>(ct + i)));
                }
                // vector<variant<string<10>, double>, 10>;
                handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<0>(), "seven"));
                handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<1>(), 8.0));
                handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<0>(), "nine"));
                //! [fill vectors]

                sample.publish();
            })
            .or_else([](auto& error) {
                // do something with error
                std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
            });

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return (EXIT_SUCCESS);
}

