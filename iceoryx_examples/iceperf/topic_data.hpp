// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_EXAMPLES_ICEPERF_TOPIC_DATA_HPP
#define IOX_EXAMPLES_ICEPERF_TOPIC_DATA_HPP

#include "example_common.hpp"

#include <cstdint>

//! [topic data definitions]
struct PerfSettings
{
    Benchmark benchmark{Benchmark::ALL};
    Technology technology{Technology::ALL};
    uint64_t numberOfSamples{10000U};
};

struct PerfTopic
{
    uint32_t payloadSize{0};
    uint32_t subPackets{0};
    RunFlag runFlag{RunFlag::RUN};
};
//! [topic data definitions]

#endif // IOX_EXAMPLES_ICEPERF_TOPIC_DATA_HPP
