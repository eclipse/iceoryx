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
#include "iceoryx_posh/internal/roudi/process_manager.hpp"
#include "iceoryx_posh/internal/runtime/ipc_interface_creator.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/roudi/memory/iceoryx_roudi_memory_manager.hpp"
#include "iceoryx_posh/roudi/memory/roudi_memory_interface.hpp"
#include "iceoryx_posh/version/compatibility_check_level.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/platform/types.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"
#include "test.hpp"

using namespace ::testing;
using namespace iox::roudi;
using namespace iox::popo;
using namespace iox::runtime;
using namespace iox::posix;
using namespace iox::version;
using ::testing::Return;

class ProcessManager_test : public Test
{
  public:
    void SetUp() override
    {
        auto config = iox::RouDiConfig_t().setDefaults();
        m_roudiMemoryManager = std::make_unique<IceOryxRouDiMemoryManager>(config);
        EXPECT_FALSE(m_roudiMemoryManager->createAndAnnounceMemory().has_error());
        m_portManager = std::make_unique<PortManager>(m_roudiMemoryManager.get());
        CompatibilityCheckLevel m_compLevel{CompatibilityCheckLevel::OFF};
        m_sut = std::make_unique<ProcessManager>(*m_roudiMemoryManager, *m_portManager, m_compLevel);
        m_sut->initIntrospection(&m_processIntrospection);
    }

    void TearDown() override
    {
    }

    const iox::RuntimeName_t m_processname{"TestProcess"};
    const pid_t m_pid{42U};
    PosixUser m_user{iox::posix::PosixUser::getUserOfCurrentProcess().getName()};
    const bool m_isMonitored{true};
    VersionInfo m_versionInfo{42U, 42U, 42U, 42U, "Foo", "Bar"};

    IpcInterfaceCreator m_processIpcInterface{m_processname};
    ProcessIntrospectionType m_processIntrospection;

    std::unique_ptr<IceOryxRouDiMemoryManager> m_roudiMemoryManager{nullptr};
    std::unique_ptr<PortManager> m_portManager{nullptr};
    std::unique_ptr<ProcessManager> m_sut{nullptr};
};


TEST_F(ProcessManager_test, RegisterProcessWithMonitorningWorks)
{
    auto result = m_sut->registerProcess(m_processname, m_pid, m_user, m_isMonitored, 1U, 1U, m_versionInfo);

    EXPECT_TRUE(result);
}

TEST_F(ProcessManager_test, RegisterProcessWithoutMonitoringWorks)
{
    constexpr bool isNotMonitored{false};
    auto result = m_sut->registerProcess(m_processname, m_pid, m_user, isNotMonitored, 1U, 1U, m_versionInfo);

    EXPECT_TRUE(result);
}

TEST_F(ProcessManager_test, RegisterSameProcessTwiceWithMonitoringWorks)
{
    auto result1 = m_sut->registerProcess(m_processname, m_pid, m_user, m_isMonitored, 1U, 1U, m_versionInfo);
    auto result2 = m_sut->registerProcess(m_processname, m_pid, m_user, m_isMonitored, 1U, 1U, m_versionInfo);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
}

TEST_F(ProcessManager_test, RegisterSameProcessTwiceWithoutMonitoringWorks)
{
    constexpr bool isNotMonitored{false};
    auto result1 = m_sut->registerProcess(m_processname, m_pid, m_user, isNotMonitored, 1U, 1U, m_versionInfo);
    auto result2 = m_sut->registerProcess(m_processname, m_pid, m_user, isNotMonitored, 1U, 1U, m_versionInfo);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
}

TEST_F(ProcessManager_test, UnregisterNonExistentProcessLeadsToError)
{
    auto unregisterResult = m_sut->unregisterProcess(m_processname);

    EXPECT_FALSE(unregisterResult);
}

TEST_F(ProcessManager_test, RegisterAndUnregisterWorks)
{
    m_sut->registerProcess(m_processname, m_pid, m_user, m_isMonitored, 1U, 1U, m_versionInfo);
    auto unregisterResult = m_sut->unregisterProcess(m_processname);

    EXPECT_TRUE(unregisterResult);
}

TEST_F(ProcessManager_test, HandleProcessShutdownPreparationRequestWorks)
{
    m_sut->registerProcess(m_processname, m_pid, m_user, m_isMonitored, 1U, 1U, m_versionInfo);

    auto user = iox::posix::PosixUser::getUserOfCurrentProcess().getName();
    auto payloadDataSegmentMemoryManager = m_roudiMemoryManager->segmentManager()
                                               .value()
                                               ->getSegmentInformationWithWriteAccessForUser(user)
                                               .m_memoryManager;

    ASSERT_TRUE(payloadDataSegmentMemoryManager.has_value());

    // get publisher and subscriber
    PublisherOptions publisherOptions{
        0U, iox::NodeName_t("node"), true, iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER};
    PublisherPortUser publisher(m_portManager
                                    ->acquirePublisherPortData({1U, 1U, 1U},
                                                               publisherOptions,
                                                               m_processname,
                                                               &payloadDataSegmentMemoryManager.value().get(),
                                                               PortConfigInfo())
                                    .value());

    ASSERT_TRUE(publisher.isOffered());

    m_sut->handleProcessShutdownPreparationRequest(m_processname);

    // we just check if handleProcessShutdownPreparationRequest calls PortManager::unblockProcessShutdown
    // ideally this should be checked by a mock, but since there isn't on for PortManager we just check the side effect
    ASSERT_FALSE(publisher.isOffered());
}
