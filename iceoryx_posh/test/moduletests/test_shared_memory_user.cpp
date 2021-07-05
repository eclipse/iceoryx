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

#include "iceoryx_posh/internal/roudi_environment/roudi_environment.hpp"
#include "iceoryx_posh/internal/runtime/shared_memory_user.hpp"

#include "test.hpp"

using namespace ::testing;
using namespace iox::runtime;
using namespace iox::roudi;

namespace iox
{
namespace test
{
class SharedMemoryUserTest : public Test
{
  public:
    RouDiEnvironment m_roudiEnv{iox::RouDiConfig_t().setDefaults()};
    const uint64_t segmentId{1U};
    rp::BaseRelativePointer::offset_t segmentManagerAddressOffset{0U};
    void SetUp(){}
    void TearDown(){}
};

TEST_F(SharedMemoryUserTest, ConstructorShmObjectWithZeroTopicSizeAndDoMapSharedMemoryIntoThreadBoolSetToTrueReturnError)
{
    iox::cxx::optional<iox::Error> detectedError;
    const bool doMapSharedMemoryIntoThread{"TRUE"};
    const size_t topicSize{0U};

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
    [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
    detectedError.emplace(error);
    EXPECT_EQ(errorLevel, iox::ErrorLevel::FATAL);
    });
    SharedMemoryUser ShmUser(doMapSharedMemoryIntoThread, topicSize, segmentId, segmentManagerAddressOffset);
  
    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOSH__SHM_APP_MAPP_ERR);
}

TEST_F(SharedMemoryUserTest, ConstructorShmObjectWithZeroTopicSizeAndDoMapSharedMemoryIntoThreadBoolSetToFalseReturnError)
{
    iox::cxx::optional<iox::Error> detectedError;
    const bool doMapSharedMemoryIntoThread{"FALSE"};
    const size_t topicSize{0U};

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
    [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
    detectedError.emplace(error);
    EXPECT_EQ(errorLevel, iox::ErrorLevel::FATAL);
    });
    SharedMemoryUser ShmUser(doMapSharedMemoryIntoThread, topicSize, segmentId, segmentManagerAddressOffset);
  
    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOSH__SHM_APP_MAPP_ERR);
}

TEST_F(SharedMemoryUserTest, ConstructorShmObjectWithValidTopicSizeAndDoMapSharedMemoryIntoThreadBoolSetToFalseReturnNoError)
{
    iox::cxx::optional<iox::Error> detectedError;
    const bool doMapSharedMemoryIntoThread{"FALSE"};
    const size_t topicSize{1U};

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
    [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
    detectedError.emplace(error);
    EXPECT_EQ(errorLevel, iox::ErrorLevel::FATAL);
    });

    SharedMemoryUser ShmUser(doMapSharedMemoryIntoThread, topicSize, segmentId, segmentManagerAddressOffset);
    EXPECT_FALSE(detectedError.has_value());
}

TEST_F(SharedMemoryUserTest, ConstructorShmObjectWithValidTopicSizeAndDoMapSharedMemoryIntoThreadBoolSetToTrueReturnNoError)
{
    iox::cxx::optional<iox::Error> detectedError;
    const bool doMapSharedMemoryIntoThread{"TRUE"};
    const size_t topicSize{1U};

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
    [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
    detectedError.emplace(error);
    EXPECT_EQ(errorLevel, iox::ErrorLevel::FATAL);
    });
    SharedMemoryUser ShmUser(doMapSharedMemoryIntoThread, topicSize, segmentId, segmentManagerAddressOffset);
  
    EXPECT_FALSE(detectedError.has_value());
}
} // namespace test
} // namespace iox
