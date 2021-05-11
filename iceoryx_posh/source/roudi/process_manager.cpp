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

#include "iceoryx_posh/internal/roudi/process_manager.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/deadline_timer.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"
#include "iceoryx_utils/platform/signal.hpp"
#include "iceoryx_utils/platform/types.hpp"
#include "iceoryx_utils/platform/wait.hpp"

#include <chrono>
#include <csignal>
#include <thread>

using namespace iox::units::duration_literals;

namespace iox
{
namespace roudi
{
ProcessManager::ProcessManager(RouDiMemoryInterface& roudiMemoryInterface,
                               PortManager& portManager,
                               const version::CompatibilityCheckLevel compatibilityCheckLevel) noexcept
    : m_roudiMemoryInterface(roudiMemoryInterface)
    , m_portManager(portManager)
    , m_compatibilityCheckLevel(compatibilityCheckLevel)
{
    bool fatalError{false};

    auto maybeSegmentManager = m_roudiMemoryInterface.segmentManager();
    if (!maybeSegmentManager.has_value())
    {
        LogFatal() << "Invalid state! Could not obtain SegmentManager!";
        fatalError = true;
    }
    m_segmentManager = maybeSegmentManager.value();

    auto maybeIntrospectionMemoryManager = m_roudiMemoryInterface.introspectionMemoryManager();
    if (!maybeIntrospectionMemoryManager.has_value())
    {
        LogFatal() << "Invalid state! Could not obtain MemoryManager for instrospection!";
        fatalError = true;
    }
    m_introspectionMemoryManager = maybeIntrospectionMemoryManager.value();

    auto maybeMgmtSegmentId = m_roudiMemoryInterface.mgmtMemoryProvider()->segmentId();
    if (!maybeMgmtSegmentId.has_value())
    {
        LogFatal() << "Invalid state! Could not obtain SegmentId for iceoryx management segment!";
        fatalError = true;
    }
    m_mgmtSegmentId = maybeMgmtSegmentId.value();

    if (fatalError)
    {
        /// @todo #539 Use separate error enums once RouDi is more modular
        errorHandler(Error::kROUDI__PRECONDITIONS_FOR_PROCESS_MANAGER_NOT_FULFILLED, nullptr, ErrorLevel::FATAL);
    }
}

void ProcessManager::handleProcessShutdownPreparationRequest(const RuntimeName_t& name) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            m_portManager.unblockProcessShutdown(name);
            // Reply with PREPARE_APP_TERMINATION_ACK and let process shutdown
            runtime::IpcMessage sendBuffer;
            sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::PREPARE_APP_TERMINATION_ACK);
            process.sendViaIpcChannel(sendBuffer);
        },
        [&]() { LogWarn() << "Unknown application " << name << " requested shutdown preparation."; });
}

void ProcessManager::requestShutdownOfAllProcesses() noexcept
{
    // send SIG_TERM to all running applications and wait for processes to answer with TERMINATION
    for (auto& process : m_processList)
    {
        requestShutdownOfProcess(process, ShutdownPolicy::SIG_TERM);
    }

    // this unblocks the RouDi shutdown if a publisher port is blocked by a full subscriber queue
    m_portManager.unblockRouDiShutdown();
}

bool ProcessManager::isAnyRegisteredProcessStillRunning() noexcept
{
    for (auto& process : m_processList)
    {
        if (isProcessAlive(process))
        {
            return true;
        }
    }
    return false;
}

void ProcessManager::killAllProcesses() noexcept
{
    for (auto& process : m_processList)
    {
        LogWarn() << "Process ID " << process.getPid() << " named '" << process.getName()
                  << "' is still running after SIGTERM was sent. RouDi is sending SIGKILL now.";
        requestShutdownOfProcess(process, ShutdownPolicy::SIG_KILL);
    }
}

void ProcessManager::printWarningForRegisteredProcessesAndClearProcessList() noexcept
{
    for (auto& process : m_processList)
    {
        LogWarn() << "Process ID " << process.getPid() << " named '" << process.getName()
                  << "' is still running after SIGKILL was sent. RouDi is ignoring this process.";
    }
    m_processList.clear();
}

