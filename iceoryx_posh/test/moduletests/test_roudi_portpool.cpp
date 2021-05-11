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

#include "iceoryx_posh/internal/roudi/port_pool_data.hpp"
#include "iceoryx_posh/internal/runtime/node_data.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_posh/roudi/port_pool.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::Return;
using namespace iox::capro;

namespace iox
{
namespace test
{
static constexpr uint32_t DEFAULT_DEVICE_ID{20U};
static constexpr uint32_t DEFAULT_MEMORY_TYPE{100U};
class PortPool_test : public Test
{
  public:
    roudi::PortPoolData m_portPoolData;
    roudi::PortPool sut{m_portPoolData};

    ServiceDescription m_serviceDescription{"service1", "instance1"};
    RuntimeName_t m_applicationName{"AppName"};
    RuntimeName_t m_runtimeName{"runtimeName"};
    NodeName_t m_nodeName{"nodeName"};
    const uint64_t m_nodeDeviceId = 999U;
    mepoo::MemoryManager m_memoryManager;
    popo::PublisherOptions m_publisherOptions{10U, m_nodeName};
    popo::SubscriberOptions m_subscriberOptions{
        iox::popo::SubscriberPortData::ChunkQueueData_t::MAX_CAPACITY, 10U, m_nodeName};
    iox::mepoo::MemoryInfo m_memoryInfo{DEFAULT_DEVICE_ID, DEFAULT_MEMORY_TYPE};
};

TEST_F(PortPool_test, AddNodeDataIsSuccessful)
{
    auto nodeData = sut.addNodeData(m_runtimeName, m_nodeName, m_nodeDeviceId);

    ASSERT_THAT(nodeData.has_error(), Eq(false));
    EXPECT_EQ(nodeData.value()->m_runtimeName, m_runtimeName);
    EXPECT_EQ(nodeData.value()->m_nodeName, m_nodeName);
    EXPECT_EQ(nodeData.value()->m_nodeDeviceIdentifier, m_nodeDeviceId);
}

TEST_F(PortPool_test, AddNodeDataWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 1U; i <= MAX_NODE_NUMBER; ++i)
    {
        auto nodeData = sut.addNodeData(m_runtimeName, m_nodeName, i);

        ASSERT_THAT(nodeData.has_error(), Eq(false));
    }

    EXPECT_EQ(sut.getNodeDataList().size(), MAX_NODE_NUMBER);
}


TEST_F(PortPool_test, AddNodeDataWhenNodeListIsFullReturnsError)
{
    for (uint32_t i = 0U; i < MAX_NODE_NUMBER; ++i)
    {
        ASSERT_FALSE(sut.addNodeData(m_runtimeName, m_nodeName, i).has_error());
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });

    ASSERT_TRUE(sut.addNodeData(m_runtimeName, m_nodeName, MAX_NODE_NUMBER).has_error());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__NODELIST_OVERFLOW);
}

TEST_F(PortPool_test, GetNodeDataListIsSuccessful)
{
    ASSERT_FALSE(sut.addNodeData(m_runtimeName, m_nodeName, m_nodeDeviceId).has_error());

    auto nodeDataList = sut.getNodeDataList();

    EXPECT_EQ(nodeDataList.size(), 1U);
    EXPECT_EQ(nodeDataList[0]->m_runtimeName, m_runtimeName);
    EXPECT_EQ(nodeDataList[0]->m_nodeName, m_nodeName);
    EXPECT_EQ(nodeDataList[0]->m_nodeDeviceIdentifier, m_nodeDeviceId);
}

TEST_F(PortPool_test, GetNodeDataListWhenEmptyIsSuccessful)
{
    auto nodeDataList = sut.getNodeDataList();

    EXPECT_EQ(nodeDataList.size(), 0U);
}

