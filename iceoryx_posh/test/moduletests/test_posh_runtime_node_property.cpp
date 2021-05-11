// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/runtime/node_property.hpp"
#include "iceoryx_posh/internal/runtime/ipc_message.hpp"

#include "test.hpp"

using namespace ::testing;
using namespace iox::runtime;
using namespace iox::roudi;


namespace iox
{
namespace test
{
/// @brief Test goal: This test suit verifies class node

class PoshRuntimeNodeProperty_test : public Test
{
  public:
    PoshRuntimeNodeProperty_test()
    {
    }

    virtual ~PoshRuntimeNodeProperty_test()
    {
    }
};

TEST_F(PoshRuntimeNodeProperty_test, ConstructorNodePropertyWithNodeNameIsSuccessful)
{
    const NodeName_t nodeName{"Node"};
    uint64_t nodeDeviceIdentifier = 1U;

    NodeProperty sut(nodeName,nodeDeviceIdentifier);

    EXPECT_EQ(sut.m_name, nodeName);
    EXPECT_EQ(sut.m_nodeDeviceIdentifier, nodeDeviceIdentifier);
}

TEST_F(PoshRuntimeNodeProperty_test, ConstructorNodePropertyWithSerializationIsSuccessful)
{
    const NodeName_t nodeName{"Node"};
    uint64_t nodeDeviceIdentifier = 1U;

    NodeProperty sut(nodeName,nodeDeviceIdentifier);

    IpcMessage sendBuffer;
    sendBuffer << static_cast<cxx::Serialization>(sut).toString();

    NodeProperty sut2(cxx::Serialization(sendBuffer.getElementAtIndex(0U)));

    EXPECT_EQ(sut2.m_name, nodeName);
    EXPECT_EQ(sut2.m_nodeDeviceIdentifier, nodeDeviceIdentifier);
}

TEST_F(PoshRuntimeNodeProperty_test, ConstructorNodePropertyWithWrongSerializationIsNotSuccessful)
{
    const NodeName_t nodeName{""};
    NodeProperty sut(cxx::Serialization("Node"));

    EXPECT_EQ(sut.m_name, nodeName);
}

} // namespace test
} // namespace iox
