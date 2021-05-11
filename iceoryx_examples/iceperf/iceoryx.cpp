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

#include "iceoryx.hpp"

#include <chrono>
#include <thread>

Iceoryx::Iceoryx(const iox::capro::IdString_t& publisherName, const iox::capro::IdString_t& subscriberName) noexcept
    : m_publisher({"IcePerf", publisherName, "C++-API"}, iox::popo::PublisherOptions{1U})
    , m_subscriber({"IcePerf", subscriberName, "C++-API"}, iox::popo::SubscriberOptions{1U, 1U})
{
}

void Iceoryx::initLeader() noexcept
{
    init();
}

void Iceoryx::initFollower() noexcept
{
    init();
}

void Iceoryx::init() noexcept
{
    std::cout << "Waiting for: subscription" << std::flush;
    while (m_subscriber.getSubscriptionState() != iox::SubscribeState::SUBSCRIBED)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << ", subscriber" << std::flush;
    while (!m_publisher.hasSubscribers())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << " [ success ]" << std::endl;
}

void Iceoryx::shutdown() noexcept
{
    m_subscriber.unsubscribe();

    std::cout << "Waiting for: unsubscribe " << std::flush;
    while (!m_publisher.hasSubscribers())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // with stopOffer we disconnect all subscribers and the publisher is no more visible
    m_publisher.stopOffer();
    std::cout << " [ finished ]" << std::endl;
}

void Iceoryx::sendPerfTopic(const uint32_t payloadSizeInBytes, const RunFlag runFlag) noexcept
{
    m_publisher.loan(payloadSizeInBytes).and_then([&](auto& userPayload) {
        auto sendSample = static_cast<PerfTopic*>(userPayload);
        sendSample->payloadSize = payloadSizeInBytes;
        sendSample->runFlag = runFlag;
        sendSample->subPackets = 1;

        m_publisher.publish(userPayload);
    });
}

PerfTopic Iceoryx::receivePerfTopic() noexcept
{
    bool hasReceivedSample{false};
    PerfTopic receivedSample;

    do
    {
        m_subscriber.take().and_then([&](const void* data) {
            receivedSample = *(static_cast<const PerfTopic*>(data));
            hasReceivedSample = true;
            m_subscriber.release(data);
        });
    } while (!hasReceivedSample);

    return receivedSample;
}
