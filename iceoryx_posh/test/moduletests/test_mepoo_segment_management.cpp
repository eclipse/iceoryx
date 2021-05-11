// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/segment_manager.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/mepoo/segment_config.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/base_relative_pointer.hpp"
#include "iceoryx_utils/testing/test_definitions.hpp"
#include "test.hpp"


using namespace ::testing;
using namespace iox::mepoo;
using namespace iox::posix;

class MePooSegmentMock
{
  public:
    MePooSegmentMock(const MePooConfig& mempoolConfig IOX_MAYBE_UNUSED,
                     Allocator& managementAllocator IOX_MAYBE_UNUSED,
                     const PosixGroup& readerGroup IOX_MAYBE_UNUSED,
                     const PosixGroup& writerGroup IOX_MAYBE_UNUSED,
                     const MemoryInfo& memoryInfo IOX_MAYBE_UNUSED) noexcept
    {
    }
};

class SegmentManager_test : public Test
{
  public:
    SegmentManager_test()
        : surpressOutput{SurpressOutput(true)}
    {
    }

    ~SegmentManager_test()
    {
        if (surpressOutput)
        {
            if (Test::HasFailure())
            {
                std::cout << testing::internal::GetCapturedStderr() << std::endl;
            }
            else
            {
                (void)testing::internal::GetCapturedStderr();
            }
        }
    }

    void SetUp(){};
    void TearDown()
    {
        iox::rp::BaseRelativePointer::unregisterAll();
    };

    MePooConfig getMempoolConfig()
    {
        MePooConfig config;
        config.addMemPool({128, 5});
        config.addMemPool({256, 7});
        return config;
    }

    SegmentConfig getSegmentConfig()
    {
        SegmentConfig config;
        config.m_sharedMemorySegments.push_back({"iox_roudi_test1", "iox_roudi_test2", mepooConfig});
        config.m_sharedMemorySegments.push_back({"iox_roudi_test2", "iox_roudi_test3", mepooConfig});
        return config;
    }

    SegmentConfig getInvalidSegmentConfig()
    {
        SegmentConfig config;
        config.m_sharedMemorySegments.push_back({"iox_roudi_test1", "iox_roudi_test1", mepooConfig});
        config.m_sharedMemorySegments.push_back({"iox_roudi_test3", "iox_roudi_test1", mepooConfig});
        return config;
    }

    SegmentConfig getSegmentConfigWithMaximumNumberOfSegements()
    {
        SegmentConfig config;
        for (uint64_t i = 0U; i < iox::MAX_SHM_SEGMENTS; ++i)
        {
            config.m_sharedMemorySegments.push_back({"iox_roudi_test1", "iox_roudi_test1", mepooConfig});
        }
        return config;
    }

    bool SurpressOutput(const bool v)
    {
        if (v)
        {
            testing::internal::CaptureStderr();
        }
        return v;
    }

    bool surpressOutput;
    static constexpr size_t MEM_SIZE{20000};
    char memory[MEM_SIZE];
    iox::posix::Allocator allocator{memory, MEM_SIZE};
    MePooConfig mepooConfig = getMempoolConfig();
    SegmentConfig segmentConfig = getSegmentConfig();
    SegmentManager<> sut{segmentConfig, &allocator};
};

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getSegmentMappingsForReadUser))
{
    auto mapping = sut.getSegmentMappings({"iox_roudi_test1"});
    ASSERT_THAT(mapping.size(), Eq(1u));
    EXPECT_THAT(mapping[0].m_isWritable, Eq(false));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getSegmentMappingsForWriteUser))
{
    auto mapping = sut.getSegmentMappings({"iox_roudi_test2"});
    ASSERT_THAT(mapping.size(), Eq(2u));
    EXPECT_THAT(mapping[0].m_isWritable == mapping[1].m_isWritable, Eq(false));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getSegmentMappingsEmptyForNonRegisteredUser))
{
    auto mapping = sut.getSegmentMappings({"roudi_test4"});
    ASSERT_THAT(mapping.size(), Eq(0u));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getSegmentMappingsEmptyForNonExistingUser))
{
    auto mapping = sut.getSegmentMappings({"no_user"});
    ASSERT_THAT(mapping.size(), Eq(0u));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getMemoryManagerForUserWithWriteUser))
{
    auto memoryManager = sut.getSegmentInformationWithWriteAccessForUser({"iox_roudi_test2"}).m_memoryManager;
    ASSERT_TRUE(memoryManager.has_value());
    ASSERT_THAT(memoryManager.value().get().getNumberOfMemPools(), Eq(2u));

    auto poolInfo1 = memoryManager.value().get().getMemPoolInfo(0);
    auto poolInfo2 = memoryManager.value().get().getMemPoolInfo(1);
    EXPECT_THAT(poolInfo1.m_numChunks, Eq(5u));
    EXPECT_THAT(poolInfo2.m_numChunks, Eq(7u));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getMemoryManagerForUserFailWithReadOnlyUser))
{
    EXPECT_FALSE(sut.getSegmentInformationWithWriteAccessForUser({"iox_roudi_test1"}).m_memoryManager.has_value());
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(getMemoryManagerForUserFailWithNonExistingUser))
{
    EXPECT_FALSE(sut.getSegmentInformationWithWriteAccessForUser({"no_user"}).m_memoryManager.has_value());
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(addingMoreThanOneWriterGroupFails))
{
    auto errorHandlerCalled{false};
    iox::Error receivedError{iox::Error::kNO_ERROR};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled,
         &receivedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
            receivedError = error;
        });

    SegmentConfig segmentConfig = getInvalidSegmentConfig();
    SegmentManager<> sut{segmentConfig, &allocator};
    sut.getSegmentMappings(PosixUser("iox_roudi_test1"));

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_THAT(receivedError, Eq(iox::Error::kMEPOO__USER_WITH_MORE_THAN_ONE_WRITE_SEGMENT));
}

TEST_F(SegmentManager_test, ADD_TEST_WITH_ADDITIONAL_USER(addingMaximumNumberOfSegmentsWorks))
{
    SegmentConfig segmentConfig = getSegmentConfigWithMaximumNumberOfSegements();
    SegmentManager<MePooSegmentMock> sut{segmentConfig, &allocator};
}
