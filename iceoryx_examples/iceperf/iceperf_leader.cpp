// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceperf_leader.hpp"
#include "iceoryx.hpp"
#include "iceoryx_c.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/convert.hpp"
#include "mq.hpp"
#include "topic_data.hpp"
#include "uds.hpp"

#include <iomanip>
#include <iostream>

//! [use constants instead of magic values]
constexpr const char APP_NAME[]{"iceperf-bench-leader"};
constexpr const char PUBLISHER[]{"Leader"};
constexpr const char SUBSCRIBER[]{"Follower"};
//! [use constants instead of magic values]

IcePerfLeader::IcePerfLeader(const PerfSettings settings) noexcept
    : m_settings(settings)
{
    //! [cleanup outdated resources]
#ifndef __APPLE__
    MQ::cleanupOutdatedResources(PUBLISHER, SUBSCRIBER);
#endif
    UDS::cleanupOutdatedResources(PUBLISHER, SUBSCRIBER);
    //! [cleanup outdated resources]
}

//! [do the measurement for a single technology]
void IcePerfLeader::doMeasurement(IcePerfBase& ipcTechnology) noexcept
{
    ipcTechnology.initLeader();

    std::vector<std::tuple<uint32_t, iox::units::Duration>> latencyMeasurements;
    const std::vector<uint32_t> payloadSizesInKB{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    std::cout << "Measurement for:";
    const char* separator = " ";
    for (const auto payloadSizeInKB : payloadSizesInKB)
    {
        std::cout << separator << payloadSizeInKB << " kB" << std::flush;
        separator = ", ";
        auto payloadSizeInBytes = payloadSizeInKB * IcePerfBase::ONE_KILOBYTE;

        ipcTechnology.preLatencyPerfTestLeader(payloadSizeInBytes);

        auto latency = ipcTechnology.latencyPerfTestLeader(m_settings.numberOfSamples);

        latencyMeasurements.push_back(std::make_tuple(payloadSizeInKB, latency));

        ipcTechnology.postLatencyPerfTestLeader();
    }
    std::cout << std::endl;

    ipcTechnology.releaseFollower();

    ipcTechnology.shutdown();

    std::cout << std::endl;
    std::cout << "#### Measurement Result ####" << std::endl;
    std::cout << m_settings.numberOfSamples << " round trips for each payload." << std::endl;
    std::cout << std::endl;
    std::cout << "| Payload Size [kB] | Average Latency [µs] |" << std::endl;
    std::cout << "|------------------:|---------------------:|" << std::endl;
    for (const auto& latencyMeasuement : latencyMeasurements)
    {
        auto payloadSizeInKB = std::get<0>(latencyMeasuement);
        auto latencyInMicroseconds = static_cast<double>(std::get<1>(latencyMeasuement).toNanoseconds()) / 1000.0;
        std::cout << "| " << std::setw(17) << payloadSizeInKB << " | " << std::setw(20) << std::setprecision(2)
                  << latencyInMicroseconds << " |" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Finished!" << std::endl;
}
//! [do the measurement for a single technology]

//! [run all technologies]
int IcePerfLeader::run() noexcept
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    //! [send setting to follower application]
    iox::capro::ServiceDescription serviceDescription{"IcePerf", "Settings", "Generic"};
    iox::popo::PublisherOptions options;
    options.historyCapacity = 1U;
    iox::popo::Publisher<PerfSettings> settingsPublisher{serviceDescription, options};
    if (!settingsPublisher.publishCopyOf(m_settings))
    {
        std::cerr << "Could not send settings to follower!" << std::endl;
        return EXIT_FAILURE;
    }
    //! [send setting to follower application]

    //! [create an run technologies]
    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::POSIX_MESSAGE_QUEUE)
    {
#ifndef __APPLE__
        std::cout << std::endl << "******   MESSAGE QUEUE    ********" << std::endl;
        MQ mq(PUBLISHER, SUBSCRIBER);
        doMeasurement(mq);
#else
        if (m_settings.technology == Technology::POSIX_MESSAGE_QUEUE)
        {
            std::cout << "The message queue is not supported on macOS and will be skipped!" << std::endl;
        }
#endif
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::UNIX_DOMAIN_SOCKET)
    {
        std::cout << std::endl << "****** UNIX DOMAIN SOCKET ********" << std::endl;
        UDS uds(PUBLISHER, SUBSCRIBER);
        doMeasurement(uds);
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::ICEORYX_CPP_API)
    {
        std::cout << std::endl << "******      ICEORYX       ********" << std::endl;
        Iceoryx iceoryx(PUBLISHER, SUBSCRIBER);
        doMeasurement(iceoryx);
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::ICEORYX_C_API)
    {
        std::cout << std::endl << "******   ICEORYX C API    ********" << std::endl;
        IceoryxC iceoryxc(PUBLISHER, SUBSCRIBER);
        doMeasurement(iceoryxc);
    }
    //! [create an run technologies]

    return EXIT_SUCCESS;
}
//! [run all technologies]
