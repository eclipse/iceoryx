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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/ports/application_port.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/interface_port.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "test.hpp"

using namespace ::testing;
using namespace iox::capro;
using namespace iox::popo;

const iox::capro::ServiceDescription SERVICE_DESCRIPTION_VALID("Radar", "FrontRight", "ChuckNorrisDetected");
const iox::capro::ServiceDescription SERVICE_DESCRIPTION_EMPTY(0, 0, 0);

const iox::RuntimeName_t RUNTIME_NAME_EMPTY = {""};
const iox::RuntimeName_t RUNTIME_NAME_FOR_PUBLISHER_PORTS = {"PublisherPort"};
const iox::RuntimeName_t RUNTIME_NAME_FOR_SUBSCRIBER_PORTS = {"SubscriberPort"};
const iox::RuntimeName_t RUNTIME_NAME_FOR_INTERFACE_PORTS = {"InterfacePort"};
const iox::RuntimeName_t RUNTIME_NAME_FOR_APPLICATION_PORTS = {"AppPort"};

iox::mepoo::MemoryManager m_memoryManager;
std::vector<iox::UniquePortId> uniquePortIds;

using PortDataTypes =
    Types<BasePortData, PublisherPortData, SubscriberPortData, InterfacePortData, ApplicationPortData>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(BasePort_test, PortDataTypes);
#pragma GCC diagnostic pop

// port data factories

template <typename T>
T* createPortData()
{
    return nullptr;
}
template <>
BasePortData* createPortData()
{
    return new BasePortData();
}
template <>
PublisherPortData* createPortData()
{
    PublisherOptions options;
    options.historyCapacity = 1U;
    return new PublisherPortData(
        SERVICE_DESCRIPTION_VALID, RUNTIME_NAME_FOR_PUBLISHER_PORTS, &m_memoryManager, options);
}
template <>
SubscriberPortData* createPortData()
{
    return new SubscriberPortData(SERVICE_DESCRIPTION_VALID,
                                  RUNTIME_NAME_FOR_SUBSCRIBER_PORTS,
                                  iox::cxx::VariantQueueTypes::FiFo_MultiProducerSingleConsumer,
                                  SubscriberOptions());
}
template <>
InterfacePortData* createPortData()
{
    return new InterfacePortData(RUNTIME_NAME_FOR_INTERFACE_PORTS, iox::capro::Interfaces::INTERNAL);
}
template <>
ApplicationPortData* createPortData()
{
    return new ApplicationPortData(RUNTIME_NAME_FOR_APPLICATION_PORTS);
}

// expected ServiceDescription factories

template <typename T>
const ServiceDescription& expectedServiceDescription()
{
    return SERVICE_DESCRIPTION_EMPTY;
}
template <>
const ServiceDescription& expectedServiceDescription<PublisherPortData>()
{
    return SERVICE_DESCRIPTION_VALID;
}
template <>
const ServiceDescription& expectedServiceDescription<SubscriberPortData>()
{
    return SERVICE_DESCRIPTION_VALID;
}

// expected ProcessName factories

template <typename T>
const iox::RuntimeName_t& expectedProcessName()
{
    return RUNTIME_NAME_EMPTY;
}
template <>
const iox::RuntimeName_t& expectedProcessName<PublisherPortData>()
{
    return RUNTIME_NAME_FOR_PUBLISHER_PORTS;
}
template <>
const iox::RuntimeName_t& expectedProcessName<SubscriberPortData>()
{
    return RUNTIME_NAME_FOR_SUBSCRIBER_PORTS;
}
template <>
const iox::RuntimeName_t& expectedProcessName<InterfacePortData>()
{
    return RUNTIME_NAME_FOR_INTERFACE_PORTS;
}
template <>
const iox::RuntimeName_t& expectedProcessName<ApplicationPortData>()
{
    return RUNTIME_NAME_FOR_APPLICATION_PORTS;
}

template <typename PortData>
class BasePort_test : public Test
{
  public:
    using PortData_t = PortData;

    virtual void SetUp()
    {
        for (auto& id : uniquePortIds)
        {
            EXPECT_THAT(this->sut.getUniqueID(), Ne(id));
        }
        uniquePortIds.emplace_back(sut.getUniqueID());
    }

    virtual void TearDown()
    {
    }

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};

    std::unique_ptr<PortData> sutData{createPortData<PortData>()};
    BasePort sut{sutData.get()};
};

TYPED_TEST(BasePort_test, CallingGetCaProServiceDescriptionWorks)
{
    using PortData_t = typename TestFixture::PortData_t;
    EXPECT_THAT(this->sut.getCaProServiceDescription(), Eq(expectedServiceDescription<PortData_t>()));
}

TYPED_TEST(BasePort_test, CallingGetRuntimeNameWorks)
{
    using PortData_t = typename TestFixture::PortData_t;
    EXPECT_THAT(this->sut.getRuntimeName(), Eq(expectedProcessName<PortData_t>()));
}
