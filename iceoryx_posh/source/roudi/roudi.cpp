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

#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/runtime/node_property.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_posh/roudi/memory/roudi_memory_manager.hpp"
#include "iceoryx_posh/runtime/port_config_info.hpp"
#include "iceoryx_utils/cxx/convert.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/posix_wrapper/thread.hpp"

namespace iox
{
namespace roudi
{
RouDi::RouDi(RouDiMemoryInterface& roudiMemoryInterface,
             PortManager& portManager,
             RoudiStartupParameters roudiStartupParameters)
    : m_killProcessesInDestructor(roudiStartupParameters.m_killProcessesInDestructor)
    , m_runMonitoringAndDiscoveryThread(true)
    , m_runHandleRuntimeMessageThread(true)
    , m_roudiMemoryInterface(&roudiMemoryInterface)
    , m_portManager(&portManager)
    , m_prcMgr(concurrent::ForwardArgsToCTor,
               *m_roudiMemoryInterface,
               portManager,
               roudiStartupParameters.m_compatibilityCheckLevel)
    , m_mempoolIntrospection(*m_roudiMemoryInterface->introspectionMemoryManager()
                                  .value(), /// @todo create a RouDiMemoryManagerData struct with all the pointer
                             *m_roudiMemoryInterface->segmentManager().value(),
                             PublisherPortUserType(m_prcMgr->addIntrospectionPublisherPort(IntrospectionMempoolService,
                                                                                           IPC_CHANNEL_ROUDI_NAME)))
    , m_monitoringMode(roudiStartupParameters.m_monitoringMode)
    , m_processKillDelay(roudiStartupParameters.m_processKillDelay)
{
    if (cxx::isCompiledOn32BitSystem())
    {
        LogWarn() << "Runnning RouDi on 32-bit architectures is not supported! Use at your own risk!";
    }
    m_processIntrospection.registerPublisherPort(PublisherPortUserType(
        m_prcMgr->addIntrospectionPublisherPort(IntrospectionProcessService, IPC_CHANNEL_ROUDI_NAME)));
    m_prcMgr->initIntrospection(&m_processIntrospection);
    m_processIntrospection.run();
    m_mempoolIntrospection.run();

    // since RouDi offers the introspection services, also add it to the list of processes
    m_processIntrospection.addProcess(getpid(), IPC_CHANNEL_ROUDI_NAME);

    // run the threads
    m_monitoringAndDiscoveryThread = std::thread(&RouDi::monitorAndDiscoveryUpdate, this);
    posix::setThreadName(m_monitoringAndDiscoveryThread.native_handle(), "Mon+Discover");

    if (roudiStartupParameters.m_runtimesMessagesThreadStart == RuntimeMessagesThreadStart::IMMEDIATE)
    {
        startProcessRuntimeMessagesThread();
    }
}

RouDi::~RouDi()
{
    shutdown();
}

void RouDi::startProcessRuntimeMessagesThread()
{
    m_handleRuntimeMessageThread = std::thread(&RouDi::processRuntimeMessages, this);
    posix::setThreadName(m_handleRuntimeMessageThread.native_handle(), "IPC-msg-process");
}

void RouDi::shutdown()
{
    m_processIntrospection.stop();
    m_portManager->stopPortIntrospection();

    // stop the process management thread in order to prevent application to register while shutting down
    m_runMonitoringAndDiscoveryThread = false;
    if (m_monitoringAndDiscoveryThread.joinable())
    {
        LogDebug() << "Joining 'Mon+Discover' thread...";
        m_monitoringAndDiscoveryThread.join();
        LogDebug() << "...'Mon+Discover' thread joined.";
    }

    if (m_killProcessesInDestructor)
    {
        cxx::DeadlineTimer finalKillTimer(m_processKillDelay);

        m_prcMgr->requestShutdownOfAllProcesses();

        using namespace units::duration_literals;
        auto remainingDurationForWarnPrint = m_processKillDelay - 2_s;
        while (m_prcMgr->isAnyRegisteredProcessStillRunning() && !finalKillTimer.hasExpired())
        {
            if (remainingDurationForWarnPrint > finalKillTimer.remainingTime())
            {
                LogWarn() << "Some applications seem to not shutdown gracefully! Time until hard shutdown: "
                          << finalKillTimer.remainingTime().toSeconds() << "s!";
                remainingDurationForWarnPrint = remainingDurationForWarnPrint - 5_s;
            }
            // give processes some time to terminate
            std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_TERMINATED_CHECK_INTERVAL.toMilliseconds()));
        }

