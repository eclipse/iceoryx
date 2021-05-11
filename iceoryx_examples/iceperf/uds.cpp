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

#include "uds.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"

#include <chrono>
#include <thread>

UDS::UDS(const std::string& publisherName, const std::string& subscriberName) noexcept
    : m_publisherSocketName(PREFIX + publisherName)
    , m_subscriberSocketName(PREFIX + subscriberName)
{
    initSocketAddress(m_sockAddrPublisher, m_publisherSocketName);
    initSocketAddress(m_sockAddrSubscriber, m_subscriberSocketName);
}

void UDS::initSocketAddress(sockaddr_un& socketAddr, const std::string& socketName)
{
    memset(&socketAddr, 0, sizeof(sockaddr_un));
    socketAddr.sun_family = AF_LOCAL;
    const uint64_t maxDestinationLength = iox::cxx::strlen2(socketAddr.sun_path);
    iox::cxx::Ensures(maxDestinationLength >= socketName.size() && "Socketname too large!");
    strncpy(socketAddr.sun_path, socketName.c_str(), socketName.size());
}

void UDS::cleanupOutdatedResources(const std::string& publisherName, const std::string& subscriberName) noexcept
{
    auto publisherSocketName = PREFIX + publisherName;
    sockaddr_un sockAddrPublisher;
    initSocketAddress(sockAddrPublisher, publisherSocketName);
    auto unlinkCallPublisher = iox::cxx::makeSmartC(
        unlink, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {ENOENT}, sockAddrPublisher.sun_path);

    if (unlinkCallPublisher.hasErrors())
    {
        std::cout << "unlink error" << std::endl;
        exit(1);
    }

    auto subscriberSocketName = PREFIX + subscriberName;
    sockaddr_un sockAddrSubscriber;
    initSocketAddress(sockAddrSubscriber, subscriberSocketName);
    auto unlinkCallSubscriber = iox::cxx::makeSmartC(
        unlink, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {ENOENT}, sockAddrSubscriber.sun_path);

    if (unlinkCallSubscriber.hasErrors())
    {
        std::cout << "unlink error" << std::endl;
        exit(1);
    }
}

void UDS::initLeader() noexcept
{
    init();

    std::cout << "waiting for follower" << std::endl;
    waitForFollower();

    receivePerfTopic();
}

void UDS::initFollower() noexcept
{
    init();

    std::cout << "registering with the leader" << std::endl;
    waitForLeader();

    sendPerfTopic(sizeof(PerfTopic), RunFlag::RUN);
}

void UDS::init() noexcept
{
    // init subscriber
    auto socketCallSubscriber = iox::cxx::makeSmartC(
        socket, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, AF_LOCAL, SOCK_DGRAM, 0);

    if (socketCallSubscriber.hasErrors())
    {
        std::cout << "socket error" << std::endl;
        exit(1);
    }

    m_sockfdSubscriber = socketCallSubscriber.getReturnValue();

    auto bindCall = iox::cxx::makeSmartC(bind,
                                         iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                         {ERROR_CODE},
                                         {},
                                         m_sockfdSubscriber,
                                         reinterpret_cast<struct sockaddr*>(&m_sockAddrSubscriber),
                                         static_cast<socklen_t>(sizeof(m_sockAddrSubscriber)));

    if (bindCall.hasErrors())
    {
        std::cout << "bind error" << std::endl;
        exit(1);
    }

    // init publisher
    auto socketCallPublisher = iox::cxx::makeSmartC(
        socket, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, AF_LOCAL, SOCK_DGRAM, 0);

    if (socketCallPublisher.hasErrors())
    {
        std::cout << "socket error" << std::endl;
        exit(1);
    }

    m_sockfdPublisher = socketCallPublisher.getReturnValue();
}
void UDS::waitForLeader() noexcept
{
    // try to send an empty message
    constexpr bool TRY_TO_SEND{true};
    while (TRY_TO_SEND)
    {
        auto sendCall = iox::cxx::makeSmartC(sendto,
                                             iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                             {static_cast<long int>(ERROR_CODE)},
                                             {ENOENT},
                                             m_sockfdPublisher,
                                             nullptr,
                                             static_cast<size_t>(0),
                                             static_cast<int>(0),
                                             reinterpret_cast<struct sockaddr*>(&m_sockAddrPublisher),
                                             static_cast<socklen_t>(sizeof(m_sockAddrPublisher)));

        if (sendCall.hasErrors())
        {
            std::cout << "send error" << std::endl;
            exit(1);
        }
        else if (sendCall.getErrNum() == ENOENT)
        {
            constexpr std::chrono::milliseconds RETRY_INTERVAL{10};
            std::this_thread::sleep_for(RETRY_INTERVAL);
            continue;
        }

        break;
    }
}

