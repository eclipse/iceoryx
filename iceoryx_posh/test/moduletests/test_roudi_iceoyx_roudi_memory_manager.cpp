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

#include "test.hpp"

#include "iceoryx_posh/roudi/memory/iceoryx_roudi_memory_manager.hpp"

using namespace ::testing;

using iox::roudi::IceOryxRouDiMemoryManager;

namespace iox
{
namespace test
{
/// @brief This test file verifies that the BaseClass IceoryxRouDiMemoryManager is tested
class IceoryxRoudiMemoryManager_test : public Test
{
  public:
    std::unique_ptr<IceOryxRouDiMemoryManager> m_roudiMemoryManagerTest;

    void SetUp() override
    {
        auto config = iox::RouDiConfig_t().setDefaults();
        m_roudiMemoryManagerTest = std::unique_ptr<IceOryxRouDiMemoryManager>(new IceOryxRouDiMemoryManager(config));
    }

    void TearDown() override
    {
    }
};

TEST_F(IceoryxRoudiMemoryManager_test, ConstructorSuccess)
{
    EXPECT_THAT(m_roudiMemoryManagerTest, Not(Eq(nullptr)));
}

TEST_F(IceoryxRoudiMemoryManager_test, IntrospectionMemoryManagerNulloptWhenNotPresent)
{
    auto result = m_roudiMemoryManagerTest->introspectionMemoryManager();
    EXPECT_THAT(result, Eq(iox::cxx::nullopt_t()));
}

TEST_F(IceoryxRoudiMemoryManager_test, segmentManagerNulloptWhenNotPresent)
{
    auto resultTest = m_roudiMemoryManagerTest->segmentManager();
    EXPECT_THAT(resultTest, Eq(iox::cxx::nullopt_t()));
}

TEST_F(IceoryxRoudiMemoryManager_test, portPoolNulloptWhenNotPresent)
{
    auto testResult = m_roudiMemoryManagerTest->portPool();
    EXPECT_THAT(testResult, Eq(iox::cxx::nullopt_t()));
}

TEST_F(IceoryxRoudiMemoryManager_test, CreateAndAnnouceMemoryHasNoError)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    EXPECT_THAT(testResult.has_error(), Eq(false));
}

TEST_F(IceoryxRoudiMemoryManager_test, MgmtMemoryProviderReturnNonNullPtr)
{
    auto testResult = m_roudiMemoryManagerTest->mgmtMemoryProvider();
    EXPECT_THAT(testResult, Not(Eq(nullptr)));
}

TEST_F(IceoryxRoudiMemoryManager_test, AcquiringIntrospectionMemoryManagerAfterCreateAndAnnounceMemoryIsSuccessful)
{
    auto tr = m_roudiMemoryManagerTest->createAndAnnounceMemory();

    EXPECT_THAT(tr.has_error(), Eq(false));

    auto result = m_roudiMemoryManagerTest->introspectionMemoryManager();
    EXPECT_THAT(result, Not(Eq(iox::cxx::nullopt_t())));
}

TEST_F(IceoryxRoudiMemoryManager_test, AcquiringSegmentManagerAfterCreateAndAnnounceMemoryIsSuccessful)
{
    auto tr = m_roudiMemoryManagerTest->createAndAnnounceMemory();

    EXPECT_THAT(tr.has_error(), Eq(false));

    auto resultTest = m_roudiMemoryManagerTest->segmentManager();
    EXPECT_THAT(resultTest, Not(Eq(iox::cxx::nullopt_t())));
}

TEST_F(IceoryxRoudiMemoryManager_test, AcquiringPortPoolAfterCreateAndAnnounceMemoryIsSuccessful)
{
    auto tr = m_roudiMemoryManagerTest->createAndAnnounceMemory();

    EXPECT_THAT(tr.has_error(), Eq(false));

    auto testResult = m_roudiMemoryManagerTest->portPool();
    EXPECT_THAT(testResult, Not(Eq(iox::cxx::nullopt_t())));
}

TEST_F(IceoryxRoudiMemoryManager_test, DestroyMemoryReturnNoError)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    EXPECT_FALSE(testResult.has_error());

    auto result = m_roudiMemoryManagerTest->destroyMemory();

    EXPECT_THAT(result.has_error(), Eq(false));
}

TEST_F(IceoryxRoudiMemoryManager_test, DestroyMemoryIntrospectionMemoryManagerReturnNullOpt)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    ASSERT_FALSE(testResult.has_error());

    auto result = m_roudiMemoryManagerTest->destroyMemory();
    ASSERT_FALSE(result.has_error());

    auto res = m_roudiMemoryManagerTest->introspectionMemoryManager();
    EXPECT_THAT(res, Eq(iox::cxx::nullopt_t()));
}

TEST_F(IceoryxRoudiMemoryManager_test, DestroyMemorySegmentManagerReturnNullOpt)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    ASSERT_FALSE(testResult.has_error());

    auto result = m_roudiMemoryManagerTest->destroyMemory();
    ASSERT_FALSE(result.has_error());

    auto resultTest = m_roudiMemoryManagerTest->segmentManager();
    EXPECT_THAT(resultTest, Eq(iox::cxx::nullopt_t()));
}

TEST_F(IceoryxRoudiMemoryManager_test, CreateAndAnnouceMemoryFailingAfterCalledTwoTimes)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    ASSERT_FALSE(testResult.has_error());

    auto result = m_roudiMemoryManagerTest->createAndAnnounceMemory();

    EXPECT_THAT(result.has_error(), Eq(true));
    EXPECT_THAT(result.get_error(), Eq(iox::roudi::RouDiMemoryManagerError::MEMORY_CREATION_FAILED));
}

TEST_F(IceoryxRoudiMemoryManager_test, DestroyMemoryNotFailingAfterCalledTwoTimes)
{
    auto testResult = m_roudiMemoryManagerTest->createAndAnnounceMemory();
    ASSERT_FALSE(testResult.has_error());

    auto result = m_roudiMemoryManagerTest->destroyMemory();
    ASSERT_FALSE(result.has_error());

    auto res = m_roudiMemoryManagerTest->destroyMemory();

    EXPECT_THAT(res.has_error(), Eq(false));
}

} // namespace test
} // namespace iox