        // Is any processes still alive?
        if (m_prcMgr->isAnyRegisteredProcessStillRunning() && finalKillTimer.hasExpired())
        {
            // Time to kill them
            m_prcMgr->killAllProcesses();
        }

        if (m_prcMgr->isAnyRegisteredProcessStillRunning())
        {
            m_prcMgr->printWarningForRegisteredProcessesAndClearProcessList();
        }
    }

    // Postpone the IpcChannelThread in order to receive TERMINATION
    m_runHandleRuntimeMessageThread = false;

    if (m_handleRuntimeMessageThread.joinable())
    {
        LogDebug() << "Joining 'IPC-msg-process' thread...";
        m_handleRuntimeMessageThread.join();
        LogDebug() << "...'IPC-msg-process' thread joined.";
    }
}

void RouDi::cyclicUpdateHook()
{
    // default implementation; do nothing
}

void RouDi::monitorAndDiscoveryUpdate()
{
    while (m_runMonitoringAndDiscoveryThread)
    {
        m_prcMgr->run();

        cyclicUpdateHook();

        std::this_thread::sleep_for(std::chrono::milliseconds(DISCOVERY_INTERVAL.toMilliseconds()));
    }
}

void RouDi::processRuntimeMessages()
{
    runtime::IpcInterfaceCreator roudiIpcInterface{IPC_CHANNEL_ROUDI_NAME};

    // the logger is intentionally not used, to ensure that this message is always printed
    std::cout << "RouDi is ready for clients" << std::endl;

    while (m_runHandleRuntimeMessageThread)
    {
        // read RouDi's IPC channel
        runtime::IpcMessage message;
        if (roudiIpcInterface.timedReceive(m_runtimeMessagesThreadTimeout, message))
        {
            auto cmd = runtime::stringToIpcMessageType(message.getElementAtIndex(0).c_str());
            std::string runtimeName = message.getElementAtIndex(1);

            processMessage(message, cmd, RuntimeName_t(cxx::TruncateToCapacity, runtimeName));
        }
    }
}

version::VersionInfo RouDi::parseRegisterMessage(const runtime::IpcMessage& message,
                                                 uint32_t& pid,
                                                 uid_t& userId,
                                                 int64_t& transmissionTimestamp)
{
    cxx::convert::fromString(message.getElementAtIndex(2).c_str(), pid);
    cxx::convert::fromString(message.getElementAtIndex(3).c_str(), userId);
    cxx::convert::fromString(message.getElementAtIndex(4).c_str(), transmissionTimestamp);
    cxx::Serialization serializationVersionInfo(message.getElementAtIndex(5));
    return serializationVersionInfo;
}