bool ProcessManager::requestShutdownOfProcess(Process& process, ShutdownPolicy shutdownPolicy) noexcept
{
    static constexpr int32_t ERROR_CODE = -1;
    auto killC = iox::cxx::makeSmartC(kill,
                                      iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                      {ERROR_CODE},
                                      {},
                                      static_cast<pid_t>(process.getPid()),
                                      (shutdownPolicy == ShutdownPolicy::SIG_KILL ? SIGKILL : SIGTERM));

    if (killC.hasErrors())
    {
        evaluateKillError(process, killC.getErrNum(), killC.getErrorString(), shutdownPolicy);
        return false;
    }
    return false;
}

bool ProcessManager::isProcessAlive(const Process& process) noexcept
{
    static constexpr int32_t ERROR_CODE = -1;
    auto checkCommand = iox::cxx::makeSmartC(kill,
                                             iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                             {ERROR_CODE},
                                             {ESRCH},
                                             static_cast<pid_t>(process.getPid()),
                                             SIGTERM);

    if (checkCommand.getErrNum() == ESRCH)
    {
        return false;
    }

    if (checkCommand.hasErrors())
    {
        evaluateKillError(process, checkCommand.getErrNum(), checkCommand.getErrorString(), ShutdownPolicy::SIG_TERM);
    }

    return true;
}

void ProcessManager::evaluateKillError(const Process& process,
                                       const int32_t& errnum,
                                       const char* errorString,
                                       ShutdownPolicy shutdownPolicy) noexcept
{
    if ((errnum == EINVAL) || (errnum == EPERM) || (errnum == ESRCH))
    {
        LogWarn() << "Process ID " << process.getPid() << " named '" << process.getName()
                  << "' could not be killed with "
                  << (shutdownPolicy == ShutdownPolicy::SIG_KILL ? "SIGKILL" : "SIGTERM")
                  << ", because the command failed with the following error: " << errorString
                  << " See manpage for kill(2) or type 'man 2 kill' in console for more information";
        errorHandler(Error::kPOSH__ROUDI_PROCESS_SHUTDOWN_FAILED, nullptr, ErrorLevel::SEVERE);
    }
    else
    {
        LogWarn() << "Process ID " << process.getPid() << " named '" << process.getName()
                  << "' could not be killed with"
                  << (shutdownPolicy == ShutdownPolicy::SIG_KILL ? "SIGKILL" : "SIGTERM") << " for unknown reason: ’"
                  << errorString << "'";
        errorHandler(Error::kPOSH__ROUDI_PROCESS_SHUTDOWN_FAILED, nullptr, ErrorLevel::SEVERE);
    }
}

bool ProcessManager::registerProcess(const RuntimeName_t& name,
                                     const uint32_t pid,
                                     const posix::PosixUser user,
                                     const bool isMonitored,
                                     const int64_t transmissionTimestamp,
                                     const uint64_t sessionId,
                                     const version::VersionInfo& versionInfo) noexcept
{
    bool returnValue{false};

    searchForProcessAndThen(
        name,
        [&](Process& process) {
            // process is already in list (i.e. registered)
            // depending on the mode we clean up the process resources and register it again
            // if it is monitored, we reject the registration and wait for automatic cleanup
            // otherwise we remove the process ourselves and register it again

            if (process.isMonitored())
            {
                LogWarn() << "Received register request, but termination of " << name << " not detected yet";
            }

            // process exists, we expect that the existing process crashed
            LogWarn() << "Application " << name << " crashed. Re-registering application";

            // remove the existing process and add the new process afterwards, we do not send ack to new process
            constexpr TerminationFeedback terminationFeedback{TerminationFeedback::DO_NOT_SEND_ACK_TO_PROCESS};
            if (!searchForProcessAndRemoveIt(name, terminationFeedback))
            {
                LogWarn() << "Application " << name << " could not be removed";
                return;
            }
            else
            {
                // try registration again, should succeed since removal was successful
                returnValue = addProcess(name, pid, user, isMonitored, transmissionTimestamp, sessionId, versionInfo);
            }
        },
        [&]() {
            // process does not exist in list and can be added
            returnValue = addProcess(name, pid, user, isMonitored, transmissionTimestamp, sessionId, versionInfo);
        });

    return returnValue;
}

