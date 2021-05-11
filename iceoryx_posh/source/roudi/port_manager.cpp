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

#include "iceoryx_posh/internal/roudi/port_manager.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/popo/publisher_options.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_posh/runtime/node.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

#include <cstdint>

namespace iox
{
namespace roudi
{
capro::Interfaces StringToCaProInterface(const capro::IdString_t& str) noexcept
{
    int32_t i{0};
    cxx::convert::fromString(str.c_str(), i);
    if (i >= static_cast<int32_t>(capro::Interfaces::INTERFACE_END))
    {
        LogWarn() << "invalid enum (out of range: " << i << ")";
        return capro::Interfaces::INTERNAL;
    }
    return static_cast<capro::Interfaces>(i);
}

PortManager::PortManager(RouDiMemoryInterface* roudiMemoryInterface) noexcept
{
    m_roudiMemoryInterface = roudiMemoryInterface;

    auto maybePortPool = m_roudiMemoryInterface->portPool();
    if (!maybePortPool.has_value())
    {
        LogFatal() << "Could not get PortPool!";
        errorHandler(Error::kPORT_MANAGER__PORT_POOL_UNAVAILABLE, nullptr, iox::ErrorLevel::FATAL);
    }
    m_portPool = maybePortPool.value();

    auto maybeIntrospectionMemoryManager = m_roudiMemoryInterface->introspectionMemoryManager();
    if (!maybeIntrospectionMemoryManager.has_value())
    {
        LogFatal() << "Could not get MemoryManager for introspection!";
        errorHandler(Error::kPORT_MANAGER__INTROSPECTION_MEMORY_MANAGER_UNAVAILABLE, nullptr, iox::ErrorLevel::FATAL);
    }
    auto introspectionMemoryManager = maybeIntrospectionMemoryManager.value();

    popo::PublisherOptions options;
    options.historyCapacity = 1;
    options.nodeName = INTROSPECTION_NODE_NAME;
    // Remark: m_portIntrospection is not fully functional in base class RouDiBase (has no active publisher port)
    // are there used instances of RouDiBase?
    auto maybePublisher = acquirePublisherPortData(
        IntrospectionPortService, options, IPC_CHANNEL_ROUDI_NAME, introspectionMemoryManager, PortConfigInfo());
    if (maybePublisher.has_error())
    {
        LogError() << "Could not create PublisherPort for IntrospectionPortService";
        errorHandler(
            Error::kPORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONPORTSERVICE, nullptr, iox::ErrorLevel::SEVERE);
    }
    auto portGeneric = maybePublisher.value();

    maybePublisher = acquirePublisherPortData(IntrospectionPortThroughputService,
                                              options,
                                              IPC_CHANNEL_ROUDI_NAME,
                                              introspectionMemoryManager,
                                              PortConfigInfo());
    if (maybePublisher.has_error())
    {
        LogError() << "Could not create PublisherPort for IntrospectionPortThroughputService";
        errorHandler(Error::kPORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONPORTTHROUGHPUTSERVICE,
                     nullptr,
                     iox::ErrorLevel::SEVERE);
    }
    auto portThroughput = maybePublisher.value();

    maybePublisher = acquirePublisherPortData(IntrospectionSubscriberPortChangingDataService,
                                              options,
                                              IPC_CHANNEL_ROUDI_NAME,
                                              introspectionMemoryManager,
                                              PortConfigInfo());
    if (maybePublisher.has_error())
    {
        LogError() << "Could not create PublisherPort for IntrospectionSubscriberPortChangingDataService";
        errorHandler(Error::kPORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONCHANGINGDATASERVICE,
                     nullptr,
                     iox::ErrorLevel::SEVERE);
    }
    auto subscriberPortsData = maybePublisher.value();

    m_portIntrospection.registerPublisherPort(PublisherPortUserType(std::move(portGeneric)),
                                              PublisherPortUserType(std::move(portThroughput)),
                                              PublisherPortUserType(std::move(subscriberPortsData)));
    m_portIntrospection.run();
}

void PortManager::stopPortIntrospection() noexcept
{
    m_portIntrospection.stop();
}

void PortManager::doDiscovery() noexcept
{
    handlePublisherPorts();

    handleSubscriberPorts();

    handleApplications();

    handleInterfaces();

    handleNodes();

    handleConditionVariables();
}

void PortManager::handlePublisherPorts() noexcept
{
    // get the changes of publisher port offer state
    for (auto publisherPortData : m_portPool->getPublisherPortDataList())
    {
        PublisherPortRouDiType publisherPort(publisherPortData);

        doDiscoveryForPublisherPort(publisherPort);

        // check if we have to destroy this publisher port
        if (publisherPort.toBeDestroyed())
        {
            destroyPublisherPort(publisherPortData);
        }
    }
}

void PortManager::doDiscoveryForPublisherPort(PublisherPortRouDiType& publisherPort) noexcept
{
    publisherPort.tryGetCaProMessage().and_then([this, &publisherPort](auto caproMessage) {
        m_portIntrospection.reportMessage(caproMessage);
        if (capro::CaproMessageType::OFFER == caproMessage.m_type)
        {
            this->addEntryToServiceRegistry(caproMessage.m_serviceDescription.getServiceIDString(),
                                            caproMessage.m_serviceDescription.getInstanceIDString());
        }
        else if (capro::CaproMessageType::STOP_OFFER == caproMessage.m_type)
        {
            this->removeEntryFromServiceRegistry(caproMessage.m_serviceDescription.getServiceIDString(),
                                                 caproMessage.m_serviceDescription.getInstanceIDString());
        }
        else
        {
            // protocol error
            errorHandler(
                Error::kPORT_MANAGER__HANDLE_PUBLISHER_PORTS_INVALID_CAPRO_MESSAGE, nullptr, iox::ErrorLevel::MODERATE);
        }

        this->sendToAllMatchingSubscriberPorts(caproMessage, publisherPort);
        // forward to interfaces
        this->sendToAllMatchingInterfacePorts(caproMessage);
    });
}

void PortManager::handleSubscriberPorts() noexcept
{
    // get requests for change of subscription state of subscribers
    for (auto subscriberPortData : m_portPool->getSubscriberPortDataList())
    {
        SubscriberPortType subscriberPort(subscriberPortData);

        doDiscoveryForSubscriberPort(subscriberPort);

        // check if we have to destroy this subscriber port
        if (subscriberPort.toBeDestroyed())
        {
            destroySubscriberPort(subscriberPortData);
        }
    }
}

void PortManager::doDiscoveryForSubscriberPort(SubscriberPortType& subscriberPort) noexcept
{
    subscriberPort.tryGetCaProMessage().and_then([this, &subscriberPort](auto caproMessage) {
        if ((capro::CaproMessageType::SUB == caproMessage.m_type)
            || (capro::CaproMessageType::UNSUB == caproMessage.m_type))
        {
            m_portIntrospection.reportMessage(caproMessage, subscriberPort.getUniqueID());
            if (!this->sendToAllMatchingPublisherPorts(caproMessage, subscriberPort))
            {
                LogDebug() << "capro::SUB/UNSUB, no matching publisher!!";
                capro::CaproMessage nackMessage(capro::CaproMessageType::NACK,
                                                subscriberPort.getCaProServiceDescription());
                auto returnMessage = subscriberPort.dispatchCaProMessageAndGetPossibleResponse(nackMessage);
                // No response on NACK messages
                cxx::Ensures(!returnMessage.has_value());
            }
        }
        else
        {
            // protocol error
            errorHandler(Error::kPORT_MANAGER__HANDLE_SUBSCRIBER_PORTS_INVALID_CAPRO_MESSAGE,
                         nullptr,
                         iox::ErrorLevel::MODERATE);
        }
    });
}


void PortManager::handleInterfaces() noexcept
{
    // check if there are new interfaces that must get an initial offer information
    cxx::vector<popo::InterfacePortData*, MAX_INTERFACE_NUMBER> interfacePortsForInitialForwarding;


    for (auto interfacePortData : m_portPool->getInterfacePortDataList())
    {
        if (interfacePortData->m_doInitialOfferForward)
        {
            interfacePortsForInitialForwarding.push_back(interfacePortData);
            interfacePortData->m_doInitialOfferForward = false;
        }

        // check if we have to destroy this interface port
        if (interfacePortData->m_toBeDestroyed.load(std::memory_order_relaxed))
        {
            m_portPool->removeInterfacePort(interfacePortData);
            LogDebug() << "Destroyed InterfacePortData";
        }
    }

    if (interfacePortsForInitialForwarding.size() > 0)
    {
        // provide offer information from all active publisher ports to all new interfaces
        capro::CaproMessage caproMessage;
        caproMessage.m_type = capro::CaproMessageType::OFFER;
        for (auto publisherPortData : m_portPool->getPublisherPortDataList())
        {
            PublisherPortUserType publisherPort(publisherPortData);
            if (publisherPort.isOffered())
            {
                caproMessage.m_serviceDescription = publisherPort.getCaProServiceDescription();
                for (auto& interfacePortData : interfacePortsForInitialForwarding)
                {
                    auto interfacePort = popo::InterfacePort(interfacePortData);
                    // do not offer on same interface
                    if (publisherPort.getCaProServiceDescription().getSourceInterface()
                        != interfacePort.getCaProServiceDescription().getSourceInterface())
                    {
                        interfacePort.dispatchCaProMessage(caproMessage);
                    }
                }
            }
        }
        // also forward services from service registry
        auto serviceMap = m_serviceRegistry.getServiceMap();

        caproMessage.m_subType = capro::CaproMessageSubType::SERVICE;

        for (auto const& x : serviceMap)
        {
            for (auto& instance : x.second.instanceSet)
            {
                caproMessage.m_serviceDescription = capro::ServiceDescription(x.first, instance, capro::AnyEventString);

                for (auto& interfacePortData : interfacePortsForInitialForwarding)
                {
                    auto interfacePort = popo::InterfacePort(interfacePortData);
                    interfacePort.dispatchCaProMessage(caproMessage);
                }
            }
        }
    }
}

void PortManager::handleApplications() noexcept
{
    capro::CaproMessage caproMessage;

    for (auto applicationPortData : m_portPool->getApplicationPortDataList())
    {
        iox::popo::ApplicationPort applicationPort(applicationPortData);

        while (auto maybeCaproMessage = applicationPort.tryGetCaProMessage())
        {
            auto& caproMessage = maybeCaproMessage.value();
            switch (caproMessage.m_type)
            {
            case capro::CaproMessageType::OFFER:
            {
                auto serviceDescription = caproMessage.m_serviceDescription;
                addEntryToServiceRegistry(serviceDescription.getServiceIDString(),
                                          serviceDescription.getInstanceIDString());
                break;
            }
            case capro::CaproMessageType::STOP_OFFER:
            {
                auto serviceDescription = caproMessage.m_serviceDescription;
                removeEntryFromServiceRegistry(serviceDescription.getServiceIDString(),
                                               serviceDescription.getInstanceIDString());

                break;
            }
            default:
            {
                LogError() << "Roudi: Something went wrong in receiving CaproMessage in ApplicationPortList!";
            }
            }

            // forward to interfaces
            sendToAllMatchingInterfacePorts(caproMessage);
        }

        // check if we have to destroy this application port
        if (applicationPort.toBeDestroyed())
        {
            m_portPool->removeApplicationPort(applicationPortData);
            LogDebug() << "Destroyed ApplicationPortData";
        }
    }
}

void PortManager::handleNodes() noexcept
{
    /// @todo we have to update the introspection but node information is in process introspection which is not
    // accessible here. So currently nodes will be removed not before a process is removed
    // m_processIntrospection->removeNode(RuntimeName_t(process.c_str()),
    // NodeName_t(node.c_str()));

    for (auto nodeData : m_portPool->getNodeDataList())
    {
        if (nodeData->m_toBeDestroyed.load(std::memory_order_relaxed))
        {
            m_portPool->removeNodeData(nodeData);
            LogDebug() << "Destroyed NodeData";
        }
    }
}

void PortManager::handleConditionVariables() noexcept
{
    for (auto conditionVariableData : m_portPool->getConditionVariableDataList())
    {
        if (conditionVariableData->m_toBeDestroyed.load(std::memory_order_relaxed))
        {
            m_portPool->removeConditionVariableData(conditionVariableData);
            LogDebug() << "Destroyed ConditionVariableData";
        }
    }
}

bool PortManager::sendToAllMatchingPublisherPorts(const capro::CaproMessage& message,
                                                  SubscriberPortType& subscriberSource) noexcept
{
    bool publisherFound = false;
    for (auto publisherPortData : m_portPool->getPublisherPortDataList())
    {
        PublisherPortRouDiType publisherPort(publisherPortData);
        if (subscriberSource.getCaProServiceDescription() == publisherPort.getCaProServiceDescription()
            && !(publisherPort.getSubscriberTooSlowPolicy() == popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA
                 && subscriberSource.getQueueFullPolicy() == popo::QueueFullPolicy::BLOCK_PUBLISHER))
        {
            auto publisherResponse = publisherPort.dispatchCaProMessageAndGetPossibleResponse(message);
            if (publisherResponse.has_value())
            {
                // send response to subscriber port
                auto returnMessage =
                    subscriberSource.dispatchCaProMessageAndGetPossibleResponse(publisherResponse.value());

                // ACK or NACK are sent back to the subscriber port, no further response from this one expected
                cxx::Ensures(!returnMessage.has_value());

                // inform introspection
                m_portIntrospection.reportMessage(publisherResponse.value(), subscriberSource.getUniqueID());
            }
            publisherFound = true;
        }
    }
    return publisherFound;
}

void PortManager::sendToAllMatchingSubscriberPorts(const capro::CaproMessage& message,
                                                   PublisherPortRouDiType& publisherSource) noexcept
{
    for (auto subscriberPortData : m_portPool->getSubscriberPortDataList())
    {
        SubscriberPortType subscriberPort(subscriberPortData);
        if (subscriberPort.getCaProServiceDescription() == publisherSource.getCaProServiceDescription()
            && !(publisherSource.getSubscriberTooSlowPolicy() == popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA
                 && subscriberPort.getQueueFullPolicy() == popo::QueueFullPolicy::BLOCK_PUBLISHER))
        {
            auto subscriberResponse = subscriberPort.dispatchCaProMessageAndGetPossibleResponse(message);

            // if the subscribers react on the change, process it immediately on publisher side
            if (subscriberResponse.has_value())
            {
                // we only expect reaction on OFFER
                cxx::Expects(capro::CaproMessageType::OFFER == message.m_type);

                // inform introspection
                m_portIntrospection.reportMessage(subscriberResponse.value());

                auto publisherResponse =
                    publisherSource.dispatchCaProMessageAndGetPossibleResponse(subscriberResponse.value());
                if (publisherResponse.has_value())
                {
                    // sende responsee to subscriber port
                    auto returnMessage =
                        subscriberPort.dispatchCaProMessageAndGetPossibleResponse(publisherResponse.value());

                    // ACK or NACK are sent back to the subscriber port, no further response from this one expected
                    cxx::Ensures(!returnMessage.has_value());

                    // inform introspection
                    m_portIntrospection.reportMessage(publisherResponse.value());
                }
            }
        }
    }
}

void PortManager::sendToAllMatchingInterfacePorts(const capro::CaproMessage& message) noexcept
{
    for (auto interfacePortData : m_portPool->getInterfacePortDataList())
    {
        iox::popo::InterfacePort interfacePort(interfacePortData);
        // not to the interface the port is located
        if (message.m_serviceDescription.getSourceInterface()
            != interfacePort.getCaProServiceDescription().getSourceInterface())
        {
            interfacePort.dispatchCaProMessage(message);
        }
    }
}

void PortManager::unblockProcessShutdown(const RuntimeName_t& runtimeName) noexcept
{
    for (auto port : m_portPool->getPublisherPortDataList())
    {
        PublisherPortRouDiType publisherPort(port);
        if (runtimeName == publisherPort.getRuntimeName())
        {
            port->m_offeringRequested.store(false, std::memory_order_relaxed);
            doDiscoveryForPublisherPort(publisherPort);
        }
    }
}

void PortManager::unblockRouDiShutdown() noexcept
{
    makeAllPublisherPortsToStopOffer();
}

void PortManager::makeAllPublisherPortsToStopOffer() noexcept
{
    for (auto port : m_portPool->getPublisherPortDataList())
    {
        port->m_offeringRequested.store(false, std::memory_order_relaxed);

        PublisherPortRouDiType publisherPort(port);
        doDiscoveryForPublisherPort(publisherPort);
    }
}

void PortManager::deletePortsOfProcess(const RuntimeName_t& runtimeName) noexcept
{
    for (auto port : m_portPool->getPublisherPortDataList())
    {
        PublisherPortRouDiType sender(port);
        if (runtimeName == sender.getRuntimeName())
        {
            destroyPublisherPort(port);
        }
    }

    for (auto port : m_portPool->getSubscriberPortDataList())
    {
        SubscriberPortUserType subscriber(port);
        if (runtimeName == subscriber.getRuntimeName())
        {
            destroySubscriberPort(port);
        }
    }

    for (auto port : m_portPool->getInterfacePortDataList())
    {
        popo::InterfacePort interface(port);
        if (runtimeName == interface.getRuntimeName())
        {
            m_portPool->removeInterfacePort(port);
            LogDebug() << "Deleted Interface of application " << runtimeName;
        }
    }

    for (auto port : m_portPool->getApplicationPortDataList())
    {
        popo::ApplicationPort application(port);
        if (runtimeName == application.getRuntimeName())
        {
            m_portPool->removeApplicationPort(port);
            LogDebug() << "Deleted ApplicationPort of application " << runtimeName;
        }
    }

    for (auto nodeData : m_portPool->getNodeDataList())
    {
        if (runtimeName == nodeData->m_runtimeName)
        {
            m_portPool->removeNodeData(nodeData);
            LogDebug() << "Deleted node of application " << runtimeName;
        }
    }

    for (auto conditionVariableData : m_portPool->getConditionVariableDataList())
    {
        if (runtimeName == conditionVariableData->m_runtimeName)
        {
            m_portPool->removeConditionVariableData(conditionVariableData);
            LogDebug() << "Deleted condition variable of application" << runtimeName;
        }
    }
}

void PortManager::destroyPublisherPort(PublisherPortRouDiType::MemberType_t* const publisherPortData) noexcept
{
    // create temporary publisher ports to orderly shut this publisher down
    PublisherPortRouDiType publisherPortRoudi{publisherPortData};
    PublisherPortUserType publisherPortUser{publisherPortData};

    publisherPortRoudi.releaseAllChunks();
    publisherPortUser.stopOffer();

    // process STOP_OFFER for this publisher in RouDi and distribute it
    publisherPortRoudi.tryGetCaProMessage().and_then([this, &publisherPortRoudi](auto caproMessage) {
        cxx::Ensures(caproMessage.m_type == capro::CaproMessageType::STOP_OFFER);

        m_portIntrospection.reportMessage(caproMessage);
        this->removeEntryFromServiceRegistry(caproMessage.m_serviceDescription.getServiceIDString(),
                                             caproMessage.m_serviceDescription.getInstanceIDString());
        this->sendToAllMatchingSubscriberPorts(caproMessage, publisherPortRoudi);
        this->sendToAllMatchingInterfacePorts(caproMessage);
    });

    m_portIntrospection.removePublisher(publisherPortUser);

    // delete publisher port from list after STOP_OFFER was processed
    m_portPool->removePublisherPort(publisherPortData);

    LogDebug() << "Destroyed publisher port";
}

void PortManager::destroySubscriberPort(SubscriberPortType::MemberType_t* const subscriberPortData) noexcept
{
    // create temporary subscriber ports to orderly shut this subscriber down
    SubscriberPortType subscriberPortRoudi(subscriberPortData);
    SubscriberPortUserType subscriberPortUser(subscriberPortData);

    subscriberPortRoudi.releaseAllChunks();
    subscriberPortUser.unsubscribe();

    // process UNSUB for this subscriber in RouDi and distribute it
    subscriberPortRoudi.tryGetCaProMessage().and_then([this, &subscriberPortRoudi](auto caproMessage) {
        cxx::Ensures(caproMessage.m_type == capro::CaproMessageType::UNSUB);

        m_portIntrospection.reportMessage(caproMessage);
        this->sendToAllMatchingPublisherPorts(caproMessage, subscriberPortRoudi);
    });

    m_portIntrospection.removeSubscriber(subscriberPortUser);
    // delete subscriber port from list after UNSUB was processed
    m_portPool->removeSubscriberPort(subscriberPortData);

    LogDebug() << "Destroyed subscriber port";
}

runtime::IpcMessage PortManager::findService(const capro::ServiceDescription& service) noexcept
{
    // send find to all interfaces
    capro::CaproMessage caproMessage(capro::CaproMessageType::FIND, service);

    for (auto interfacePortData : m_portPool->getInterfacePortDataList())
    {
        iox::popo::InterfacePort interfacePort(interfacePortData);
        interfacePort.dispatchCaProMessage(caproMessage);
    }

    // add all found instances to instanceString
    runtime::IpcMessage instanceMessage;

    ServiceRegistry::InstanceSet_t instances;
    m_serviceRegistry.find(instances, service.getServiceIDString(), service.getInstanceIDString());
    for (auto& instance : instances)
    {
        instanceMessage << instance;
    }

    return instanceMessage;
}

const std::atomic<uint64_t>* PortManager::serviceRegistryChangeCounter() noexcept
{
    return m_portPool->serviceRegistryChangeCounter();
}

cxx::expected<PublisherPortRouDiType::MemberType_t*, PortPoolError>
PortManager::acquirePublisherPortData(const capro::ServiceDescription& service,
                                      const popo::PublisherOptions& publisherOptions,
                                      const RuntimeName_t& runtimeName,
                                      mepoo::MemoryManager* const payloadDataSegmentMemoryManager,
                                      const PortConfigInfo& portConfigInfo) noexcept
{
    if (doesViolateCommunicationPolicy<iox::build::CommunicationPolicy>(service).and_then(
            [&](const auto& usedByProcess) {
                LogWarn()
                    << "Process '" << runtimeName
                    << "' violates the communication policy by requesting a PublisherPort which is already used by '"
                    << usedByProcess << "' with service '" << service.operator cxx::Serialization().toString() << "'.";
            }))
    {
        errorHandler(Error::kPOSH__PORT_MANAGER_PUBLISHERPORT_NOT_UNIQUE, nullptr, ErrorLevel::MODERATE);
        return cxx::error<PortPoolError>(PortPoolError::UNIQUE_PUBLISHER_PORT_ALREADY_EXISTS);
    }

    // we can create a new port
    auto maybePublisherPortData = m_portPool->addPublisherPort(
        service, payloadDataSegmentMemoryManager, runtimeName, publisherOptions, portConfigInfo.memoryInfo);
    if (!maybePublisherPortData.has_error())
    {
        auto publisherPortData = maybePublisherPortData.value();
        if (publisherPortData)
        {
            m_portIntrospection.addPublisher(*publisherPortData);

            // we do discovery here for trying to connect the waiting subscribers if offer on create is desired
            PublisherPortRouDiType publisherPort(publisherPortData);
            doDiscoveryForPublisherPort(publisherPort);
        }
    }

    return maybePublisherPortData;
}

cxx::expected<SubscriberPortType::MemberType_t*, PortPoolError>
PortManager::acquireSubscriberPortData(const capro::ServiceDescription& service,
                                       const popo::SubscriberOptions& subscriberOptions,
                                       const RuntimeName_t& runtimeName,
                                       const PortConfigInfo& portConfigInfo) noexcept
{
    auto maybeSubscriberPortData =
        m_portPool->addSubscriberPort(service, runtimeName, subscriberOptions, portConfigInfo.memoryInfo);
    if (!maybeSubscriberPortData.has_error())
    {
        auto subscriberPortData = maybeSubscriberPortData.value();
        if (subscriberPortData)
        {
            m_portIntrospection.addSubscriber(*subscriberPortData);

            // we do discovery here for trying to connect with publishers if subscribe on create is desired
            SubscriberPortType subscriberPort(subscriberPortData);
            doDiscoveryForSubscriberPort(subscriberPort);
        }
    }

    return maybeSubscriberPortData;
}


/// @todo return a cxx::expected
popo::InterfacePortData* PortManager::acquireInterfacePortData(capro::Interfaces interface,
                                                               const RuntimeName_t& runtimeName,
                                                               const NodeName_t& /*node*/) noexcept
{
    auto result = m_portPool->addInterfacePort(runtimeName, interface);
    if (!result.has_error())
    {
        return result.value();
    }
    else
    {
        return nullptr;
    }
}

/// @todo return a cxx::expected
popo::ApplicationPortData* PortManager::acquireApplicationPortData(const RuntimeName_t& runtimeName) noexcept
{
    auto result = m_portPool->addApplicationPort(runtimeName);
    if (!result.has_error())
    {
        return result.value();
    }
    else
    {
        return nullptr;
    }
}

void PortManager::addEntryToServiceRegistry(const capro::IdString_t& service,
                                            const capro::IdString_t& instance) noexcept
{
    m_serviceRegistry.add(service, instance);
    m_portPool->serviceRegistryChangeCounter()->fetch_add(1, std::memory_order_relaxed);
}

void PortManager::removeEntryFromServiceRegistry(const capro::IdString_t& service,
                                                 const capro::IdString_t& instance) noexcept
{
    m_serviceRegistry.remove(service, instance);
    m_portPool->serviceRegistryChangeCounter()->fetch_add(1, std::memory_order_relaxed);
}

cxx::expected<runtime::NodeData*, PortPoolError> PortManager::acquireNodeData(const RuntimeName_t& runtimeName,
                                                                              const NodeName_t& nodeName) noexcept
{
    return m_portPool->addNodeData(runtimeName, nodeName, 0);
}

cxx::expected<popo::ConditionVariableData*, PortPoolError>
PortManager::acquireConditionVariableData(const RuntimeName_t& runtimeName) noexcept
{
    return m_portPool->addConditionVariableData(runtimeName);
}

} // namespace roudi
} // namespace iox
