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

#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "mocks/publisher_mock.hpp"
#include "mocks/subscriber_mock.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

#include "iceoryx_posh/internal/roudi/introspection/port_introspection.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"

#include <cstdint>

template <typename PublisherPort, typename SubscriberPort>
class PortIntrospectionAccess : public iox::roudi::PortIntrospection<PublisherPort, SubscriberPort>
{
  public:
    using iox::roudi::PortIntrospection<PublisherPort, SubscriberPort>::sendPortData;

    void sendThroughputData()
    {
        iox::roudi::PortIntrospection<PublisherPort, SubscriberPort>::sendThroughputData();
    }
    iox::cxx::optional<PublisherPort>& getPublisherPort()
    {
        return this->m_publisherPort;
    }
    iox::cxx::optional<PublisherPort>& getPublisherPortThroughput()
    {
        return this->m_publisherPortThroughput;
    }
};

class PortIntrospection_test : public Test
{
  public:
    PortIntrospection_test()
    {
    }

    ~PortIntrospection_test()
    {
    }

    virtual void SetUp()
    {
        internal::CaptureStdout();
        ASSERT_THAT(m_introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection),
                                                                std::move(m_mockPublisherPortUserIntrospection),
                                                                std::move(m_mockPublisherPortUserIntrospection)),
                    Eq(true));
    }

    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStdout();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    bool comparePortData(const iox::roudi::SubscriberPortData& a, const iox::roudi::SubscriberPortData& b)
    {
        auto nameA = std::string(a.m_name);
        auto nameB = std::string(b.m_name);

        if (nameA.compare(nameB) != 0)
        {
            return false;
        }
        if (a.m_caproInstanceID != b.m_caproInstanceID)
        {
            return false;
        }
        if (a.m_caproServiceID != b.m_caproServiceID)
        {
            return false;
        }
        if (a.m_caproEventMethodID != b.m_caproEventMethodID)
        {
            return false;
        }
        if (a.m_node != b.m_node)
        {
            return false;
        }

        return true;
    }

    bool comparePortData(const iox::roudi::PublisherPortData& a, const iox::roudi::PublisherPortData& b)
    {
        auto nameA = std::string(a.m_name);
        auto nameB = std::string(b.m_name);

        if (nameA.compare(nameB) != 0)
        {
            return false;
        }
        if (a.m_caproInstanceID != b.m_caproInstanceID)
        {
            return false;
        }
        if (a.m_caproServiceID != b.m_caproServiceID)
        {
            return false;
        }
        if (a.m_caproEventMethodID != b.m_caproEventMethodID)
        {
            return false;
        }
        if (a.m_node != b.m_node)
        {
            return false;
        }

        return true;
    }

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};

    MockPublisherPortUser m_mockPublisherPortUserIntrospection;
    MockPublisherPortUser m_mockPublisherPortUserIntrospection2;

    PortIntrospectionAccess<MockPublisherPortUser, MockSubscriberPortUser> m_introspectionAccess;
};


TEST_F(PortIntrospection_test, registerPublisherPort)
{
    auto introspection = std::unique_ptr<iox::roudi::PortIntrospection<MockPublisherPortUser, MockSubscriberPortUser>>(
        new iox::roudi::PortIntrospection<MockPublisherPortUser, MockSubscriberPortUser>);

    EXPECT_THAT(introspection->registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection),
                                                     std::move(m_mockPublisherPortUserIntrospection),
                                                     std::move(m_mockPublisherPortUserIntrospection)),
                Eq(true));

    EXPECT_THAT(introspection->registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection2),
                                                     std::move(m_mockPublisherPortUserIntrospection2),
                                                     std::move(m_mockPublisherPortUserIntrospection2)),
                Eq(false));
}


