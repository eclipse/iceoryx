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
#ifndef IOX_POSH_ROUDI_INTROSPECTION_PORT_INTROSPECTION_INL
#define IOX_POSH_ROUDI_INTROSPECTION_PORT_INTROSPECTION_INL

#include "iceoryx_utils/posix_wrapper/thread.hpp"

namespace iox
{
namespace roudi
{
template <typename PublisherPort, typename SubscriberPort>
inline PortIntrospection<PublisherPort, SubscriberPort>::PortIntrospection() noexcept
{
}

template <typename PublisherPort, typename SubscriberPort>
inline PortIntrospection<PublisherPort, SubscriberPort>::PortData::PortData() noexcept
    : m_newData(true)
{
}

template <typename PublisherPort, typename SubscriberPort>
inline PortIntrospection<PublisherPort, SubscriberPort>::~PortIntrospection() noexcept
{
    stop();
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::reportMessage(const capro::CaproMessage& message) noexcept
{
    m_portData.updateConnectionState(message);
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::reportMessage(const capro::CaproMessage& message,
                                                                            const UniquePortId& id)
{
    m_portData.updateSubscriberConnectionState(message, id);
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::registerPublisherPort(
    PublisherPort&& publisherPortGeneric,
    PublisherPort&& publisherPortThroughput,
    PublisherPort&& publisherPortSubscriberPortsData) noexcept
{
    if (m_publisherPort || m_publisherPortThroughput || m_publisherPortSubscriberPortsData)
    {
        return false;
    }

    m_publisherPort.emplace(std::move(publisherPortGeneric));
    m_publisherPortThroughput.emplace(std::move(publisherPortThroughput));
    m_publisherPortSubscriberPortsData.emplace(std::move(publisherPortSubscriberPortsData));

    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::run() noexcept
{
    cxx::Expects(m_publisherPort.has_value());
    cxx::Expects(m_publisherPortThroughput.has_value());
    cxx::Expects(m_publisherPortSubscriberPortsData.has_value());

    // this is a field, there needs to be a sample before activate is called
    sendPortData();
    sendThroughputData();
    sendSubscriberPortsData();
    m_publisherPort->offer();
    m_publisherPortThroughput->offer();
    m_publisherPortSubscriberPortsData->offer();

    m_publishingTask.start(m_sendInterval);
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::send() noexcept
{
    if (m_portData.isNew())
    {
        sendPortData();
    }
    sendThroughputData();
    sendSubscriberPortsData();
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::sendPortData() noexcept
{
    auto maybeChunkHeader = m_publisherPort->tryAllocateChunk(sizeof(PortIntrospectionFieldTopic),
                                                              alignof(PortIntrospectionFieldTopic),
                                                              CHUNK_NO_USER_HEADER_SIZE,
                                                              CHUNK_NO_USER_HEADER_ALIGNMENT);
    if (!maybeChunkHeader.has_error())
    {
        auto sample = static_cast<PortIntrospectionFieldTopic*>(maybeChunkHeader.value()->userPayload());
        new (sample) PortIntrospectionFieldTopic();

        m_portData.prepareTopic(*sample); // requires internal mutex (blocks
                                          // further introspection events)
        m_publisherPort->sendChunk(maybeChunkHeader.value());
    }
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::sendThroughputData() noexcept
{
    auto maybeChunkHeader = m_publisherPortThroughput->tryAllocateChunk(sizeof(PortThroughputIntrospectionFieldTopic),
                                                                        alignof(PortThroughputIntrospectionFieldTopic),
                                                                        CHUNK_NO_USER_HEADER_SIZE,
                                                                        CHUNK_NO_USER_HEADER_ALIGNMENT);
    if (!maybeChunkHeader.has_error())
    {
        auto throughputSample =
            static_cast<PortThroughputIntrospectionFieldTopic*>(maybeChunkHeader.value()->userPayload());
        new (throughputSample) PortThroughputIntrospectionFieldTopic();

        m_portData.prepareTopic(*throughputSample); // requires internal mutex (blocks
        // further introspection events)
        m_publisherPortThroughput->sendChunk(maybeChunkHeader.value());
    }
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::sendSubscriberPortsData() noexcept
{
    auto maybeChunkHeader =
        m_publisherPortSubscriberPortsData->tryAllocateChunk(sizeof(SubscriberPortChangingIntrospectionFieldTopic),
                                                             alignof(SubscriberPortChangingIntrospectionFieldTopic),
                                                             CHUNK_NO_USER_HEADER_SIZE,
                                                             CHUNK_NO_USER_HEADER_ALIGNMENT);
    if (!maybeChunkHeader.has_error())
    {
        auto subscriberPortChangingDataSample =
            static_cast<SubscriberPortChangingIntrospectionFieldTopic*>(maybeChunkHeader.value()->userPayload());
        new (subscriberPortChangingDataSample) SubscriberPortChangingIntrospectionFieldTopic();

        m_portData.prepareTopic(*subscriberPortChangingDataSample); // requires internal mutex (blocks
        // further introspection events)
        m_publisherPortSubscriberPortsData->sendChunk(maybeChunkHeader.value());
    }
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::setSendInterval(const units::Duration interval) noexcept
{
    m_sendInterval = interval;
    if (m_publishingTask.isActive())
    {
        m_publishingTask.stop();
        m_publishingTask.start(m_sendInterval);
    }
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::stop() noexcept
{
    m_publishingTask.stop();
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::updateConnectionState(
    const capro::CaproMessage& message) noexcept
{
    const capro::ServiceDescription& service = message.m_serviceDescription;
    capro::CaproMessageType messageType = message.m_type;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_connectionMap.find(service);
    if (iter == m_connectionMap.end())
    {
        return false; // no corresponding capro Id ...
    }

    auto& innerConnectionMap = iter->second;

    for (auto& pair : innerConnectionMap)
    {
        auto& connection = m_connectionContainer[pair.second];
        connection.state = getNextState<iox::build::CommunicationPolicy>(connection.state, messageType);
    }

    setNew(true);
    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::updateSubscriberConnectionState(
    const capro::CaproMessage& message, const UniquePortId& id)
{
    const capro::ServiceDescription& service = message.m_serviceDescription;
    capro::CaproMessageType messageType = message.m_type;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_connectionMap.find(service);
    if (iter == m_connectionMap.end())
    {
        return false; // no corresponding capro Id ...
    }

    auto& innerConnectionMap = iter->second;

    auto iterInnerMap = innerConnectionMap.find(id);
    if (iterInnerMap == innerConnectionMap.end())
    {
        return false;
    }

    auto& connection = m_connectionContainer[iterInnerMap->second];
    connection.state = getNextState<iox::build::CommunicationPolicy>(connection.state, messageType);

    setNew(true);
    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline bool
PortIntrospection<PublisherPort, SubscriberPort>::PortData::addPublisher(typename PublisherPort::MemberType_t& port)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto service = port.m_serviceDescription;
    auto uniqueId = port.m_uniqueId;

    auto index = m_publisherContainer.add(PublisherInfo(port));
    if (index < 0)
    {
        return false;
    }

    auto iter = m_publisherMap.find(service);
    if (iter == m_publisherMap.end())
    {
        // service is new, create new map
        std::map<UniquePortId, typename PublisherContainer::Index_t> innerPublisherMap;
        innerPublisherMap.insert(std::make_pair(uniqueId, index));
        m_publisherMap.insert(std::make_pair(service, innerPublisherMap));
    }
    else
    {
        // service exists, add new entry
        auto& innerPublisherMap = iter->second;
        auto iter = innerPublisherMap.find(uniqueId);
        if (iter == innerPublisherMap.end())
        {
            innerPublisherMap.insert(std::make_pair(uniqueId, index));
        }
        else
        {
            return false;
        }
    }

    // connect publisher to all subscribers with the same Id
    PublisherInfo* publisher = m_publisherContainer.get(index);

    // find corresponding subscribers
    auto connIter = m_connectionMap.find(service);
    if (connIter != m_connectionMap.end())
    {
        auto& innerConnectionMap = connIter->second;
        for (auto& pair : innerConnectionMap)
        {
            auto& connection = m_connectionContainer[pair.second];
            if (service == connection.subscriberInfo.service)
            {
                connection.publisherInfo = publisher;
            }
        }
    }

    setNew(true);
    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::addSubscriber(
    typename SubscriberPort::MemberType_t& portData)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto service = portData.m_serviceDescription;
    auto uniqueId = portData.m_uniqueId;

    auto index = m_connectionContainer.add(ConnectionInfo(portData));
    if (index < 0)
    {
        return false;
    }

    auto iter = m_connectionMap.find(service);

    if (iter == m_connectionMap.end())
    {
        // service is new, create new map
        std::map<UniquePortId, typename ConnectionContainer::Index_t> innerConnectionMap;
        innerConnectionMap.insert(std::make_pair(uniqueId, index));
        m_connectionMap.insert(std::make_pair(service, innerConnectionMap));
    }
    else
    {
        // service exists, add new entry
        auto& innerConnectionMap = iter->second;
        auto iter = innerConnectionMap.find(uniqueId);
        if (iter == innerConnectionMap.end())
        {
            innerConnectionMap.insert(std::make_pair(uniqueId, index));
        }
        else
        {
            return false;
        }
    }

    auto& connection = m_connectionContainer[index];

    auto sendIter = m_publisherMap.find(service);
    if (sendIter != m_publisherMap.end())
    {
        auto& innerPublisherMap = sendIter->second;
        for (auto& iter : innerPublisherMap)
        {
            auto publisher = m_publisherContainer.get(iter.second);
            connection.publisherInfo = publisher; // set corresponding publisher if exists
        }
    }

    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::removePublisher(const PublisherPort& port)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_publisherMap.find(port.getCaProServiceDescription());
    if (iter == m_publisherMap.end())
    {
        return false;
    }

    auto& innerPublisherMap = iter->second;
    auto iterInnerMap = innerPublisherMap.find(port.getUniqueID());
    if (iterInnerMap == innerPublisherMap.end())
    {
        return false;
    }
    auto m_publisherIndex = iterInnerMap->second;
    auto& publisher = m_publisherContainer[m_publisherIndex];

    // disconnect publisher from all its subscribers
    for (auto& pair : publisher.connectionMap)
    {
        pair.second->publisherInfo = nullptr;          // publisher is disconnected
        pair.second->state = ConnectionState::DEFAULT; // connection state is now default
    }

    innerPublisherMap.erase(iterInnerMap);
    m_publisherContainer.remove(m_publisherIndex);
    setNew(true); // indicates we have to send new data because
                  // something changed

    return true;
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::removeSubscriber(const SubscriberPort& port)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_connectionMap.find(port.getCaProServiceDescription());
    if (iter == m_connectionMap.end())
    {
        return false; // not found and therefore not removed
    }

    auto& innerConnectionMap = iter->second;
    auto mapIter = innerConnectionMap.find(port.getUniqueID());

    if (mapIter == innerConnectionMap.end())
    {
        return false; // not found and therefore not removed
    }

    // remove subscriber in corresponding publisher
    auto connectionIndex = mapIter->second;
    auto& connection = m_connectionContainer[connectionIndex];
    auto& publisher = connection.publisherInfo;

    if (publisher)
    {
        auto connIter = publisher->connectionMap.find(connectionIndex);
        if (connIter != publisher->connectionMap.end())
        {
            publisher->connectionMap.erase(connIter);
        }
    }

    innerConnectionMap.erase(mapIter);
    m_connectionContainer.remove(connectionIndex);

    setNew(true);
    return true;
}

template <typename PublisherPort, typename SubscriberPort>
template <typename T, std::enable_if_t<std::is_same<T, iox::build::OneToManyPolicy>::value>*>
inline typename PortIntrospection<PublisherPort, SubscriberPort>::ConnectionState
PortIntrospection<PublisherPort, SubscriberPort>::PortData::getNextState(ConnectionState currentState,
                                                                         capro::CaproMessageType messageType) noexcept
{
    ConnectionState nextState = currentState; // stay in currentState as default transition

    // publisher and subscriber can only send a subset of messages (e.g. no
    // sub request from publisher), so it is not required to check whether
    // publisher or subscriber has sent the message...

    switch (currentState)
    {
    case ConnectionState::DEFAULT:
    {
        if (messageType == capro::CaproMessageType::SUB)
        {
            nextState = ConnectionState::SUB_REQUESTED;
        }
        break;
    }

    case ConnectionState::SUB_REQUESTED:
    {
        if (messageType == capro::CaproMessageType::ACK)
        {
            nextState = ConnectionState::CONNECTED;
        }
        else if (messageType == capro::CaproMessageType::NACK)
        {
            nextState = ConnectionState::DEFAULT;
        }
        break;
    }

    case ConnectionState::CONNECTED:
    {
        if (messageType == capro::CaproMessageType::STOP_OFFER)
        {
            nextState = ConnectionState::DEFAULT;
        }
        else if (messageType == capro::CaproMessageType::UNSUB)
        {
            nextState = ConnectionState::DEFAULT;
        }
        break;
    }

    default:
    { // stay in current state
    }
    };

    return nextState;
}

template <typename PublisherPort, typename SubscriberPort>
template <typename T, std::enable_if_t<std::is_same<T, iox::build::ManyToManyPolicy>::value>*>
inline typename PortIntrospection<PublisherPort, SubscriberPort>::ConnectionState
PortIntrospection<PublisherPort, SubscriberPort>::PortData::getNextState(ConnectionState currentState,
                                                                         capro::CaproMessageType messageType) noexcept
{
    ConnectionState nextState = currentState; // stay in currentState as default transition

    switch (currentState)
    {
    case ConnectionState::DEFAULT:
    {
        if (messageType == capro::CaproMessageType::SUB)
        {
            nextState = ConnectionState::CONNECTED;
        }
        break;
    }

    case ConnectionState::CONNECTED:
    {
        if (messageType == capro::CaproMessageType::UNSUB)
        {
            nextState = ConnectionState::DEFAULT;
        }
        break;
    }

    default:
    { // stay in current state
    }
    };

    return nextState;
}

template <typename PublisherPort, typename SubscriberPort>
inline void
PortIntrospection<PublisherPort, SubscriberPort>::PortData::prepareTopic(PortIntrospectionTopic& topic) noexcept
{
    auto& m_publisherList = topic.m_publisherList;

    std::lock_guard<std::mutex> lock(m_mutex); // we need to lock the internal data structs

    int32_t index{0};
    for (auto& pub : m_publisherMap)
    {
        auto& innerPublisherMap = pub.second;
        for (auto& pair : innerPublisherMap)
        {
            auto m_publisherIndex = pair.second;
            if (m_publisherIndex >= 0)
            {
                auto& publisherInfo = m_publisherContainer[m_publisherIndex];
                PublisherPortData publisherData;
                PublisherPort port(publisherInfo.portData);
                publisherData.m_publisherPortID = static_cast<uint64_t>(port.getUniqueID());
                publisherData.m_sourceInterface = publisherInfo.service.getSourceInterface();
                publisherData.m_name = publisherInfo.process;
                publisherData.m_node = publisherInfo.node;

                publisherData.m_caproInstanceID = publisherInfo.service.getInstanceIDString();
                publisherData.m_caproServiceID = publisherInfo.service.getServiceIDString();
                publisherData.m_caproEventMethodID = publisherInfo.service.getEventIDString();

                m_publisherList.emplace_back(publisherData);
                publisherInfo.index = index++;
            }
        }
    }

    auto& m_subscriberList = topic.m_subscriberList;
    for (auto& connPair : m_connectionMap)
    {
        for (auto& pair : connPair.second)
        {
            auto connectionIndex = pair.second;
            if (connectionIndex >= 0)
            {
                auto& connection = m_connectionContainer[connectionIndex];
                SubscriberPortData subscriberData;
                auto& subscriberInfo = connection.subscriberInfo;

                subscriberData.m_name = subscriberInfo.process;
                subscriberData.m_node = subscriberInfo.node;

                subscriberData.m_caproInstanceID = subscriberInfo.service.getInstanceIDString();
                subscriberData.m_caproServiceID = subscriberInfo.service.getServiceIDString();
                subscriberData.m_caproEventMethodID = subscriberInfo.service.getEventIDString();
                m_subscriberList.emplace_back(subscriberData);
            }
        }
    }

    // needs to be done while holding the lock
    setNew(false);
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::PortData::prepareTopic(
    PortThroughputIntrospectionTopic& topic IOX_MAYBE_UNUSED) noexcept
{
    /// @todo #402 re-add port throughput
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::PortData::prepareTopic(
    SubscriberPortChangingIntrospectionFieldTopic& topic) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& connPair : m_connectionMap)
    {
        for (auto& pair : connPair.second)
        {
            auto connectionIndex = pair.second;
            if (connectionIndex >= 0)
            {
                auto& connection = m_connectionContainer[connectionIndex];
                auto& subscriberInfo = connection.subscriberInfo;
                SubscriberPortChangingData subscriberData;
                if (subscriberInfo.portData != nullptr)
                {
                    SubscriberPort port(subscriberInfo.portData);
                    subscriberData.subscriptionState = port.getSubscriptionState();

                    // subscriberData.fifoCapacity = port .getDeliveryFiFoCapacity();
                    // subscriberData.fifoSize = port.getDeliveryFiFoSize();
                    subscriberData.propagationScope = port.getCaProServiceDescription().getScope();
                }
                else
                {
                    subscriberData.fifoCapacity = 0u;
                    subscriberData.fifoSize = 0u;
                    subscriberData.subscriptionState = iox::SubscribeState::NOT_SUBSCRIBED;
                    subscriberData.propagationScope = capro::Scope::INVALID;
                }
                topic.subscriberPortChangingDataList.push_back(subscriberData);
            }
        }
    }
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::PortData::isNew() const noexcept
{
    return m_newData.load(std::memory_order_acquire);
}

template <typename PublisherPort, typename SubscriberPort>
inline void PortIntrospection<PublisherPort, SubscriberPort>::PortData::setNew(bool value) noexcept
{
    m_newData.store(value, std::memory_order_release);
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::addPublisher(typename PublisherPort::MemberType_t& port)
{
    return m_portData.addPublisher(port);
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::addSubscriber(typename SubscriberPort::MemberType_t& port)
{
    return m_portData.addSubscriber(port);
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::removePublisher(const PublisherPort& port)
{
    return m_portData.removePublisher(port);
}

template <typename PublisherPort, typename SubscriberPort>
inline bool PortIntrospection<PublisherPort, SubscriberPort>::removeSubscriber(const SubscriberPort& port)
{
    return m_portData.removeSubscriber(port);
}

} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_INTROSPECTION_PORT_INTROSPECTION_INL
