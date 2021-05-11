// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_utils/cxx/unique_ptr.hpp"

#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::_;

// anonymous namespace to prevent linker issues or sanitizer false positives
// if a struct with the same name is used in other tests
namespace
{
struct DummyData
{
    DummyData() = default;
    uint32_t val = 42;
};
struct DummyHeader
{
    DummyHeader() = default;
    uint64_t counter = 0;
};
} // namespace

template <typename T, typename H = iox::mepoo::NoUserHeader>
class MockPublisherInterface : public iox::popo::PublisherInterface<T, H>
{
  public:
    void publish(iox::popo::Sample<T, H>&& sample) noexcept override
    {
        auto s = std::move(sample); // this step is necessary since the mock method doesn't execute the move
        return publishMock(std::move(s));
    }
    MOCK_METHOD1_T(publishMock, void(iox::popo::Sample<T, H>&&));
};

class SampleTest : public Test
{
  public:
    SampleTest()
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

  protected:
};

TEST_F(SampleTest, PublishesSampleViaPublisherInterfaceWorks)
{
    // ===== Setup ===== //
    ChunkMock<DummyData> chunk;
    iox::cxx::unique_ptr<DummyData> testSamplePtr{chunk.sample(), [](DummyData*) {}};
    MockPublisherInterface<DummyData> mockPublisherInterface{};

    auto sut = iox::popo::Sample<DummyData>(std::move(testSamplePtr), mockPublisherInterface);

    EXPECT_CALL(mockPublisherInterface, publishMock).Times(1);

    // ===== Test ===== //
    sut.publish();

    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SampleTest, PublishingEmptySampleCallsErrorHandler)
{
    // ===== Setup ===== //
    ChunkMock<DummyData> chunk;
    iox::cxx::unique_ptr<DummyData> testSamplePtr{chunk.sample(), [](DummyData*) {}};
    MockPublisherInterface<DummyData> mockPublisherInterface{};

    auto sut = iox::popo::Sample<DummyData>(std::move(testSamplePtr), mockPublisherInterface);

    EXPECT_CALL(mockPublisherInterface, publishMock).Times(1);
    sut.publish();

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::MODERATE));
        });

    // ===== Test ===== //
    sut.publish();

    // ===== Verify ===== //
    ASSERT_TRUE(detectedError.has_value());
    ASSERT_THAT(detectedError.value(), Eq(iox::Error::kPOSH__PUBLISHING_EMPTY_SAMPLE));

    // ===== Cleanup ===== //
}

TEST_F(SampleTest, CallingGetUserHeaderFromNonConstTypeReturnsCorrectAddress)
{
    // ===== Setup ===== //
    ChunkMock<DummyData, DummyHeader> chunk;
    iox::cxx::unique_ptr<DummyData> testSamplePtr{chunk.sample(), [](DummyData*) {}};
    MockPublisherInterface<DummyData, DummyHeader> mockPublisherInterface{};

    auto sut = iox::popo::Sample<DummyData, DummyHeader>(std::move(testSamplePtr), mockPublisherInterface);

    // this line is actually not needed for the tests but if it is not present gmock raises a unused-function warning in
    // gmock-spec-builders.h which breaks the build with -Werror
    EXPECT_CALL(mockPublisherInterface, publishMock).Times(0);

    // ===== Test ===== //
    auto& userHeader = sut.getUserHeader();

    // ===== Verify ===== //
    ASSERT_EQ(&userHeader, chunk.userHeader());

    // ===== Cleanup ===== //
}

TEST_F(SampleTest, CallingGetUserHeaderFromConstTypeReturnsCorrectAddress)
{
    // ===== Setup ===== //
    ChunkMock<DummyData, DummyHeader> chunk;
    iox::cxx::unique_ptr<const DummyData> testSamplePtr{chunk.sample(), [](const DummyData*) {}};

    auto sut = iox::popo::Sample<const DummyData, const DummyHeader>(std::move(testSamplePtr));

    // ===== Test ===== //
    auto& userHeader = sut.getUserHeader();

    // ===== Verify ===== //
    ASSERT_EQ(&userHeader, chunk.userHeader());

    // ===== Cleanup ===== //
}