TEST_F(PortPool_test, GetNodeDataListWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 1U; i <= MAX_NODE_NUMBER; ++i)
    {
        auto nodeData = sut.addNodeData(m_runtimeName, m_nodeName, i);

        ASSERT_THAT(nodeData.has_error(), Eq(false));
    }

    auto nodeDataList = sut.getNodeDataList();

    EXPECT_EQ(nodeDataList.size(), MAX_NODE_NUMBER);
}

TEST_F(PortPool_test, RemoveNodeDataIsSuccessful)
{
    auto nodeData = sut.addNodeData(m_runtimeName, m_nodeName, m_nodeDeviceId);

    sut.removeNodeData(nodeData.value());
    auto nodeDataList = sut.getNodeDataList();

    EXPECT_EQ(nodeDataList.size(), 0U);
}

TEST_F(PortPool_test, AddPublisherPortIsSuccessful)
{
    auto publisherPort = sut.addPublisherPort(
        m_serviceDescription, &m_memoryManager, m_applicationName, m_publisherOptions, m_memoryInfo);

    ASSERT_THAT(publisherPort.has_error(), Eq(false));
    EXPECT_EQ(publisherPort.value()->m_serviceDescription, m_serviceDescription);
    EXPECT_EQ(publisherPort.value()->m_runtimeName, m_applicationName);
    EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_historyCapacity, m_publisherOptions.historyCapacity);
    EXPECT_EQ(publisherPort.value()->m_nodeName, m_publisherOptions.nodeName);
    EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_memoryInfo.deviceId, DEFAULT_DEVICE_ID);
    EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_memoryInfo.memoryType, DEFAULT_MEMORY_TYPE);
}

TEST_F(PortPool_test, AddPublisherPortWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_PUBLISHERS; ++i)
    {
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};

        auto publisherPort = sut.addPublisherPort(
            m_serviceDescription, &m_memoryManager, applicationName, m_publisherOptions, m_memoryInfo);

        ASSERT_THAT(publisherPort.has_error(), Eq(false));
        EXPECT_EQ(publisherPort.value()->m_serviceDescription, m_serviceDescription);
        EXPECT_EQ(publisherPort.value()->m_runtimeName, applicationName);
        EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_historyCapacity, m_publisherOptions.historyCapacity);
        EXPECT_EQ(publisherPort.value()->m_nodeName, m_publisherOptions.nodeName);
        EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_memoryInfo.deviceId, DEFAULT_DEVICE_ID);
        EXPECT_EQ(publisherPort.value()->m_chunkSenderData.m_memoryInfo.memoryType, DEFAULT_MEMORY_TYPE);
    }
}

TEST_F(PortPool_test, AddPublisherPortWhenPublisherListOverflowsReturnsError)
{
    auto addPublisherPort = [&](const uint32_t i) -> bool {
        std::string service = "service" + std::to_string(i);
        std::string instance = "instance" + std::to_string(i);
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};

        return sut
            .addPublisherPort(
                {IdString_t(cxx::TruncateToCapacity, service), IdString_t(cxx::TruncateToCapacity, instance)},
                &m_memoryManager,
                applicationName,
                m_publisherOptions)
            .has_error();
    };

    for (uint32_t i = 0U; i < MAX_PUBLISHERS; ++i)
    {
        EXPECT_FALSE(addPublisherPort(i));
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });

    EXPECT_TRUE(addPublisherPort(MAX_PUBLISHERS));

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__PUBLISHERLIST_OVERFLOW);
}

TEST_F(PortPool_test, GetPublisherPortDataListIsSuccessful)
{
    auto publisherPortDataList = sut.getPublisherPortDataList();

    EXPECT_EQ(publisherPortDataList.size(), 0U);

    ASSERT_FALSE(sut.addPublisherPort(m_serviceDescription, &m_memoryManager, m_applicationName, m_publisherOptions)
                     .has_error());
    publisherPortDataList = sut.getPublisherPortDataList();

    EXPECT_EQ(publisherPortDataList.size(), 1U);
}