void RouDi::processMessage(const runtime::IpcMessage& message,
                           const iox::runtime::IpcMessageType& cmd,
                           const RuntimeName_t& runtimeName)
{
    switch (cmd)
    {
    case runtime::IpcMessageType::SERVICE_REGISTRY_CHANGE_COUNTER:
    {
        m_prcMgr->sendServiceRegistryChangeCounterToProcess(runtimeName);
        break;
    }
    case runtime::IpcMessageType::REG:
    {
        if (message.getNumberOfElements() != 6)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::REG\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            uint32_t pid;
            uid_t userId;
            int64_t transmissionTimestamp;
            version::VersionInfo versionInfo = parseRegisterMessage(message, pid, userId, transmissionTimestamp);

            registerProcess(
                runtimeName, pid, {userId}, transmissionTimestamp, getUniqueSessionIdForProcess(), versionInfo);
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_PUBLISHER:
    {
        if (message.getNumberOfElements() != 8)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_PUBLISHER\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            capro::ServiceDescription service(cxx::Serialization(message.getElementAtIndex(2)));
            cxx::Serialization portConfigInfoSerialization(message.getElementAtIndex(7));

            popo::PublisherOptions options;
            options.historyCapacity = std::stoull(message.getElementAtIndex(3));
            options.nodeName = NodeName_t(cxx::TruncateToCapacity, message.getElementAtIndex(4));
            options.offerOnCreate = (0U == std::stoull(message.getElementAtIndex(5))) ? false : true;
            options.subscriberTooSlowPolicy =
                static_cast<popo::SubscriberTooSlowPolicy>(std::stoul(message.getElementAtIndex(6)));

            m_prcMgr->addPublisherForProcess(
                runtimeName, service, options, iox::runtime::PortConfigInfo(portConfigInfoSerialization));
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_SUBSCRIBER:
    {
        if (message.getNumberOfElements() != 9)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_SUBSCRIBER\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            capro::ServiceDescription service(cxx::Serialization(message.getElementAtIndex(2)));
            cxx::Serialization portConfigInfoSerialization(message.getElementAtIndex(8));


            popo::SubscriberOptions options;
            options.historyRequest = std::stoull(message.getElementAtIndex(3));
            options.queueCapacity = std::stoull(message.getElementAtIndex(4));
            options.nodeName = NodeName_t(cxx::TruncateToCapacity, message.getElementAtIndex(5));
            options.subscribeOnCreate = (0U == std::stoull(message.getElementAtIndex(6))) ? false : true;
            options.queueFullPolicy = static_cast<popo::QueueFullPolicy>(std::stoul(message.getElementAtIndex(7)));

            m_prcMgr->addSubscriberForProcess(
                runtimeName, service, options, iox::runtime::PortConfigInfo(portConfigInfoSerialization));
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_CONDITION_VARIABLE:
    {
        if (message.getNumberOfElements() != 2)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_CONDITION_VARIABLE\" from \""
                       << runtimeName << "\"received!";
        }
        else
        {
            m_prcMgr->addConditionVariableForProcess(runtimeName);
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_INTERFACE:
    {
        if (message.getNumberOfElements() != 4)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_INTERFACE\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            capro::Interfaces interface =
                StringToCaProInterface(capro::IdString_t(cxx::TruncateToCapacity, message.getElementAtIndex(2)));

            m_prcMgr->addInterfaceForProcess(
                runtimeName, interface, NodeName_t(cxx::TruncateToCapacity, message.getElementAtIndex(3)));
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_APPLICATION:
    {
        if (message.getNumberOfElements() != 2)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_APPLICATION\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            m_prcMgr->addApplicationForProcess(runtimeName);
        }
        break;
    }
    case runtime::IpcMessageType::CREATE_NODE:
    {
        if (message.getNumberOfElements() != 3)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::CREATE_NODE\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            runtime::NodeProperty nodeProperty(cxx::Serialization(message.getElementAtIndex(2)));
            m_prcMgr->addNodeForProcess(runtimeName, nodeProperty.m_name);
        }
        break;
    }
    case runtime::IpcMessageType::FIND_SERVICE:
    {
        if (message.getNumberOfElements() != 3)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::FIND_SERVICE\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            capro::ServiceDescription service(cxx::Serialization(message.getElementAtIndex(2)));

            m_prcMgr->findServiceForProcess(runtimeName, service);
        }
        break;
    }
    case runtime::IpcMessageType::KEEPALIVE:
    {
        m_prcMgr->updateLivelinessOfProcess(runtimeName);
        break;
    }
    case runtime::IpcMessageType::PREPARE_APP_TERMINATION:
    {
        if (message.getNumberOfElements() != 2)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::PREPARE_APP_TERMINATION\" from \""
                       << runtimeName << "\"received!";
        }
        else
        {
            // this is used to unblock a potentially block application by blocking publisher
            m_prcMgr->handleProcessShutdownPreparationRequest(runtimeName);
        }
        break;
    }
    case runtime::IpcMessageType::TERMINATION:
    {
        if (message.getNumberOfElements() != 2)
        {
            LogError() << "Wrong number of parameters for \"IpcMessageType::TERMINATION\" from \"" << runtimeName
                       << "\"received!";
        }
        else
        {
            IOX_DISCARD_RESULT(m_prcMgr->unregisterProcess(runtimeName));
        }
        break;
    }
    default:
    {
        LogError() << "Unknown IPC message command [" << runtime::IpcMessageTypeToString(cmd) << "]";

        m_prcMgr->sendMessageNotSupportedToRuntime(runtimeName);
        break;
    }
    }
}

void RouDi::registerProcess(const RuntimeName_t& name,
                            const uint32_t pid,
                            const posix::PosixUser user,
                            const int64_t transmissionTimestamp,
                            const uint64_t sessionId,
                            const version::VersionInfo& versionInfo)
{
    bool monitorProcess = (m_monitoringMode == roudi::MonitoringMode::ON);
    IOX_DISCARD_RESULT(
        m_prcMgr->registerProcess(name, pid, user, monitorProcess, transmissionTimestamp, sessionId, versionInfo));
}

uint64_t RouDi::getUniqueSessionIdForProcess()
{
    static uint64_t sessionId = 0;
    return ++sessionId;
}

void RouDi::IpcMessageErrorHandler()
{
}

} // namespace roudi
} // namespace iox
