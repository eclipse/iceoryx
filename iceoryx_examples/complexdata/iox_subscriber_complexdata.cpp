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

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

std::atomic_bool killswitch{false};
constexpr char APP_NAME[] = "iox-cpp-subscriber-complexdata";

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

    // initialize subscriber
    iox::popo::Subscriber<ComplexDataType> subscriber({"Group", "Instance", "ComplexDataTopic"});

    // run until interrupted by Ctrl-C
    while (!killswitch)
    {
        subscriber.take()
            .and_then([](auto& sample) {
                std::stringstream s;
                s << APP_NAME << " got values:";
                const char* separator = " ";

                s << std::endl << "stringForwardList:";
                for (const auto& entry : sample->stringForwardList)
                {
                    s << separator << entry;
                    separator = ", ";
                }

                s << std::endl << "integerList:";
                separator = " ";
                for (const auto& entry : sample->integerList)
                {
                    s << separator << entry;
                    separator = ", ";
                }

                s << std::endl << "optionalList:";
                separator = " ";
                //! [read optional list]
                for (const auto& entry : sample->optionalList)
                {
                    (entry.has_value()) ? s << separator << entry.value() : s << separator << "optional is empty";
                    separator = ", ";
                }
                //! [read optional list]

                s << std::endl << "floatStack:";
                separator = " ";
                //! [read stack]
                auto stackCopy = sample->floatStack;
                while (stackCopy.size() > 0U)
                {
                    auto result = stackCopy.pop();
                    s << separator << result.value();
                    separator = ", ";
                }
                //! [read stack]

                s << std::endl << "someString: " << sample->someString;

                s << std::endl << "doubleVector:";
                separator = " ";
                for (const auto& entry : sample->doubleVector)
                {
                    s << separator << entry;
                    separator = ", ";
                }

                s << std::endl << "variantVector:";
                separator = " ";
                //! [read variant vector]
                for (const auto& i : sample->variantVector)
                {
                    switch (i.index())
                    {
                    case 0:
                        s << separator << *i.template get_at_index<0>();
                        break;
                    case 1:
                        s << separator << *i.template get_at_index<1>();
                        break;
                    case INVALID_VARIANT_INDEX:
                        s << separator << "variant does not contain a type";
                        break;
                    default:
                        s << separator << "this is a new type";
                    }
                    separator = ", ";
                }
                //! [read variant vector]

                std::cout << s.str() << std::endl << std::endl;
            })
            .or_else([](auto& result) {
                // only has to be called if the alternative is of interest,
                // i.e. if nothing has to happen when no data is received and
                // a possible error alternative is not checked or_else is not needed
                if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                {
                    std::cout << "Error receiving chunk." << std::endl;
                }
            });

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return (EXIT_SUCCESS);
}