bool ProcessManager::addProcess(const RuntimeName_t& name,
                                const uint32_t pid,
                                const posix::PosixUser& user,
                                const bool isMonitored,
                                const int64_t transmissionTimestamp,
                                const uint64_t sessionId,
                                const version::VersionInfo& versionInfo) noexcept
{
    if (!version::VersionInfo::getCurrentVersion().checkCompatibility(versionInfo, m_compatibilityCheckLevel))
    {
        LogError()
            << "Version mismatch from '" << name
            << "'! Please build your app and RouDi against the same iceoryx version (version & commitID). RouDi: "
            << version::VersionInfo::getCurrentVersion().operator iox::cxx::Serialization().toString()
            << " App: " << versionInfo.operator iox::cxx::Serialization().toString();
        return false;
    }
    // overflow check
    if (m_processList.size() >= MAX_PROCESS_NUMBER)
    {
        LogError() << "Could not register process '" << name << "' - too many processes";
        return false;
    }
    m_processList.emplace_back(name, pid, user, isMonitored, sessionId);

    // send REG_ACK and BaseAddrString
    runtime::IpcMessage sendBuffer;

    auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, m_segmentManager);
    sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::REG_ACK)
               << m_roudiMemoryInterface.mgmtMemoryProvider()->size() << offset << transmissionTimestamp
               << m_mgmtSegmentId;

    m_processList.back().sendViaIpcChannel(sendBuffer);

    // set current timestamp again (already done in Process's constructor
    m_processList.back().setTimestamp(mepoo::BaseClock_t::now());

    m_processIntrospection->addProcess(static_cast<int>(pid), RuntimeName_t(cxx::TruncateToCapacity, name.c_str()));

    LogDebug() << "Registered new application " << name;
    return true;
}

bool ProcessManager::unregisterProcess(const RuntimeName_t& name) noexcept
{
    constexpr TerminationFeedback feedback{TerminationFeedback::SEND_ACK_TO_PROCESS};
    if (!searchForProcessAndRemoveIt(name, feedback))
    {
        LogError() << "Application " << name << " could not be unregistered!";
        return false;
    }
    return true;
}

bool ProcessManager::searchForProcessAndRemoveIt(const RuntimeName_t& name, const TerminationFeedback feedback) noexcept
{
    // we need to search for the process (currently linear search)
    auto it = m_processList.begin();
    while (it != m_processList.end())
    {
        auto otherName = it->getName();
        if (name == otherName)
        {
            if (removeProcessAndDeleteRespectiveSharedMemoryObjects(it, feedback))
            {
                LogDebug() << "Removed existing application " << name;
            }
            return true; // we can assume there are no other processes with this name
        }
        ++it;
    }
    return false;
}

bool ProcessManager::removeProcessAndDeleteRespectiveSharedMemoryObjects(ProcessList_t::iterator& processIter,
                                                                         const TerminationFeedback feedback) noexcept
{
    if (processIter != m_processList.end())
    {
        m_portManager.deletePortsOfProcess(processIter->getName());
        m_processIntrospection->removeProcess(static_cast<int32_t>(processIter->getPid()));

        if (feedback == TerminationFeedback::SEND_ACK_TO_PROCESS)
        {
            // Reply with TERMINATION_ACK and let process shutdown
            runtime::IpcMessage sendBuffer;
            sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::TERMINATION_ACK);
            processIter->sendViaIpcChannel(sendBuffer);
        }

        processIter = m_processList.erase(processIter); // delete application
        return true;
    }
    return false;
}

void ProcessManager::updateLivelinessOfProcess(const RuntimeName_t& name) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            // reset timestamp
            process.setTimestamp(mepoo::BaseClock_t::now());
        },
        [&]() { LogWarn() << "Received Keepalive from unknown process " << name; });
}

