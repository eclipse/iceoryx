// Copyright (c) 2019, 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_hoofs/cxx/convert.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_posh/testing/roudi_gtest.hpp"

namespace
{
using iox::capro::IdString_t;
using iox::capro::ServiceDescription;
using iox::runtime::ServiceContainer;

class RoudiFindService_test : public RouDi_GTest
{
  public:
    void SetUp()
    {
    }

    void TearDown()
    {
    }

    iox::runtime::PoshRuntime* senderRuntime{&iox::runtime::PoshRuntime::initRuntime("sender")};
    iox::runtime::PoshRuntime* receiverRuntime{&iox::runtime::PoshRuntime::initRuntime("receiver")};
};

TEST_F(RoudiFindService_test, OfferSingleMethodServiceSingleInstance)
{
    auto isServiceOffered = senderRuntime->offerService({"service1", "instance1", "event1"});
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));

    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));
    ASSERT_EQ(true, isServiceOffered);
}


TEST_F(RoudiFindService_test, OfferServiceWithDefaultServiceDescriptionFails)
{
    auto isServiceOffered = senderRuntime->offerService(iox::capro::ServiceDescription());
    this->InterOpWait();

    ASSERT_EQ(false, isServiceOffered);
}

TEST_F(RoudiFindService_test, OfferServiceWithValidEventIdSucessfull)
{
    auto isServiceOffered = senderRuntime->offerService({"service1", "instance1", "event1"});
    this->InterOpWait();

    ASSERT_EQ(true, isServiceOffered);
}

TEST_F(RoudiFindService_test, OfferServiceWithInvalidEventIdFails)
{
    auto isServiceOffered =
        senderRuntime->offerService({"service1", iox::capro::InvalidIdString, iox::capro::InvalidIdString});
    this->InterOpWait();

    ASSERT_EQ(false, isServiceOffered);
}

TEST_F(RoudiFindService_test, ReofferedServiceWithValidServiceDescriptionCanBeFound)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));

    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));
}

TEST_F(RoudiFindService_test, OfferExsistingServiceMultipleTimesIsRedundant)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));

    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));
}

TEST_F(RoudiFindService_test, FindSameServiceMultipleTimesReturnsSingleInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));
}

TEST_F(RoudiFindService_test, OfferMultiMethodServiceSingleInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service3", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service3"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service3", "instance1", "event1"}));
}

TEST_F(RoudiFindService_test, OfferMultiMethodServiceWithDistinctSingleInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance2", "event2"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance2"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance2", "event2"}));
}

TEST_F(RoudiFindService_test, SubscribeAnyInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance2", "event2"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance3", "event3"}));
    this->InterOpWait();
    ServiceContainer serviceContainerExp;
    serviceContainerExp.push_back({"service1", "instance1", "event1"});
    serviceContainerExp.push_back({"service1", "instance2", "event2"});
    serviceContainerExp.push_back({"service1", "instance3", "event3"});

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), iox::runtime::Wildcard_t());
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(3u));
    EXPECT_TRUE(serviceContainer.value() == serviceContainerExp);
}

TEST_F(RoudiFindService_test, OfferSingleMethodServiceMultiInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance2", "event2"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance3", "event3"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance2"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance2", "event2"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance3"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance3", "event3"}));
}

TEST_F(RoudiFindService_test, OfferMultiMethodServiceMultiInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance2", "event2"}));
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance3", "event3"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance2", "event2"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance3", "event3"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance2"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance2", "event2"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance3"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance3", "event3"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance2"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance2", "event2"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance3"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance3", "event3"}));
}

TEST_F(RoudiFindService_test, StopOfferWithInvalidServiceDescriptionFails)
{
    EXPECT_FALSE(senderRuntime->stopOfferService(
        {iox::capro::InvalidIdString, iox::capro::InvalidIdString, iox::capro::InvalidIdString}));
}

TEST_F(RoudiFindService_test, StopOfferSingleMethodServiceSingleInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));
}

TEST_F(RoudiFindService_test, StopOfferMultiMethodServiceSingleInstance)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service3", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->stopOfferService({"service3", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));

    serviceContainer = receiverRuntime->findService(IdString_t("service2"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1u));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service2", "instance1", "event1"}));

    serviceContainer = receiverRuntime->findService(IdString_t("service3"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));
}