void UDS::waitForFollower() noexcept
{
    // try to receive the empty message
    receive(m_message);
}

void UDS::shutdown() noexcept
{
    if (m_sockfdPublisher != INVALID_FD)
    {
        auto closeCall = iox::cxx::makeSmartC(
            iox_close, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, m_sockfdPublisher);

        if (closeCall.hasErrors())
        {
            std::cout << "close error" << std::endl;
            exit(1);
        }
    }

    if (m_sockfdSubscriber != INVALID_FD)
    {
        auto closeCall = iox::cxx::makeSmartC(
            iox_close, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, m_sockfdSubscriber);

        if (closeCall.hasErrors())
        {
            std::cout << "close error" << std::endl;
            exit(1);
        }

        auto unlinkCall = iox::cxx::makeSmartC(
            unlink, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {ERROR_CODE}, {}, m_sockAddrSubscriber.sun_path);

        if (unlinkCall.hasErrors())
        {
            std::cout << "unlink error" << std::endl;
            exit(1);
        }
    }
}

void UDS::sendPerfTopic(const uint32_t payloadSizeInBytes, const RunFlag runFlag) noexcept
{
    char* buffer = new char[payloadSizeInBytes];
    auto sample = reinterpret_cast<PerfTopic*>(&buffer[0]);

    // Specify the payload size for the measurement
    sample->payloadSize = payloadSizeInBytes;
    sample->runFlag = runFlag;
    if (payloadSizeInBytes <= MAX_MESSAGE_SIZE)
    {
        sample->subPackets = 1;
        send(&buffer[0], payloadSizeInBytes);
    }
    else
    {
        sample->subPackets = payloadSizeInBytes / MAX_MESSAGE_SIZE;
        for (uint32_t i = 0U; i < sample->subPackets; ++i)
        {
            send(&buffer[0], MAX_MESSAGE_SIZE);
        }
    }
    delete[] buffer;
}

PerfTopic UDS::receivePerfTopic() noexcept
{
    receive(&m_message[0]);

    auto receivedSample = reinterpret_cast<const PerfTopic*>(&m_message[0]);

    if (receivedSample->subPackets > 1)
    {
        for (uint32_t i = 0U; i < receivedSample->subPackets - 1; ++i)
        {
            receive(&m_message[0]);
        }
    }

    return *receivedSample;
}

void UDS::send(const char* buffer, uint32_t length) noexcept
{
    while (true)
    {
        auto sendCall = iox::cxx::makeSmartC(sendto,
                                             iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                             {static_cast<long int>(ERROR_CODE)},
                                             {ENOBUFS},
                                             m_sockfdPublisher,
                                             buffer,
                                             static_cast<size_t>(length),
                                             static_cast<int>(0),
                                             reinterpret_cast<struct sockaddr*>(&m_sockAddrPublisher),
                                             static_cast<socklen_t>(sizeof(m_sockAddrPublisher)));

        if (sendCall.hasErrors() && sendCall.getErrNum() != ENOBUFS)
        {
            std::cout << std::endl << "send error" << std::endl;
            exit(1);
        }
        // only return from this loop when the message could be send successfully
        // if the OS socket message buffer if full, retry until it is free'd by
        // the OS and the message could be send
        else if (!sendCall.hasErrors() && sendCall.getErrNum() != ENOBUFS)
        {
            break;
        }
    }
}

void UDS::receive(char* buffer) noexcept
{
    auto recvCall = iox::cxx::makeSmartC(recvfrom,
                                         iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                         {static_cast<long int>(ERROR_CODE)},
                                         {},
                                         m_sockfdSubscriber,
                                         buffer,
                                         MAX_MESSAGE_SIZE,
                                         0,
                                         nullptr,
                                         nullptr);

    if (recvCall.hasErrors())
    {
        std::cout << "receive error" << std::endl;
        exit(1);
    }
}