TEST_F(PortPool_test, GetPublisherPortDataListWhenEmptyIsSuccessful)
{
    auto nodeDataList = sut.getPublisherPortDataList();

    EXPECT_EQ(nodeDataList.size(), 0U);
}

TEST_F(PortPool_test, GetPublisherPortDataListCompletelyFilledSuccessfully)
{
    for (uint32_t i = 0U; i < MAX_PUBLISHERS; ++i)
    {
        std::string service = "service" + std::to_string(i);
        std::string instance = "instance" + std::to_string(i);
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};

        ASSERT_FALSE(sut.addPublisherPort({IdString_t(cxx::TruncateToCapacity, service),
                                           IdString_t(cxx::TruncateToCapacity, instance)},
                                          &m_memoryManager,
                                          applicationName,
                                          m_publisherOptions)
                         .has_error());
    }

    auto publisherPortDataList = sut.getPublisherPortDataList();

    EXPECT_EQ(publisherPortDataList.size(), MAX_PUBLISHERS);
}

TEST_F(PortPool_test, RemovePublisherPortIsSuccessful)
{
    auto publisherPort =
        sut.addPublisherPort(m_serviceDescription, &m_memoryManager, m_applicationName, m_publisherOptions);
    sut.removePublisherPort(publisherPort.value());
    auto publisherPortDataList = sut.getPublisherPortDataList();

    EXPECT_EQ(publisherPortDataList.size(), 0U);
}

TEST_F(PortPool_test, AddSubscriberPortIsSuccessful)
{
    auto subscriberPort =
        sut.addSubscriberPort(m_serviceDescription, m_applicationName, m_subscriberOptions, m_memoryInfo);

    ASSERT_THAT(subscriberPort.has_error(), Eq(false));
    EXPECT_EQ(subscriberPort.value()->m_serviceDescription, m_serviceDescription);
    EXPECT_EQ(subscriberPort.value()->m_runtimeName, m_applicationName);
    EXPECT_EQ(subscriberPort.value()->m_nodeName, m_subscriberOptions.nodeName);
    EXPECT_EQ(subscriberPort.value()->m_historyRequest, m_subscriberOptions.historyRequest);
    EXPECT_EQ(subscriberPort.value()->m_chunkReceiverData.m_queue.capacity(), 256U);
    EXPECT_EQ(subscriberPort.value()->m_chunkReceiverData.m_memoryInfo.deviceId, DEFAULT_DEVICE_ID);
    EXPECT_EQ(subscriberPort.value()->m_chunkReceiverData.m_memoryInfo.memoryType, DEFAULT_MEMORY_TYPE);
}

TEST_F(PortPool_test, AddSubscriberPortToMaxCapacityIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_SUBSCRIBERS; ++i)
    {
        std::string service = "service" + std::to_string(i);
        std::string instance = "instance" + std::to_string(i);
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};


        auto subscriberPort =
            sut.addSubscriberPort(m_serviceDescription, applicationName, m_subscriberOptions, m_memoryInfo);

        ASSERT_THAT(subscriberPort.has_error(), Eq(false));
        EXPECT_EQ(subscriberPort.value()->m_serviceDescription, m_serviceDescription);
        EXPECT_EQ(subscriberPort.value()->m_runtimeName, applicationName);
        EXPECT_EQ(subscriberPort.value()->m_nodeName, m_subscriberOptions.nodeName);
        EXPECT_EQ(subscriberPort.value()->m_chunkReceiverData.m_memoryInfo.deviceId, DEFAULT_DEVICE_ID);
        EXPECT_EQ(subscriberPort.value()->m_chunkReceiverData.m_memoryInfo.memoryType, DEFAULT_MEMORY_TYPE);
    }
}