TEST_F(PortIntrospection_test, sendPortData_EmptyList)
{
    using Topic = iox::roudi::PortIntrospectionFieldTopic;

    auto chunk = std::unique_ptr<ChunkMock<Topic>>(new ChunkMock<Topic>);
    bool chunkWasSent = false;

    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), tryAllocateChunk(_, _, _, _))
        .WillOnce(Return(iox::cxx::expected<iox::mepoo::ChunkHeader*, iox::popo::AllocationError>::create_value(
            chunk.get()->chunkHeader())));

    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), sendChunk(_))
        .WillOnce(Invoke([&](iox::mepoo::ChunkHeader* const) { chunkWasSent = true; }));

    m_introspectionAccess.sendPortData();

    ASSERT_THAT(chunkWasSent, Eq(true));

    EXPECT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
    EXPECT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));
}

TEST_F(PortIntrospection_test, addAndRemovePublisher)
{
    using PortData = iox::roudi::PublisherPortData;
    using Topic = iox::roudi::PortIntrospectionFieldTopic;

    auto chunk = std::unique_ptr<ChunkMock<Topic>>(new ChunkMock<Topic>);

    const iox::RuntimeName_t runtimeName1{"name1"};
    const iox::RuntimeName_t runtimeName2{"name2"};
    const iox::NodeName_t nodeName1{"4"};
    const iox::NodeName_t nodeName2{"jkl"};

    // prepare expected outputs
    PortData expected1;
    expected1.m_name = runtimeName1;
    expected1.m_caproInstanceID = "1";
    expected1.m_caproServiceID = "2";
    expected1.m_caproEventMethodID = "3";
    expected1.m_node = nodeName1;

    PortData expected2;
    expected2.m_name = runtimeName2;
    expected2.m_caproInstanceID = "abc";
    expected2.m_caproServiceID = "def";
    expected2.m_caproEventMethodID = "ghi";
    expected2.m_node = nodeName2;

    // prepare inputs
    iox::capro::ServiceDescription service1(
        expected1.m_caproServiceID, expected1.m_caproInstanceID, expected1.m_caproEventMethodID);
    iox::capro::ServiceDescription service2(
        expected2.m_caproServiceID, expected2.m_caproInstanceID, expected2.m_caproEventMethodID);

    iox::mepoo::MemoryManager memoryManager;
    iox::popo::PublisherOptions publisherOptions1;
    publisherOptions1.nodeName = nodeName1;
    iox::popo::PublisherOptions publisherOptions2;
    publisherOptions2.nodeName = nodeName2;
    iox::popo::PublisherPortData portData1(service1, runtimeName1, &memoryManager, publisherOptions1);
    iox::popo::PublisherPortData portData2(service2, runtimeName2, &memoryManager, publisherOptions2);
    MockPublisherPortUser port1(&portData1);
    MockPublisherPortUser port2(&portData2);
    // test adding of ports
    // remark: duplicate publisher port insertions are not possible
    EXPECT_THAT(m_introspectionAccess.addPublisher(portData1), Eq(true));
    EXPECT_THAT(m_introspectionAccess.addPublisher(portData1), Eq(false));
    EXPECT_THAT(m_introspectionAccess.addPublisher(portData2), Eq(true));
    EXPECT_THAT(m_introspectionAccess.addPublisher(portData2), Eq(false));

    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), tryAllocateChunk(_, _, _, _))
        .WillRepeatedly(Return(iox::cxx::expected<iox::mepoo::ChunkHeader*, iox::popo::AllocationError>::create_value(
            chunk.get()->chunkHeader())));

    bool chunkWasSent = false;
    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), sendChunk(_))
        .WillRepeatedly(Invoke([&](iox::mepoo::ChunkHeader* const) { chunkWasSent = true; }));

    m_introspectionAccess.sendPortData();

    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(2U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));

        auto& publisherInfo1 = chunk->sample()->m_publisherList[0];
        auto& publisherInfo2 = chunk->sample()->m_publisherList[1];

        // remark: we cannot ensure that the order is the same as the order in
        // which the ports where added  we therefore expect to find both ports
        // with the corresponding ids (we need to check whether multiple port
        // insertions also work correctly, therefore we need at least two ports)

        if (comparePortData(publisherInfo1, expected1))
        {
            EXPECT_THAT(comparePortData(publisherInfo2, expected2), Eq(true));
        }
        else
        {
            EXPECT_THAT(comparePortData(publisherInfo2, expected1), Eq(true));
        }
    }

    // test removal of ports
    EXPECT_CALL(port1, getServiceDescription()).WillRepeatedly(Return(portData1.m_serviceDescription));
    EXPECT_CALL(port1, getUniqueID()).WillRepeatedly(Return(portData1.m_uniqueId));
    EXPECT_THAT(m_introspectionAccess.removePublisher(port1), Eq(true));
    EXPECT_THAT(m_introspectionAccess.removePublisher(port1), Eq(false));


    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(1U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));

        EXPECT_THAT(comparePortData(chunk->sample()->m_publisherList[0], expected2), Eq(true));
    }

    EXPECT_CALL(port2, getServiceDescription()).WillRepeatedly(Return(portData2.m_serviceDescription));
    EXPECT_CALL(port2, getUniqueID()).WillRepeatedly(Return(portData2.m_uniqueId));
    EXPECT_THAT(m_introspectionAccess.removePublisher(port2), Eq(true));
    EXPECT_THAT(m_introspectionAccess.removePublisher(port2), Eq(false));

    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));
    }

    EXPECT_THAT(m_introspectionAccess.removePublisher(port2), Eq(false));

    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));
    }

    chunk->sample()->~PortIntrospectionFieldTopic();
}

