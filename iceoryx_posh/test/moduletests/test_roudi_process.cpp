// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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
#include "iceoryx_posh/internal/roudi/process.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/roudi/memory/roudi_memory_interface.hpp"
#include "iceoryx_posh/version/compatibility_check_level.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/platform/types.hpp"
#include "test.hpp"

using namespace ::testing;
using namespace iox::roudi;
using namespace iox::popo;
using namespace iox::runtime;
using namespace iox::posix;
using ::testing::Return;

class IpcInterfaceUser_Mock : public iox::roudi::Process
{
  public:
    IpcInterfaceUser_Mock()
        : iox::roudi::Process("TestProcess", 200, PosixUser("foo"), true, 255)
    {
    }
    MOCK_METHOD1(sendViaIpcChannel, void(IpcMessage));
    iox::mepoo::MemoryManager m_payloadDataSegmentMemoryManager;
};

class Process_test : public Test
{
  public:
    const iox::RuntimeName_t processname = {"TestProcess"};
    pid_t pid{200U};
    PosixUser user{"foo"};
    bool isMonitored = true;
    const uint64_t dataSegmentId{0x654321U};
    const uint64_t sessionId{255U};
    IpcInterfaceUser_Mock ipcInterfaceUserMock;
};

TEST_F(Process_test, getPid)
{
    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    EXPECT_THAT(roudiproc.getPid(), Eq(pid));
}

TEST_F(Process_test, getName)
{
    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    EXPECT_THAT(roudiproc.getName(), Eq(std::string(processname)));
}

TEST_F(Process_test, isMonitored)
{
    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    EXPECT_THAT(roudiproc.isMonitored(), Eq(isMonitored));
}

TEST_F(Process_test, getSessionId)
{
    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    EXPECT_THAT(roudiproc.getSessionId(), Eq(sessionId));
}

TEST_F(Process_test, sendViaIpcChannelPass)
{
    iox::runtime::IpcMessage data{"MESSAGE_NOT_SUPPORTED"};
    EXPECT_CALL(ipcInterfaceUserMock, sendViaIpcChannel(_)).Times(1);
    ipcInterfaceUserMock.sendViaIpcChannel(data);
}
TEST_F(Process_test, sendViaIpcChannelFail)
{
    iox::runtime::IpcMessage data{""};
    iox::cxx::optional<iox::Error> sendViaIpcChannelStatusFail;

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&sendViaIpcChannelStatusFail](
            const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            sendViaIpcChannelStatusFail.emplace(error);
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::MODERATE));
        });

    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    roudiproc.sendViaIpcChannel(data);

    ASSERT_THAT(sendViaIpcChannelStatusFail.has_value(), Eq(true));
    EXPECT_THAT(sendViaIpcChannelStatusFail.value(), Eq(iox::Error::kPOSH__ROUDI_PROCESS_SEND_VIA_IPC_CHANNEL_FAILED));
}

TEST_F(Process_test, TimeStamp)
{
    auto timestmp = iox::mepoo::BaseClock_t::now();
    Process roudiproc(processname, pid, user, isMonitored, sessionId);
    roudiproc.setTimestamp(timestmp);
    EXPECT_THAT(roudiproc.getTimestamp(), Eq(timestmp));
}