void ProcessManager::findServiceForProcess(const RuntimeName_t& name, const capro::ServiceDescription& service) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            runtime::IpcMessage instanceString({m_portManager.findService(service)});
            process.sendViaIpcChannel(instanceString);
            LogDebug() << "Sent InstanceString to application " << name;
        },
        [&]() { LogWarn() << "Unknown process " << name << " requested an InstanceString."; });
}

void ProcessManager::addInterfaceForProcess(const RuntimeName_t& name,
                                            capro::Interfaces interface,
                                            const NodeName_t& node) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            // create a ReceiverPort
            popo::InterfacePortData* port = m_portManager.acquireInterfacePortData(interface, name, node);

            // send ReceiverPort to app as a serialized relative pointer
            auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, port);

            runtime::IpcMessage sendBuffer;
            sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::CREATE_INTERFACE_ACK)
                       << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
            process.sendViaIpcChannel(sendBuffer);

            LogDebug() << "Created new interface for application " << name;
        },
        [&]() { LogWarn() << "Unknown application " << name << " requested an interface."; });
}

void ProcessManager::sendServiceRegistryChangeCounterToProcess(const RuntimeName_t& runtimeName) noexcept
{
    searchForProcessAndThen(
        runtimeName,
        [&](Process& process) {
            // send counter to app as a serialized relative pointer
            auto offset =
                rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, m_portManager.serviceRegistryChangeCounter());

            runtime::IpcMessage sendBuffer;
            sendBuffer << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
            process.sendViaIpcChannel(sendBuffer);
        },
        [&]() { LogWarn() << "Unknown application " << runtimeName << " requested an serviceRegistryChangeCounter."; });
}

void ProcessManager::addApplicationForProcess(const RuntimeName_t& name) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            popo::ApplicationPortData* port = m_portManager.acquireApplicationPortData(name);

            auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, port);

            runtime::IpcMessage sendBuffer;
            sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::CREATE_APPLICATION_ACK)
                       << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
            process.sendViaIpcChannel(sendBuffer);

            LogDebug() << "Created new ApplicationPort for application " << name;
        },
        [&]() { LogWarn() << "Unknown application " << name << " requested an ApplicationPort." << name; });
}

void ProcessManager::addNodeForProcess(const RuntimeName_t& runtimeName, const NodeName_t& nodeName) noexcept
{
    searchForProcessAndThen(
        runtimeName,
        [&](Process& process) {
            m_portManager.acquireNodeData(runtimeName, nodeName)
                .and_then([&](auto nodeData) {
                    auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, nodeData);

                    runtime::IpcMessage sendBuffer;
                    sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::CREATE_NODE_ACK)
                               << std::to_string(offset) << std::to_string(m_mgmtSegmentId);

                    process.sendViaIpcChannel(sendBuffer);
                    m_processIntrospection->addNode(RuntimeName_t(cxx::TruncateToCapacity, runtimeName.c_str()),
                                                    NodeName_t(cxx::TruncateToCapacity, nodeName.c_str()));
                    LogDebug() << "Created new node " << nodeName << " for process " << runtimeName;
                })
                .or_else([&](PortPoolError error) {
                    runtime::IpcMessage sendBuffer;
                    sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::ERROR);
                    if (error == PortPoolError::NODE_DATA_LIST_FULL)
                    {
                        sendBuffer << runtime::IpcMessageErrorTypeToString(
                            runtime::IpcMessageErrorType::NODE_DATA_LIST_FULL);
                    }
                    process.sendViaIpcChannel(sendBuffer);

                    LogDebug() << "Could not create new node for process " << runtimeName;
                });
        },
        [&]() { LogWarn() << "Unknown process " << runtimeName << " requested a node."; });
}

void ProcessManager::sendMessageNotSupportedToRuntime(const RuntimeName_t& name) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            runtime::IpcMessage sendBuffer;
            sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::MESSAGE_NOT_SUPPORTED);
            process.sendViaIpcChannel(sendBuffer);

            LogError() << "Application " << name << " sent a message, which is not supported by this RouDi";
        },
        []() {});
}