TEST_F(PortPool_test, AddSubscriberPortWhenSubscriberListOverflowsReturnsError)
{
    auto addSubscriberPort = [&](const uint32_t i) -> bool {
        std::string service = "service" + std::to_string(i);
        std::string instance = "instance" + std::to_string(i);
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};


        auto publisherPort = sut.addSubscriberPort(
            {IdString_t(cxx::TruncateToCapacity, service), IdString_t(cxx::TruncateToCapacity, instance)},
            applicationName,
            m_subscriberOptions);
        return publisherPort.has_error();
    };

    for (uint32_t i = 0U; i < MAX_SUBSCRIBERS; ++i)
    {
        EXPECT_FALSE(addSubscriberPort(i));
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });
    EXPECT_TRUE(addSubscriberPort(MAX_SUBSCRIBERS));

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__SUBSCRIBERLIST_OVERFLOW);
}

TEST_F(PortPool_test, GetSubscriberPortDataListIsSuccessful)
{
    auto subscriberPort = sut.addSubscriberPort(m_serviceDescription, m_applicationName, m_subscriberOptions);
    EXPECT_FALSE(subscriberPort.has_error());
    auto subscriberPortDataList = sut.getSubscriberPortDataList();

    ASSERT_EQ(subscriberPortDataList.size(), 1U);
}

TEST_F(PortPool_test, GetSubscriberPortDataListWhenEmptyIsSuccessful)
{
    auto nodeDataList = sut.getSubscriberPortDataList();

    ASSERT_EQ(nodeDataList.size(), 0U);
}

TEST_F(PortPool_test, GetSubscriberPortDataListCompletelyFilledIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_SUBSCRIBERS; ++i)
    {
        std::string service = "service" + std::to_string(i);
        std::string instance = "instance" + std::to_string(i);
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};

        auto publisherPort = sut.addSubscriberPort(
            {IdString_t(cxx::TruncateToCapacity, service), IdString_t(cxx::TruncateToCapacity, instance)},
            applicationName,
            m_subscriberOptions);
        EXPECT_FALSE(publisherPort.has_error());
    }
    auto subscriberPortDataList = sut.getSubscriberPortDataList();

    ASSERT_EQ(subscriberPortDataList.size(), MAX_SUBSCRIBERS);
}

TEST_F(PortPool_test, RemoveSubscriberPortIsSuccessful)
{
    auto subscriberPort = sut.addSubscriberPort(m_serviceDescription, m_applicationName, m_subscriberOptions);

    sut.removeSubscriberPort(subscriberPort.value());
    auto subscriberPortDataList = sut.getSubscriberPortDataList();

    EXPECT_EQ(subscriberPortDataList.size(), 0U);
}

TEST_F(PortPool_test, AddInterfacePortIsSuccessful)
{
    auto interfacePortData = sut.addInterfacePort(m_applicationName, Interfaces::INTERNAL);

    ASSERT_THAT(interfacePortData.has_error(), Eq(false));
    EXPECT_EQ(interfacePortData.value()->m_runtimeName, m_applicationName);
    EXPECT_EQ(interfacePortData.value()->m_serviceDescription.getSourceInterface(), Interfaces::INTERNAL);
}

TEST_F(PortPool_test, AddInterfacePortWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 1U; i <= MAX_INTERFACE_NUMBER; ++i)
    {
        auto interfacePortData = sut.addInterfacePort(m_applicationName, Interfaces::INTERNAL);
        ASSERT_THAT(interfacePortData.has_error(), Eq(false));
    }

    EXPECT_EQ(sut.getInterfacePortDataList().size(), MAX_INTERFACE_NUMBER);
}

TEST_F(PortPool_test, AddInterfacePortWhenInterfaceListOverflowsReturnsError)
{
    for (uint32_t i = 0U; i < MAX_INTERFACE_NUMBER; ++i)
    {
        EXPECT_FALSE(sut.addInterfacePort(m_applicationName, Interfaces::INTERFACE_END).has_error());
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });
    EXPECT_TRUE(sut.addInterfacePort(m_applicationName, Interfaces::INTERFACE_END).has_error());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__INTERFACELIST_OVERFLOW);
}