TEST_F(PortIntrospection_test, addAndRemoveSubscriber)
{
    using PortData = iox::roudi::SubscriberPortData;
    using Topic = iox::roudi::PortIntrospectionFieldTopic;

    auto chunk = std::unique_ptr<ChunkMock<Topic>>(new ChunkMock<Topic>);

    const iox::RuntimeName_t runtimeName1{"name1"};
    const iox::RuntimeName_t runtimeName2{"name2"};
    const iox::NodeName_t nodeName1{"4"};
    const iox::NodeName_t nodeName2{"7"};

    // prepare expected outputs
    PortData expected1;
    expected1.m_name = runtimeName1;
    expected1.m_caproInstanceID = "1";
    expected1.m_caproServiceID = "2";
    expected1.m_caproEventMethodID = "3";
    expected1.m_node = nodeName1;

    PortData expected2;
    expected2.m_name = runtimeName2;
    expected2.m_caproInstanceID = "4";
    expected2.m_caproServiceID = "5";
    expected2.m_caproEventMethodID = "6";
    expected2.m_node = nodeName2;

    // prepare inputs
    iox::capro::ServiceDescription service1(
        expected1.m_caproServiceID, expected1.m_caproInstanceID, expected1.m_caproEventMethodID);
    iox::capro::ServiceDescription service2(
        expected2.m_caproServiceID, expected2.m_caproInstanceID, expected2.m_caproEventMethodID);

    iox::popo::SubscriberOptions subscriberOptions1;
    subscriberOptions1.nodeName = nodeName1;
    iox::popo::SubscriberOptions subscriberOptions2;
    subscriberOptions2.nodeName = nodeName2;

    // test adding of ports
    // remark: duplicate subscriber insertions are not possible
    iox::popo::SubscriberPortData recData1{
        service1, runtimeName1, iox::cxx::VariantQueueTypes::FiFo_MultiProducerSingleConsumer, subscriberOptions1};
    MockSubscriberPortUser port1(&recData1);
    iox::popo::SubscriberPortData recData2{
        service2, runtimeName2, iox::cxx::VariantQueueTypes::FiFo_MultiProducerSingleConsumer, subscriberOptions2};
    MockSubscriberPortUser port2(&recData2);
    EXPECT_THAT(m_introspectionAccess.addSubscriber(recData1), Eq(true));
    EXPECT_THAT(m_introspectionAccess.addSubscriber(recData1), Eq(false));
    EXPECT_THAT(m_introspectionAccess.addSubscriber(recData2), Eq(true));
    EXPECT_THAT(m_introspectionAccess.addSubscriber(recData2), Eq(false));

    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), tryAllocateChunk(_, _, _, _))
        .WillRepeatedly(Return(iox::cxx::expected<iox::mepoo::ChunkHeader*, iox::popo::AllocationError>::create_value(
            chunk.get()->chunkHeader())));

    bool chunkWasSent = false;
    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), sendChunk(_))
        .WillRepeatedly(Invoke([&](iox::mepoo::ChunkHeader* const) { chunkWasSent = true; }));

    m_introspectionAccess.sendPortData();

    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(2U));

        auto& subscriberInfo1 = chunk->sample()->m_subscriberList[0];
        auto& subscriberInfo2 = chunk->sample()->m_subscriberList[1];

        // remark: we cannot ensure that the order is the same as the order in
        // which the ports where added  we therefore expect to find both ports
        // with the corresponding ids (we need to check whether multiple port
        // insertions also work correctly, therefore we need at least two ports)

        if (comparePortData(subscriberInfo1, expected1))
        {
            EXPECT_THAT(comparePortData(subscriberInfo2, expected2), Eq(true));
        }
        else
        {
            EXPECT_THAT(comparePortData(subscriberInfo2, expected1), Eq(true));
        }
    }

    // test removal of ports
    EXPECT_CALL(port1, getUniqueID()).WillRepeatedly(Return(recData1.m_uniqueId));
    EXPECT_CALL(port1, getServiceDescription()).WillRepeatedly(Return(recData1.m_serviceDescription));
    EXPECT_THAT(m_introspectionAccess.removeSubscriber(port1), Eq(true));
    EXPECT_THAT(m_introspectionAccess.removeSubscriber(port1), Eq(false));

    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(1U));

        auto& publisherInfo = chunk->sample()->m_subscriberList[0];

        EXPECT_THAT(comparePortData(publisherInfo, expected2), Eq(true));
    }

    EXPECT_CALL(port2, getUniqueID()).WillRepeatedly(Return(recData2.m_uniqueId));
    EXPECT_CALL(port2, getServiceDescription()).WillRepeatedly(Return(recData2.m_serviceDescription));
    EXPECT_THAT(m_introspectionAccess.removeSubscriber(port2), Eq(true));
    EXPECT_THAT(m_introspectionAccess.removeSubscriber(port2), Eq(false));

    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));
    }

    EXPECT_THAT(m_introspectionAccess.removeSubscriber(port2), Eq(false));

    chunkWasSent = false;
    m_introspectionAccess.sendPortData();
    ASSERT_THAT(chunkWasSent, Eq(true));

    {
        ASSERT_THAT(chunk->sample()->m_publisherList.size(), Eq(0U));
        ASSERT_THAT(chunk->sample()->m_subscriberList.size(), Eq(0U));
    }

    chunk->sample()->~PortIntrospectionFieldTopic();
}


