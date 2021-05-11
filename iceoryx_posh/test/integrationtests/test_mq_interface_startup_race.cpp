// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "test.hpp"

#include "iceoryx_posh/internal/runtime/ipc_message.hpp"
#include "iceoryx_posh/internal/runtime/ipc_runtime_interface.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/internal/posix_wrapper/message_queue.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"


#include <chrono>
#include <mutex>
#include <thread>

using namespace ::testing;
using namespace iox;
using namespace iox::units;
using namespace iox::posix;

using iox::runtime::IpcInterfaceBase;
using iox::runtime::IpcMessage;
using iox::runtime::IpcMessageType;
using iox::runtime::IpcRuntimeInterface;


#if !defined(__APPLE__)
constexpr char DeleteRouDiMessageQueue[] = "rm /dev/mqueue/roudi";
#endif

constexpr char MqAppName[] = "racer";

class StringToMessage : public IpcInterfaceBase
{
  public:
    using IpcInterfaceBase::setMessageFromString;
};

class CMqInterfaceStartupRace_test : public Test
{
  public:
    CMqInterfaceStartupRace_test()
        : m_appQueue{IpcChannelType::create()}
    {
    }

    virtual void SetUp()
    {
        ASSERT_THAT(m_roudiQueue.has_error(), false);
    }
    virtual void TearDown()
    {
    }

    IpcMessage getIpcMessage(const std::string& request) const
    {
        IpcMessage msg;
        StringToMessage::setMessageFromString(request.c_str(), msg);
        return msg;
    }

    void checkRegRequest(const IpcMessage& msg) const
    {
        ASSERT_THAT(msg.getNumberOfElements(), Eq(6u));

        std::string cmd = msg.getElementAtIndex(0);
        ASSERT_THAT(cmd.c_str(), StrEq(IpcMessageTypeToString(IpcMessageType::REG)));

        std::string name = msg.getElementAtIndex(1);
        ASSERT_THAT(name.c_str(), StrEq(MqAppName));
    }

    void sendRegAck(const IpcMessage& oldMsg)
    {
        std::lock_guard<std::mutex> lock(m_appQueueMutex);
        IpcMessage regAck;
        constexpr uint32_t DUMMY_SHM_SIZE{37};
        constexpr uint32_t DUMMY_SHM_OFFSET{73};
        constexpr uint32_t DUMMY_SEGMENT_ID{13};
        constexpr uint32_t INDEX_OF_TIMESTAMP{4};
        regAck << IpcMessageTypeToString(IpcMessageType::REG_ACK) << DUMMY_SHM_SIZE << DUMMY_SHM_OFFSET
               << oldMsg.getElementAtIndex(INDEX_OF_TIMESTAMP) << DUMMY_SEGMENT_ID;

        if (m_appQueue.has_error())
        {
            m_appQueue = IpcChannelType::create(MqAppName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT);
        }
        ASSERT_THAT(m_appQueue.has_error(), false);

        ASSERT_FALSE(m_appQueue->send(regAck.getMessage()).has_error());
    }

    /// @note smart_lock in combination with optional is currently not really usable
    std::mutex m_roudiQueueMutex;
    IpcChannelType::result_t m_roudiQueue{
        IpcChannelType::create(roudi::IPC_CHANNEL_ROUDI_NAME, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER)};
    std::mutex m_appQueueMutex;
    IpcChannelType::result_t m_appQueue;
};

