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

#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/runtime/ipc_message.hpp"
#include "iceoryx_posh/runtime/node.hpp"
#include "iceoryx_posh/runtime/port_config_info.hpp"
#include "iceoryx_utils/cxx/convert.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/base_relative_pointer.hpp"
#include "iceoryx_utils/posix_wrapper/timer.hpp"

#include <cstdint>

namespace iox
{
namespace runtime
{
PoshRuntime::factory_t& PoshRuntime::getRuntimeFactory() noexcept
{
    static factory_t runtimeFactory = PoshRuntime::defaultRuntimeFactory;
    return runtimeFactory;
}

void PoshRuntime::setRuntimeFactory(const factory_t& factory) noexcept
{
    if (factory)
    {
        PoshRuntime::getRuntimeFactory() = factory;
    }
    else
    {
        LogFatal() << "Cannot set runtime factory. Passed factory must not be empty!";
        errorHandler(Error::kPOSH__RUNTIME_FACTORY_IS_NOT_SET);
    }
}

PoshRuntime& PoshRuntime::defaultRuntimeFactory(cxx::optional<const RuntimeName_t*> name) noexcept
{
    static PoshRuntime instance(name);
    return instance;
}

// singleton access
PoshRuntime& PoshRuntime::getInstance() noexcept
{
    return getInstance(cxx::nullopt);
}

PoshRuntime& PoshRuntime::initRuntime(const RuntimeName_t& name) noexcept
{
    return getInstance(cxx::make_optional<const RuntimeName_t*>(&name));
}

PoshRuntime& PoshRuntime::getInstance(cxx::optional<const RuntimeName_t*> name) noexcept
{
    return getRuntimeFactory()(name);
}

PoshRuntime::PoshRuntime(cxx::optional<const RuntimeName_t*> name, const bool doMapSharedMemoryIntoThread) noexcept
    : m_appName(verifyInstanceName(name))
    , m_ipcChannelInterface(roudi::IPC_CHANNEL_ROUDI_NAME, *name.value(), runtime::PROCESS_WAITING_FOR_ROUDI_TIMEOUT)
    , m_ShmInterface(doMapSharedMemoryIntoThread,
                     m_ipcChannelInterface.getShmTopicSize(),
                     m_ipcChannelInterface.getSegmentId(),
                     m_ipcChannelInterface.getSegmentManagerAddressOffset())
    , m_applicationPort(getMiddlewareApplication())
{
    if (cxx::isCompiledOn32BitSystem())
    {
        LogWarn() << "Running applications on 32-bit architectures is not supported! Use at your own risk!";
    }

    /// @todo here we could get the LogLevel and LogMode and set it on the LogManager
}

PoshRuntime::~PoshRuntime() noexcept
{
    // Inform RouDi that we're shutting down
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::TERMINATION) << m_appName;
    IpcMessage receiveBuffer;