void ProcessManager::addSubscriberForProcess(const RuntimeName_t& name,
                                             const capro::ServiceDescription& service,
                                             const popo::SubscriberOptions& subscriberOptions,
                                             const PortConfigInfo& portConfigInfo) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) {
            // create a SubscriberPort
            auto maybeSubscriber =
                m_portManager.acquireSubscriberPortData(service, subscriberOptions, name, portConfigInfo);

            if (!maybeSubscriber.has_error())
            {
                // send SubscriberPort to app as a serialized relative pointer
                auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, maybeSubscriber.value());

                runtime::IpcMessage sendBuffer;
                sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::CREATE_SUBSCRIBER_ACK)
                           << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
                process.sendViaIpcChannel(sendBuffer);

                LogDebug() << "Created new SubscriberPort for application " << name;
            }
            else
            {
                runtime::IpcMessage sendBuffer;
                sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::ERROR);
                sendBuffer << runtime::IpcMessageErrorTypeToString(runtime::IpcMessageErrorType::SUBSCRIBER_LIST_FULL);
                process.sendViaIpcChannel(sendBuffer);
                LogError() << "Could not create SubscriberPort for application " << name;
            }
        },
        [&]() { LogWarn() << "Unknown application " << name << " requested a SubscriberPort."; });
}

void ProcessManager::addPublisherForProcess(const RuntimeName_t& name,
                                            const capro::ServiceDescription& service,
                                            const popo::PublisherOptions& publisherOptions,
                                            const PortConfigInfo& portConfigInfo) noexcept
{
    searchForProcessAndThen(
        name,
        [&](Process& process) { // create a PublisherPort
            auto segmentInfo = m_segmentManager->getSegmentInformationWithWriteAccessForUser(process.getUser());

            if (!segmentInfo.m_memoryManager.has_value())
            {
                // Tell the app no writable shared memory segment was found
                runtime::IpcMessage sendBuffer;
                sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::ERROR);
                sendBuffer << runtime::IpcMessageErrorTypeToString(
                    runtime::IpcMessageErrorType::REQUEST_PUBLISHER_NO_WRITABLE_SHM_SEGMENT);
                process.sendViaIpcChannel(sendBuffer);
                return;
            }

            auto maybePublisher = m_portManager.acquirePublisherPortData(
                service, publisherOptions, name, &segmentInfo.m_memoryManager.value().get(), portConfigInfo);

            if (!maybePublisher.has_error())
            {
                // send PublisherPort to app as a serialized relative pointer
                auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, maybePublisher.value());

                runtime::IpcMessage sendBuffer;
                sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::CREATE_PUBLISHER_ACK)
                           << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
                process.sendViaIpcChannel(sendBuffer);

                LogDebug() << "Created new PublisherPort for application " << name;
            }
            else
            {
                runtime::IpcMessage sendBuffer;
                sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::ERROR);
                sendBuffer << runtime::IpcMessageErrorTypeToString( // map error codes
                    (maybePublisher.get_error() == PortPoolError::UNIQUE_PUBLISHER_PORT_ALREADY_EXISTS
                         ? runtime::IpcMessageErrorType::NO_UNIQUE_CREATED
                         : runtime::IpcMessageErrorType::PUBLISHER_LIST_FULL));
                process.sendViaIpcChannel(sendBuffer);
                LogError() << "Could not create PublisherPort for application " << name;
            }
        },
        [&]() { LogWarn() << "Unknown application " << name << " requested a PublisherPort."; });
}