#if !defined(__APPLE__)
TEST_F(CMqInterfaceStartupRace_test, DISABLED_ObsoleteRouDiMq)
{
    /// @note this test checks if the application handles the situation when the roudi mqueue was not properly cleaned
    /// up and tries to use the obsolet mqueue while RouDi gets restarted and cleans its resources up and creates a new
    /// mqueue

    std::atomic<bool> shutdown;
    shutdown = false;
    auto roudi = std::thread([&] {
        std::lock_guard<std::mutex> lock(m_roudiQueueMutex);
        // ensure that the application already opened the roudi mqueue by waiting until a REG request is sent to the
        // roudi mqueue
        auto request = m_roudiQueue->timedReceive(15_s);
        ASSERT_FALSE(request.has_error());
        auto msg = getIpcMessage(request.value());
        checkRegRequest(msg);

        // simulate the restart of RouDi with the mqueue cleanup
        auto sysC = iox::cxx::makeSmartC(
            system, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, DeleteRouDiMessageQueue);

        if (sysC.hasErrors())
        {
            std::cerr << "system call failed with error: " << sysC.getErrorString();
            exit(EXIT_FAILURE);
        }

        auto m_roudiQueue2 =
            IpcChannelType::create(roudi::IPC_CHANNEL_ROUDI_NAME, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);

        // check if the app retries to register at RouDi
        request = m_roudiQueue2->timedReceive(15_s);
        ASSERT_FALSE(request.has_error());
        msg = getIpcMessage(request.value());
        checkRegRequest(msg);

        sendRegAck(msg);

        while (!shutdown)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    IpcRuntimeInterface dut(roudi::IPC_CHANNEL_ROUDI_NAME, MqAppName, 35_s);

    shutdown = true;
    roudi.join();
}

TEST_F(CMqInterfaceStartupRace_test, DISABLED_ObsoleteRouDiMqWithFullMq)
{
    /// @note this test checks if the application handles the situation when the roudi mqueue was not properly cleaned
    /// up and tries to use the obsolet mqueue while RouDi gets restarted and cleans its resources up and creates a new
    /// mqueue, the obsolete mqueue was filled up to the max message size, e.g. by the KEEP_ALIVE messages

    std::atomic<bool> shutdown;
    shutdown = false;
    auto roudi = std::thread([&] {
        // fill the roudi mqueue
        std::lock_guard<std::mutex> lock(m_roudiQueueMutex);
        while (!m_roudiQueue->timedSend("dummy", 1_s).has_error())
        {
        }

        // wait some time for Runtime::GetInstance to send a REQ request with the full mqueue
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // simulate the restart of RouDi with the mqueue cleanup
        auto sysC = iox::cxx::makeSmartC(
            system, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, DeleteRouDiMessageQueue);

        if (sysC.hasErrors())
        {
            std::cerr << "system call failed with error: " << sysC.getErrorString();
            exit(EXIT_FAILURE);
        }
        auto newRoudi =
            IpcChannelType::create(roudi::IPC_CHANNEL_ROUDI_NAME, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);

        // check if the app retries to register at RouDi
        auto request = newRoudi->timedReceive(15_s);
        if (request.has_error())
        {
            // clear the old mqueue to prevent a deadlock in mq_send to the old roudi mqueue in the app
            while (!m_roudiQueue->timedReceive(1_s).has_error())
            {
            }
        }
        ASSERT_FALSE(request.has_error());
        auto msg = getIpcMessage(request.value());
        checkRegRequest(msg);

        sendRegAck(msg);

        while (!shutdown)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    IpcRuntimeInterface dut(roudi::IPC_CHANNEL_ROUDI_NAME, MqAppName, 35_s);

    shutdown = true;
    roudi.join();
}
#endif

TEST_F(CMqInterfaceStartupRace_test, ObsoleteRegAck)
{
    /// @note this test checks if the application handles the situation when it sends an REG request to RouDi,
    /// terminates, gets restarted and sends a new REG request while RouDi has not yet processed the first REG request;
    /// this results in a message in the application mqueue which will be read with the next command and results in a
    /// wrong response

    std::atomic<bool> shutdown;
    shutdown = false;
    auto roudi = std::thread([&] {
        std::lock_guard<std::mutex> lock(m_roudiQueueMutex);
        // wait for the REG request
        auto request = m_roudiQueue->timedReceive(5_s);
        ASSERT_FALSE(request.has_error());
        auto msg = getIpcMessage(request.value());
        checkRegRequest(msg);

        IpcMessage obsoleteMsg;
        for (uint32_t i = 0; i < 4; ++i)
        {
            obsoleteMsg << msg.getElementAtIndex(i);
        }
        // set an invalid timestamp
        obsoleteMsg << 0;
        sendRegAck(obsoleteMsg);
        sendRegAck(msg);

        while (!shutdown)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    IpcRuntimeInterface dut(roudi::IPC_CHANNEL_ROUDI_NAME, MqAppName, 35_s);

    shutdown = true;
    roudi.join();

    std::lock_guard<std::mutex> lock(m_appQueueMutex);
    // the app IPC channel should be empty after registration
    auto response = m_appQueue->timedReceive(10_ms);
    EXPECT_THAT(response.has_error(), Eq(true));
}