    if (m_ipcChannelInterface.sendRequestToRouDi(sendBuffer, receiveBuffer)
        && (1U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::TERMINATION_ACK)
        {
            LogVerbose() << "RouDi cleaned up resources of " << m_appName << ". Shutting down gracefully.";
        }
        else
        {
            LogError() << "Got wrong response from IPC channel for IpcMessageType::TERMINATION:'"
                       << receiveBuffer.getMessage() << "'";
        }
    }
    else
    {
        LogError() << "Sending IpcMessageType::TERMINATION to RouDi failed:'" << receiveBuffer.getMessage() << "'";
    }
}


const RuntimeName_t& PoshRuntime::verifyInstanceName(cxx::optional<const RuntimeName_t*> name) noexcept
{
    if (!name.has_value())
    {
        LogError() << "Cannot initialize runtime. Application name has not been specified!";
        errorHandler(Error::kPOSH__RUNTIME_NO_NAME_PROVIDED, nullptr, ErrorLevel::FATAL);
    }
    else if (name.value()->empty())
    {
        LogError() << "Cannot initialize runtime. Application name must not be empty!";
        errorHandler(Error::kPOSH__RUNTIME_NAME_EMPTY, nullptr, ErrorLevel::FATAL);
    }
    else if (name.value()->c_str()[0] == '/')
    {
        LogError() << "Cannot initialize runtime. Please remove leading slash from Application name " << *name.value();
        errorHandler(Error::kPOSH__RUNTIME_LEADING_SLASH_PROVIDED, nullptr, ErrorLevel::FATAL);
    }

    return *name.value();
}

RuntimeName_t PoshRuntime::getInstanceName() const noexcept
{
    return m_appName;
}

void PoshRuntime::shutdown() noexcept
{
    m_shutdownRequested.store(true, std::memory_order_relaxed);
}

const std::atomic<uint64_t>* PoshRuntime::getServiceRegistryChangeCounter() noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::SERVICE_REGISTRY_CHANGE_COUNTER) << m_appName;
    IpcMessage receiveBuffer;
    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (2U == receiveBuffer.getNumberOfElements()))
    {
        rp::BaseRelativePointer::offset_t offset{0U};
        cxx::convert::fromString(receiveBuffer.getElementAtIndex(0U).c_str(), offset);
        rp::BaseRelativePointer::id_t segmentId{0U};
        cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), segmentId);
        auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);

        return reinterpret_cast<std::atomic<uint64_t>*>(ptr);
    }
    else
    {
        LogError() << "unable to request service registry change counter caused by wrong response from RouDi: \""
                   << receiveBuffer.getMessage() << "\" with request: \"" << sendBuffer.getMessage() << "\"";
        return nullptr;
    }
}

PublisherPortUserType::MemberType_t* PoshRuntime::getMiddlewarePublisher(const capro::ServiceDescription& service,
                                                                         const popo::PublisherOptions& publisherOptions,
                                                                         const PortConfigInfo& portConfigInfo) noexcept
{
    constexpr uint64_t MAX_HISTORY_CAPACITY =
        PublisherPortUserType::MemberType_t::ChunkSenderData_t::ChunkDistributorDataProperties_t::MAX_HISTORY_CAPACITY;

    auto options = publisherOptions;
    if (options.historyCapacity > MAX_HISTORY_CAPACITY)
    {
        LogWarn() << "Requested history capacity " << options.historyCapacity
                  << " exceeds the maximum possible one for this publisher"
                  << ", limiting from " << publisherOptions.historyCapacity << " to " << MAX_HISTORY_CAPACITY;
        options.historyCapacity = MAX_HISTORY_CAPACITY;
    }

    if (options.nodeName.empty())
    {
        options.nodeName = m_appName;
    }

    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_PUBLISHER) << m_appName
               << static_cast<cxx::Serialization>(service).toString() << std::to_string(options.historyCapacity)
               << options.nodeName << std::to_string(options.offerOnCreate)
               << std::to_string(static_cast<uint8_t>(options.subscriberTooSlowPolicy))
               << static_cast<cxx::Serialization>(portConfigInfo).toString();

    auto maybePublisher = requestPublisherFromRoudi(sendBuffer);
    if (maybePublisher.has_error())
    {
        switch (maybePublisher.get_error())
        {
        case IpcMessageErrorType::NO_UNIQUE_CREATED:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' already in use by another process.";
            errorHandler(Error::kPOSH__RUNTIME_PUBLISHER_PORT_NOT_UNIQUE, nullptr, iox::ErrorLevel::SEVERE);
            break;
        case IpcMessageErrorType::PUBLISHER_LIST_FULL:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' could not be created since we are out of memory for publishers.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_PUBLISHER_LIST_FULL, nullptr, iox::ErrorLevel::SEVERE);
            break;
        case IpcMessageErrorType::REQUEST_PUBLISHER_WRONG_IPC_MESSAGE_RESPONSE:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' could not be created. Request publisher got wrong IPC channel response.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_REQUEST_PUBLISHER_WRONG_IPC_MESSAGE_RESPONSE,
                         nullptr,
                         iox::ErrorLevel::SEVERE);
            break;
        case IpcMessageErrorType::REQUEST_PUBLISHER_NO_WRITABLE_SHM_SEGMENT:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' could not be created. RouDi did not find a writable shared memory segment for the current "
                         "user. Try using another user or adapt RouDi's config.";
            errorHandler(Error::kPOSH__RUNTIME_NO_WRITABLE_SHM_SEGMENT, nullptr, iox::ErrorLevel::SEVERE);
            break;
        default:
            LogWarn() << "Undefined behavior occurred while creating service '"
                      << service.operator cxx::Serialization().toString() << "'.";
            errorHandler(
                Error::kPOSH__RUNTIME_PUBLISHER_PORT_CREATION_UNDEFINED_BEHAVIOR, nullptr, iox::ErrorLevel::SEVERE);
            break;
        }
        return nullptr;
    }
    return maybePublisher.value();
}