TEST_F(PortPool_test, GetInterfacePortDataListIsSuccessful)
{
    auto interfacePort = sut.addInterfacePort(m_applicationName, Interfaces::INTERNAL);
    EXPECT_FALSE(interfacePort.has_error());
    auto interfacePortDataList = sut.getInterfacePortDataList();

    ASSERT_EQ(interfacePortDataList.size(), 1U);
}

TEST_F(PortPool_test, GetInterfacePortDataListWhenEmptyIsSuccessful)
{
    auto interfacePortDataList = sut.getInterfacePortDataList();

    ASSERT_EQ(interfacePortDataList.size(), 0U);
}

TEST_F(PortPool_test, GetInterfacePortDataListCompletelyFilledIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_INTERFACE_NUMBER; ++i)
    {
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};
        ASSERT_FALSE(sut.addInterfacePort(applicationName, Interfaces::INTERNAL).has_error());
    }
    auto interfacePortDataList = sut.getInterfacePortDataList();

    ASSERT_EQ(interfacePortDataList.size(), MAX_INTERFACE_NUMBER);
}

TEST_F(PortPool_test, RemoveInterfacePortIsSuccessful)
{
    auto interfacePort = sut.addInterfacePort(m_applicationName, Interfaces::INTERNAL);

    sut.removeInterfacePort(interfacePort.value());
    auto interfacePortDataList = sut.getInterfacePortDataList();

    ASSERT_EQ(interfacePortDataList.size(), 0U);
}

TEST_F(PortPool_test, AddApplicationPortIsSuccessful)
{
    auto applicationPortData = sut.addApplicationPort(m_applicationName);

    ASSERT_THAT(applicationPortData.has_error(), Eq(false));
    EXPECT_EQ(applicationPortData.value()->m_runtimeName, m_applicationName);
}

TEST_F(PortPool_test, AddApplicationPortWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 1U; i <= MAX_PROCESS_NUMBER; ++i)
    {
        auto applicationPortData = sut.addApplicationPort(m_applicationName);
        ASSERT_THAT(applicationPortData.has_error(), Eq(false));
    }

    EXPECT_EQ(sut.getApplicationPortDataList().size(), MAX_PROCESS_NUMBER);
}

TEST_F(PortPool_test, AddApplicationPortWhenApplicationListOverflowsReturnsError)
{
    for (uint32_t i = 0U; i < MAX_PROCESS_NUMBER; ++i)
    {
        EXPECT_FALSE(sut.addApplicationPort(m_applicationName).has_error());
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });
    EXPECT_TRUE(sut.addApplicationPort(m_applicationName).has_error());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__APPLICATIONLIST_OVERFLOW);
}

TEST_F(PortPool_test, GetApplicationPortDataListWhenEmptyIsSuccessful)
{
    auto applicationPortDataList = sut.getApplicationPortDataList();

    ASSERT_EQ(applicationPortDataList.size(), 0U);
}

TEST_F(PortPool_test, GetApplicationPortDataListIsSuccessful)
{
    ASSERT_FALSE(sut.addApplicationPort(m_applicationName).has_error());
    auto applicationPortDataList = sut.getApplicationPortDataList();

    ASSERT_EQ(applicationPortDataList.size(), 1U);
}

TEST_F(PortPool_test, GetApplicationPortDataListCompletelyFilledIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_PROCESS_NUMBER; ++i)
    {
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};
        ASSERT_FALSE(sut.addApplicationPort(applicationName).has_error());
    }
    auto applicationPortDataList = sut.getApplicationPortDataList();

    ASSERT_EQ(applicationPortDataList.size(), MAX_PROCESS_NUMBER);
}

TEST_F(PortPool_test, RemoveApplicationPortIsSuccessful)
{
    auto applicationPortData = sut.addApplicationPort(m_applicationName);

    sut.removeApplicationPort(applicationPortData.value());
    auto applicationPortDataList = sut.getApplicationPortDataList();

    ASSERT_EQ(applicationPortDataList.size(), 0U);
}