TEST_F(RoudiFindService_test, StopOfferServiceRedundantCall)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));
}


TEST_F(RoudiFindService_test, StopNonExistingService)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();
    EXPECT_TRUE(senderRuntime->stopOfferService({"service2", "instance2", "event2"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(1));
    ASSERT_THAT(*serviceContainer.value().begin(), Eq(ServiceDescription{"service1", "instance1", "event1"}));
}

TEST_F(RoudiFindService_test, FindNonExistingServices)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service2", "instance1", "event1"}));
    EXPECT_TRUE(senderRuntime->offerService({"service3", "instance1", "event1"}));
    this->InterOpWait();

    auto serviceContainer = receiverRuntime->findService(IdString_t("service1"), IdString_t("schlomo"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));

    serviceContainer = receiverRuntime->findService(IdString_t("ignatz"), IdString_t("instance1"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));

    serviceContainer = receiverRuntime->findService(IdString_t("ignatz"), IdString_t("schlomo"));
    ASSERT_FALSE(serviceContainer.has_error());
    ASSERT_THAT(serviceContainer.value().size(), Eq(0u));
}

TEST_F(RoudiFindService_test, InterfacePort)
{
    EXPECT_TRUE(senderRuntime->offerService({"service1", "instance1", "event1"}));
    this->InterOpWait();

    auto interfacePortData = receiverRuntime->getMiddlewareInterface(iox::capro::Interfaces::SOMEIP);
    iox::popo::InterfacePort interfacePort(interfacePortData);
    this->InterOpWait();
    bool serviceFound = false;

    while (auto maybeCaProMessage = interfacePort.tryGetCaProMessage())
    {
        auto caproMessage = maybeCaProMessage.value();
        if ((caproMessage.m_serviceDescription.getServiceIDString() == IdString_t("service1"))
            && (caproMessage.m_serviceDescription.getInstanceIDString() == IdString_t("instance1"))
            && ((caproMessage.m_serviceDescription.getEventIDString() == IdString_t("event1"))))
        {
            serviceFound = true;
            break;
        }
    }

    EXPECT_THAT(serviceFound, Eq(true));
}

TEST_F(RoudiFindService_test, findServiceMaxServices)
{
    ServiceContainer serviceContainerExp;
    for (size_t i = 0; i < iox::MAX_NUMBER_OF_SERVICES; i++)
    {
        // Service & Instance string is kept short , to reduce the response size in find service request ,
        // (message queue has a limit of 512)
        std::string instance = "i" + iox::cxx::convert::toString(i);
        EXPECT_TRUE(senderRuntime->offerService({"s", IdString_t(iox::cxx::TruncateToCapacity, instance), "foo"}));
        serviceContainerExp.push_back({"s", IdString_t(iox::cxx::TruncateToCapacity, instance), "foo"});
        this->InterOpWait();
    }

    auto serviceContainer = receiverRuntime->findService(IdString_t("s"), iox::runtime::Wildcard_t());

    ASSERT_FALSE(serviceContainer.has_error());
    EXPECT_THAT(serviceContainer.value().size(), Eq(iox::MAX_NUMBER_OF_SERVICES));
    EXPECT_TRUE(serviceContainer.value() == serviceContainerExp);
}

TEST_F(RoudiFindService_test, findServiceserviceContainerOverflowError)
{
    size_t noOfInstances = (iox::MAX_NUMBER_OF_SERVICES + 1);
    ServiceContainer serviceContainerExp;
    for (size_t i = 0; i < noOfInstances; i++)
    {
        std::string instance = "i" + iox::cxx::convert::toString(i);
        EXPECT_TRUE(senderRuntime->offerService({"s", IdString_t(iox::cxx::TruncateToCapacity, instance), "foo"}));
        serviceContainerExp.push_back({"s", IdString_t(iox::cxx::TruncateToCapacity, instance), "foo"});
        this->InterOpWait();
    }

    auto serviceContainer = receiverRuntime->findService(IdString_t("s"), iox::runtime::Wildcard_t());

    ASSERT_THAT(serviceContainer.has_error(), Eq(true));
}

} // namespace