cxx::expected<PublisherPortUserType::MemberType_t*, IpcMessageErrorType>
PoshRuntime::requestPublisherFromRoudi(const IpcMessage& sendBuffer) noexcept
{
    IpcMessage receiveBuffer;
    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_PUBLISHER_ACK)

        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return cxx::success<PublisherPortUserType::MemberType_t*>(
                reinterpret_cast<PublisherPortUserType::MemberType_t*>(ptr));
        }
    }
    else
    {
        if (receiveBuffer.getNumberOfElements() == 2U)
        {
            std::string IpcMessage1 = receiveBuffer.getElementAtIndex(0U);
            std::string IpcMessage2 = receiveBuffer.getElementAtIndex(1U);
            if (stringToIpcMessageType(IpcMessage1.c_str()) == IpcMessageType::ERROR)
            {
                LogError() << "Request publisher received no valid publisher port from RouDi.";
                return cxx::error<IpcMessageErrorType>(stringToIpcMessageErrorType(IpcMessage2.c_str()));
            }
        }
    }

    LogError() << "Request publisher got wrong response from IPC channel :'" << receiveBuffer.getMessage() << "'";
    return cxx::error<IpcMessageErrorType>(IpcMessageErrorType::REQUEST_PUBLISHER_WRONG_IPC_MESSAGE_RESPONSE);
}

SubscriberPortUserType::MemberType_t*
PoshRuntime::getMiddlewareSubscriber(const capro::ServiceDescription& service,
                                     const popo::SubscriberOptions& subscriberOptions,
                                     const PortConfigInfo& portConfigInfo) noexcept
{
    constexpr uint64_t MAX_QUEUE_CAPACITY = SubscriberPortUserType::MemberType_t::ChunkQueueData_t::MAX_CAPACITY;

    auto options = subscriberOptions;
    if (options.queueCapacity > MAX_QUEUE_CAPACITY)
    {
        LogWarn() << "Requested queue capacity " << options.queueCapacity
                  << " exceeds the maximum possible one for this subscriber"
                  << ", limiting from " << subscriberOptions.queueCapacity << " to " << MAX_QUEUE_CAPACITY;
        options.queueCapacity = MAX_QUEUE_CAPACITY;
    }
    else if (0U == options.queueCapacity)
    {
        LogWarn() << "Requested queue capacity of 0 doesn't make sense as no data would be received,"
                  << " the capacity is set to 1";
        options.queueCapacity = 1U;
    }

    if (options.nodeName.empty())
    {
        options.nodeName = m_appName;
    }

    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_SUBSCRIBER) << m_appName
               << static_cast<cxx::Serialization>(service).toString() << std::to_string(options.historyRequest)
               << std::to_string(options.queueCapacity) << options.nodeName << std::to_string(options.subscribeOnCreate)
               << std::to_string(static_cast<uint8_t>(options.queueFullPolicy))
               << static_cast<cxx::Serialization>(portConfigInfo).toString();

    auto maybeSubscriber = requestSubscriberFromRoudi(sendBuffer);

    if (maybeSubscriber.has_error())
    {
        switch (maybeSubscriber.get_error())
        {
        case IpcMessageErrorType::SUBSCRIBER_LIST_FULL:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' could not be created since we are out of memory for subscribers.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_SUBSCRIBER_LIST_FULL, nullptr, iox::ErrorLevel::SEVERE);
            break;
        case IpcMessageErrorType::REQUEST_SUBSCRIBER_WRONG_IPC_MESSAGE_RESPONSE:
            LogWarn() << "Service '" << service.operator cxx::Serialization().toString()
                      << "' could not be created. Request subscriber got wrong IPC channel response.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_REQUEST_SUBSCRIBER_WRONG_IPC_MESSAGE_RESPONSE,
                         nullptr,
                         iox::ErrorLevel::SEVERE);
            break;
        default:
            LogWarn() << "Undefined behavior occurred while creating service '"
                      << service.operator cxx::Serialization().toString() << "'.";
            errorHandler(
                Error::kPOSH__RUNTIME_SUBSCRIBER_PORT_CREATION_UNDEFINED_BEHAVIOR, nullptr, iox::ErrorLevel::SEVERE);
            break;
        }
        return nullptr;
    }
    return maybeSubscriber.value();
}