TEST_F(PortPool_test, AddConditionVariableDataIsSuccessful)
{
    auto conditionVariableData = sut.addConditionVariableData(m_applicationName);

    ASSERT_THAT(conditionVariableData.has_error(), Eq(false));
    EXPECT_EQ(conditionVariableData.value()->m_runtimeName, m_applicationName);
}

TEST_F(PortPool_test, AddConditionVariableDataWithMaxCapacityIsSuccessful)
{
    for (uint32_t i = 1U; i <= MAX_NUMBER_OF_CONDITION_VARIABLES; ++i)
    {
        auto conditionVariableData = sut.addConditionVariableData(m_applicationName);
        ASSERT_THAT(conditionVariableData.has_error(), Eq(false));
    }

    EXPECT_EQ(sut.getConditionVariableDataList().size(), MAX_NUMBER_OF_CONDITION_VARIABLES);
}

TEST_F(PortPool_test, AddConditionVariableDataWhenContainerIsFullReturnsError)
{
    for (uint32_t i = 0U; i < MAX_NUMBER_OF_CONDITION_VARIABLES; ++i)
    {
        EXPECT_FALSE(sut.addConditionVariableData(m_applicationName).has_error());
    }

    auto errorHandlerCalled{false};
    Error errorHandlerType;
    auto errorHandlerGuard =
        ErrorHandler::SetTemporaryErrorHandler([&](const Error error, const std::function<void()>, const ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });
    EXPECT_TRUE(sut.addConditionVariableData(m_applicationName).has_error());

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, Error::kPORT_POOL__CONDITION_VARIABLE_LIST_OVERFLOW);
}

TEST_F(PortPool_test, GetConditionVariableDataListIsSuccessful)
{
    ASSERT_FALSE(sut.addConditionVariableData(m_applicationName).has_error());
    auto condtionalVariableData = sut.getConditionVariableDataList();

    ASSERT_EQ(condtionalVariableData.size(), 1U);
}

TEST_F(PortPool_test, GetConditionVariableDataListWhenEmptyIsSuccessful)
{
    auto condtionalVariableData = sut.getConditionVariableDataList();

    ASSERT_EQ(condtionalVariableData.size(), 0U);
}

TEST_F(PortPool_test, GetConditionVariableDataListCompletelyFilledIsSuccessful)
{
    for (uint32_t i = 0U; i < MAX_NUMBER_OF_CONDITION_VARIABLES; ++i)
    {
        RuntimeName_t applicationName = {cxx::TruncateToCapacity, "AppName" + std::to_string(i)};
        ASSERT_FALSE(sut.addConditionVariableData(applicationName).has_error());
    }
    auto condtionalVariableData = sut.getConditionVariableDataList();

    ASSERT_EQ(condtionalVariableData.size(), MAX_NUMBER_OF_CONDITION_VARIABLES);
}

TEST_F(PortPool_test, RemoveConditionVariableDataIsSuccessful)
{
    auto conditionVariableData = sut.addConditionVariableData(m_applicationName);

    sut.removeConditionVariableData(conditionVariableData.value());
    auto condtionalVariableData = sut.getConditionVariableDataList();

    ASSERT_EQ(condtionalVariableData.size(), 0U);
}

TEST_F(PortPool_test, GetServiceRegistryChangeCounterReturnsZeroAsInitialValue)
{
    auto serviceCounter = sut.serviceRegistryChangeCounter();

    EXPECT_EQ(serviceCounter->load(), 0U);
}

TEST_F(PortPool_test, GetServiceRegistryChangeCounterIsSuccessful)
{
    sut.serviceRegistryChangeCounter()->fetch_add(1, std::memory_order_relaxed);
    auto serviceCounter = sut.serviceRegistryChangeCounter();

    ASSERT_EQ(serviceCounter->load(), 1U);
}
} // namespace test
} // namespace iox
