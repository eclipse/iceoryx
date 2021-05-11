// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#include "base.hpp"


void IcePerfBase::preLatencyPerfTestLeader(const uint32_t payloadSizeInBytes) noexcept
{
    sendPerfTopic(payloadSizeInBytes, RunFlag::RUN);
}

void IcePerfBase::postLatencyPerfTestLeader() noexcept
{
    // Wait for the last response
    receivePerfTopic();
}

void IcePerfBase::releaseFollower() noexcept
{
    sendPerfTopic(sizeof(PerfTopic), RunFlag::STOP);
}

iox::units::Duration IcePerfBase::latencyPerfTestLeader(const uint64_t numRoundTrips) noexcept
{
    auto start = std::chrono::high_resolution_clock::now();

    // run the performance test
    for (auto i = 0U; i < numRoundTrips; ++i)
    {
        auto perfTopic = receivePerfTopic();
        sendPerfTopic(perfTopic.payloadSize, RunFlag::RUN);
    }

    auto finish = std::chrono::high_resolution_clock::now();

    constexpr uint64_t TRANSMISSIONS_PER_ROUNDTRIP{2U};
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    auto latencyInNanoSeconds =
        (static_cast<uint64_t>(duration.count()) / (numRoundTrips * TRANSMISSIONS_PER_ROUNDTRIP));
    return iox::units::Duration::fromNanoseconds(latencyInNanoSeconds);
}

void IcePerfBase::latencyPerfTestFollower() noexcept
{
    while (true)
    {
        auto perfTopic = receivePerfTopic();

        // stop replying when no more run
        if (perfTopic.runFlag == RunFlag::STOP)
        {
            break;
        }

        sendPerfTopic(perfTopic.payloadSize, RunFlag::RUN);
    }
}