cxx::expected<SubscriberPortUserType::MemberType_t*, IpcMessageErrorType>
PoshRuntime::requestSubscriberFromRoudi(const IpcMessage& sendBuffer) noexcept
{
    IpcMessage receiveBuffer;
    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_SUBSCRIBER_ACK)
        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return cxx::success<SubscriberPortUserType::MemberType_t*>(
                reinterpret_cast<SubscriberPortUserType::MemberType_t*>(ptr));
        }
    }
    else
    {
        if (receiveBuffer.getNumberOfElements() == 2U)
        {
            std::string IpcMessage1 = receiveBuffer.getElementAtIndex(0U);
            std::string IpcMessage2 = receiveBuffer.getElementAtIndex(1U);

            if (stringToIpcMessageType(IpcMessage1.c_str()) == IpcMessageType::ERROR)
            {
                LogError() << "Request subscriber received no valid subscriber port from RouDi.";
                return cxx::error<IpcMessageErrorType>(stringToIpcMessageErrorType(IpcMessage2.c_str()));
            }
        }
    }

    LogError() << "Request subscriber got wrong response from IPC channel :'" << receiveBuffer.getMessage() << "'";
    return cxx::error<IpcMessageErrorType>(IpcMessageErrorType::REQUEST_SUBSCRIBER_WRONG_IPC_MESSAGE_RESPONSE);
}

popo::InterfacePortData* PoshRuntime::getMiddlewareInterface(const capro::Interfaces interface,
                                                             const NodeName_t& nodeName) noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_INTERFACE) << m_appName
               << static_cast<uint32_t>(interface) << nodeName;

    IpcMessage receiveBuffer;

    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_INTERFACE_ACK)
        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return reinterpret_cast<popo::InterfacePortData*>(ptr);
        }
    }

    LogError() << "Get mw interface got wrong response from IPC channel :'" << receiveBuffer.getMessage() << "'";
    errorHandler(
        Error::kPOSH__RUNTIME_ROUDI_GET_MW_INTERFACE_WRONG_IPC_MESSAGE_RESPONSE, nullptr, iox::ErrorLevel::SEVERE);
    return nullptr;
}

NodeData* PoshRuntime::createNode(const NodeProperty& nodeProperty) noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_NODE) << m_appName
               << static_cast<cxx::Serialization>(nodeProperty).toString();

    IpcMessage receiveBuffer;

    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_NODE_ACK)
        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return reinterpret_cast<NodeData*>(ptr);
        }
    }

    LogError() << "Got wrong response from RouDi while creating node:'" << receiveBuffer.getMessage() << "'";
    errorHandler(Error::kPOSH__RUNTIME_ROUDI_CREATE_NODE_WRONG_IPC_MESSAGE_RESPONSE, nullptr, iox::ErrorLevel::SEVERE);
    return nullptr;
}