void ProcessManager::addConditionVariableForProcess(const RuntimeName_t& runtimeName) noexcept
{
    searchForProcessAndThen(
        runtimeName,
        [&](Process& process) { // Try to create a condition variable
            m_portManager.acquireConditionVariableData(runtimeName)
                .and_then([&](auto condVar) {
                    auto offset = rp::BaseRelativePointer::getOffset(m_mgmtSegmentId, condVar);

                    runtime::IpcMessage sendBuffer;
                    sendBuffer << runtime::IpcMessageTypeToString(
                        runtime::IpcMessageType::CREATE_CONDITION_VARIABLE_ACK)
                               << std::to_string(offset) << std::to_string(m_mgmtSegmentId);
                    process.sendViaIpcChannel(sendBuffer);

                    LogDebug() << "Created new ConditionVariable for application " << runtimeName;
                })
                .or_else([&](PortPoolError error) {
                    runtime::IpcMessage sendBuffer;
                    sendBuffer << runtime::IpcMessageTypeToString(runtime::IpcMessageType::ERROR);
                    if (error == PortPoolError::CONDITION_VARIABLE_LIST_FULL)
                    {
                        sendBuffer << runtime::IpcMessageErrorTypeToString(
                            runtime::IpcMessageErrorType::CONDITION_VARIABLE_LIST_FULL);
                    }
                    process.sendViaIpcChannel(sendBuffer);

                    LogDebug() << "Could not create new ConditionVariable for application " << runtimeName;
                });
        },
        [&]() { LogWarn() << "Unknown application " << runtimeName << " requested a ConditionVariable."; });
}

void ProcessManager::initIntrospection(ProcessIntrospectionType* processIntrospection) noexcept
{
    m_processIntrospection = processIntrospection;
}

void ProcessManager::run() noexcept
{
    monitorProcesses();
    discoveryUpdate();
}

popo::PublisherPortData* ProcessManager::addIntrospectionPublisherPort(const capro::ServiceDescription& service,
                                                                       const RuntimeName_t& process_name) noexcept
{
    popo::PublisherOptions options;
    options.historyCapacity = 1;
    options.nodeName = INTROSPECTION_NODE_NAME;
    auto maybePublisher = m_portManager.acquirePublisherPortData(
        service, options, process_name, m_introspectionMemoryManager, PortConfigInfo());

    if (maybePublisher.has_error())
    {
        LogError() << "Could not create PublisherPort for application " << process_name;
        errorHandler(
            Error::kPORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTION_SENDER_PORT, nullptr, iox::ErrorLevel::SEVERE);
    }
    return maybePublisher.value();
}

bool ProcessManager::searchForProcessAndThen(const RuntimeName_t& name,
                                             cxx::function_ref<void(Process&)> AndThenCallable,
                                             cxx::function_ref<void()> OrElseCallable) noexcept
{
    typename ProcessList_t::iterator it = m_processList.begin();
    const typename ProcessList_t::iterator itEnd = m_processList.end();

    for (; itEnd != it; ++it)
    {
        if (name == it->getName())
        {
            if (AndThenCallable)
            {
                AndThenCallable(*it);
                return true;
            }
        }
    }
    if (OrElseCallable)
    {
        OrElseCallable();
    }
    return false;
}

void ProcessManager::monitorProcesses() noexcept
{
    auto currentTimestamp = mepoo::BaseClock_t::now();

    auto processIterator = m_processList.begin();
    while (processIterator != m_processList.end())
    {
        if (processIterator->isMonitored())
        {
            auto timediff = units::Duration(currentTimestamp - processIterator->getTimestamp());

            static_assert(runtime::PROCESS_KEEP_ALIVE_TIMEOUT > runtime::PROCESS_KEEP_ALIVE_INTERVAL,
                          "keep alive timeout too small");
            if (timediff > runtime::PROCESS_KEEP_ALIVE_TIMEOUT)
            {
                LogWarn() << "Application " << processIterator->getName() << " not responding (last response "
                          << timediff.toMilliseconds() << " milliseconds ago) --> removing it";

                // note: if we would want to use the removeProcess function, it would search for the process again
                // (but we already found it and have an iterator to remove it)

                // delete all associated subscriber and publisher ports in shared
                // memory and the associated RouDi discovery ports
                // @todo Check if ShmManager and Process Manager end up in unintended condition
                m_portManager.deletePortsOfProcess(processIterator->getName());

                m_processIntrospection->removeProcess(static_cast<int32_t>(processIterator->getPid()));

                // delete application
                processIterator = m_processList.erase(processIterator);
                continue; // erase returns first element after the removed one --> skip iterator increment
            }
        }
        ++processIterator;
    }
}

void ProcessManager::discoveryUpdate() noexcept
{
    m_portManager.doDiscovery();
}

} // namespace roudi
} // namespace iox