TEST_F(PortIntrospection_test, DISABLED_thread)
{
    using PortData = iox::roudi::PortIntrospectionFieldTopic;
    auto chunkPortData = std::unique_ptr<ChunkMock<PortData>>(new ChunkMock<PortData>);

    using PortThroughput = iox::roudi::PortThroughputIntrospectionFieldTopic;
    ChunkMock<PortThroughput> chunkPortThroughput;

    using SubscriberPortChanging = iox::roudi::SubscriberPortChangingIntrospectionFieldTopic;
    ChunkMock<SubscriberPortChanging> chunkSubscriberPortChanging;

    EXPECT_CALL(m_introspectionAccess.getPublisherPort().value(), sendChunk(_)).Times(AtLeast(4));

    // we use the deliverChunk call to check how often the thread calls the send method
    using namespace iox::units::duration_literals;
    m_introspectionAccess.setSendInterval(10_ms);
    m_introspectionAccess.run();
    /// @todo this time can be reduced when the sleep mechanism of the port introspection thread is replace by a trigger
    /// queue
    std::this_thread::sleep_for(std::chrono::milliseconds(555)); // within this time, the thread should have run 6 times
    m_introspectionAccess.stop();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(555)); // if the thread doesn't stop, we have 12 runs after the sleep period
}