cxx::expected<InstanceContainer, FindServiceError>
PoshRuntime::findService(const capro::ServiceDescription& serviceDescription) noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::FIND_SERVICE) << m_appName
               << static_cast<cxx::Serialization>(serviceDescription).toString();

    IpcMessage requestResponse;

    if (!sendRequestToRouDi(sendBuffer, requestResponse))
    {
        LogError() << "Could not send FIND_SERVICE request to RouDi\n";
        errorHandler(Error::kIPC_INTERFACE__REG_UNABLE_TO_WRITE_TO_ROUDI_CHANNEL, nullptr, ErrorLevel::MODERATE);
        return cxx::error<FindServiceError>(FindServiceError::UNABLE_TO_WRITE_TO_ROUDI_CHANNEL);
    }

    InstanceContainer instanceContainer;
    uint32_t numberOfElements = requestResponse.getNumberOfElements();
    uint32_t capacity = static_cast<uint32_t>(instanceContainer.capacity());

    // Limit the instances (max value is the capacity of instanceContainer)
    uint32_t numberOfInstances = ((numberOfElements > capacity) ? capacity : numberOfElements);
    for (uint32_t i = 0; i < numberOfInstances; ++i)
    {
        capro::IdString_t instance(iox::cxx::TruncateToCapacity, requestResponse.getElementAtIndex(i).c_str());
        instanceContainer.push_back(instance);
    }

    if (numberOfElements > capacity)
    {
        LogWarn() << numberOfElements << " instances found for service \"" << serviceDescription.getServiceIDString()
                  << "\" which is more than supported number of instances(" << MAX_NUMBER_OF_INSTANCES << "\n";
        errorHandler(Error::kPOSH__SERVICE_DISCOVERY_INSTANCE_CONTAINER_OVERFLOW, nullptr, ErrorLevel::MODERATE);
        return cxx::error<FindServiceError>(FindServiceError::INSTANCE_CONTAINER_OVERFLOW);
    }
    return {cxx::success<InstanceContainer>(instanceContainer)};
}


bool PoshRuntime::offerService(const capro::ServiceDescription& serviceDescription) noexcept
{
    if (serviceDescription.isValid())
    {
        capro::CaproMessage msg(
            capro::CaproMessageType::OFFER, serviceDescription, capro::CaproMessageSubType::SERVICE);
        m_applicationPort.dispatchCaProMessage(msg);
        return true;
    }
    LogWarn() << "Could not offer service " << serviceDescription.getServiceIDString() << ","
              << " ServiceDescription is invalid\n";
    return false;
}

void PoshRuntime::stopOfferService(const capro::ServiceDescription& serviceDescription) noexcept
{
    capro::CaproMessage msg(
        capro::CaproMessageType::STOP_OFFER, serviceDescription, capro::CaproMessageSubType::SERVICE);
    m_applicationPort.dispatchCaProMessage(msg);
}

popo::ApplicationPortData* PoshRuntime::getMiddlewareApplication() noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_APPLICATION) << m_appName;

    IpcMessage receiveBuffer;

    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_APPLICATION_ACK)
        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return reinterpret_cast<popo::ApplicationPortData*>(ptr);
        }
    }

    LogError() << "Get mw application got wrong response from IPC channel :'" << receiveBuffer.getMessage() << "'";
    errorHandler(
        Error::kPOSH__RUNTIME_ROUDI_GET_MW_APPLICATION_WRONG_IPC_MESSAGE_RESPONSE, nullptr, iox::ErrorLevel::SEVERE);
    return nullptr;
}

cxx::expected<popo::ConditionVariableData*, IpcMessageErrorType>
PoshRuntime::requestConditionVariableFromRoudi(const IpcMessage& sendBuffer) noexcept
{
    IpcMessage receiveBuffer;
    if (sendRequestToRouDi(sendBuffer, receiveBuffer) && (3U == receiveBuffer.getNumberOfElements()))
    {
        std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

        if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::CREATE_CONDITION_VARIABLE_ACK)
        {
            rp::BaseRelativePointer::id_t segmentId{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), segmentId);
            rp::BaseRelativePointer::offset_t offset{0U};
            cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), offset);
            auto ptr = rp::BaseRelativePointer::getPtr(segmentId, offset);
            return cxx::success<popo::ConditionVariableData*>(reinterpret_cast<popo::ConditionVariableData*>(ptr));
        }
    }
    else
    {
        if (receiveBuffer.getNumberOfElements() == 2U)
        {
            std::string IpcMessage1 = receiveBuffer.getElementAtIndex(0U);
            std::string IpcMessage2 = receiveBuffer.getElementAtIndex(1U);
            if (stringToIpcMessageType(IpcMessage1.c_str()) == IpcMessageType::ERROR)
            {
                LogError() << "Request condition variable received no valid condition variable port from RouDi.";
                return cxx::error<IpcMessageErrorType>(stringToIpcMessageErrorType(IpcMessage2.c_str()));
            }
        }
    }

    LogError() << "Request condition variable got wrong response from IPC channel :'" << receiveBuffer.getMessage()
               << "'";
    return cxx::error<IpcMessageErrorType>(IpcMessageErrorType::REQUEST_CONDITION_VARIABLE_WRONG_IPC_MESSAGE_RESPONSE);
}

