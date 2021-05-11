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
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

constexpr char APP_NAME[] = "iox-cpp-cheeky";

int main()
{
    // initialize runtime
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    // When starting this app with the user 'notallowed'

    // 1) Subscribers can be created without any readable shared memory segment
    /// @todo currently segfaults, in this case no data should ever arrive
    iox::popo::Subscriber<RadarObject> subscriber({"Radar", "FrontLeft", "Object"});

    // 2) The publisher object can't be initalised correctly because the user 'notallowed' isn't in any group which has
    // write access to any shared memory segment.
    // The error POSH__RUNTIME_NO_WRITABLE_SHM_SEGMENT will be reported and programm execution will end
    iox::popo::Publisher<RadarObject> publisher({"Radar", "FrontLeft", "Object"});

    return EXIT_FAILURE;
}