popo::ConditionVariableData* PoshRuntime::getMiddlewareConditionVariable() noexcept
{
    IpcMessage sendBuffer;
    sendBuffer << IpcMessageTypeToString(IpcMessageType::CREATE_CONDITION_VARIABLE) << m_appName;

    auto maybeConditionVariable = requestConditionVariableFromRoudi(sendBuffer);
    if (maybeConditionVariable.has_error())
    {
        switch (maybeConditionVariable.get_error())
        {
        case IpcMessageErrorType::CONDITION_VARIABLE_LIST_FULL:
            LogWarn() << "Could not create condition variable as we are out of memory for condition variables.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_CONDITION_VARIABLE_LIST_FULL, nullptr, iox::ErrorLevel::SEVERE);
            break;
        case IpcMessageErrorType::REQUEST_CONDITION_VARIABLE_WRONG_IPC_MESSAGE_RESPONSE:
            LogWarn() << "Could not create condition variables; received wrong IPC channel response.";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_REQUEST_CONDITION_VARIABLE_WRONG_IPC_MESSAGE_RESPONSE,
                         nullptr,
                         iox::ErrorLevel::SEVERE);
            break;
        default:
            LogWarn() << "Undefined behavior occurred while creating condition variable";
            errorHandler(Error::kPOSH__RUNTIME_ROUDI_CONDITION_VARIABLE_CREATION_UNDEFINED_BEHAVIOR,
                         nullptr,
                         iox::ErrorLevel::SEVERE);
            break;
        }
        return nullptr;
    }
    return maybeConditionVariable.value();
}

bool PoshRuntime::sendRequestToRouDi(const IpcMessage& msg, IpcMessage& answer) noexcept
{
    // runtime must be thread safe
    std::lock_guard<std::mutex> g(m_appIpcRequestMutex);
    return m_ipcChannelInterface.sendRequestToRouDi(msg, answer);
}

// this is the callback for the m_keepAliveTimer
void PoshRuntime::sendKeepAliveAndHandleShutdownPreparation() noexcept
{
    if (!m_ipcChannelInterface.sendKeepalive())
    {
        LogWarn() << "Error in sending keep alive";
    }

    // this is not the nicest solution, but we cannot send this in the signal handler where m_shutdownRequested is
    // usually set; luckily the runtime already has a thread running and therefore this thread is used to unblock the
    // application shutdown from a potentially blocking publisher with the
    // SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER option set
    if (m_shutdownRequested.exchange(false, std::memory_order_relaxed))
    {
        // Inform RouDi to prepare for app shutdown
        IpcMessage sendBuffer;
        sendBuffer << IpcMessageTypeToString(IpcMessageType::PREPARE_APP_TERMINATION) << m_appName;
        IpcMessage receiveBuffer;

        if (m_ipcChannelInterface.sendRequestToRouDi(sendBuffer, receiveBuffer)
            && (1U == receiveBuffer.getNumberOfElements()))
        {
            std::string IpcMessage = receiveBuffer.getElementAtIndex(0U);

            if (stringToIpcMessageType(IpcMessage.c_str()) == IpcMessageType::PREPARE_APP_TERMINATION_ACK)
            {
                LogVerbose() << "RouDi unblocked shutdown of " << m_appName << ".";
            }
            else
            {
                LogError() << "Got wrong response from IPC channel for PREPARE_APP_TERMINATION:'"
                           << receiveBuffer.getMessage() << "'";
            }
        }
        else
        {
            LogError() << "Sending IpcMessageType::PREPARE_APP_TERMINATION to RouDi failed:'"
                       << receiveBuffer.getMessage() << "'";
        }
    }
}

} // namespace runtime
} // namespace iox
