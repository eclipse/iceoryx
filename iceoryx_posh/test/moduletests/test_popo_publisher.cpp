// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "mocks/publisher_mock.hpp"

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
    DummyData(uint64_t initialVal)
        : val(initialVal)
    {
    }
    // constexpr variable cannot be used instead in tests (linker optimization - no address)
    static constexpr uint64_t defaultVal()
    {
        return 42U;
    };
    uint64_t val{defaultVal()};
};
} // namespace

using TestPublisher = iox::popo::PublisherImpl<DummyData, iox::mepoo::NoUserHeader, MockBasePublisher<DummyData>>;

class PublisherTest : public Test
{
  public:
    PublisherTest()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

  protected:
    ChunkMock<DummyData> chunkMock;
    TestPublisher sut{{"", "", ""}};
    MockPublisherPortUser& portMock{sut.mockPort()};
};


TEST_F(PublisherTest, LoansChunkLargeEnoughForTheType)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loan();
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    EXPECT_CALL(portMock, releaseChunk(chunkMock.chunkHeader()));
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, LoanedSampleIsDefaultInitialized)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loan();
    // ===== Verify ===== //
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(result.value()->val, DummyData::defaultVal());
    EXPECT_CALL(portMock, releaseChunk(chunkMock.chunkHeader()));
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, LoanWithArgumentsCallsCustomCtor)
{
    constexpr uint64_t CUSTOM_VALUE{73};
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loan(CUSTOM_VALUE);
    // ===== Verify ===== //
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(result.value()->val, CUSTOM_VALUE);
    EXPECT_CALL(portMock, releaseChunk(chunkMock.chunkHeader()));
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfALambdaWithAdditionalArguments)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf(
        [](DummyData* allocation, int intVal) {
            auto data = new (allocation) DummyData();
            data->val = intVal;
        },
        42);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfALambdaWithNoAdditionalArguments)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf([](DummyData* allocation) {
        auto data = new (allocation) DummyData();
        data->val = 777;
    });
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfACallableStructWithNoAdditionalArguments)
{
    struct CallableStruct
    {
        void operator()(DummyData* allocation)
        {
            auto data = new (allocation) DummyData();
            data->val = 777;
        };
    };
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf(CallableStruct{});
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfACallableStructWithAdditionalArguments)
{
    struct CallableStruct
    {
        void operator()(DummyData* allocation, int, float)
        {
            auto data = new (allocation) DummyData();
            data->val = 777;
        };
    };
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf(CallableStruct{}, 42, 77.77);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

void freeFunctionNoAdditionalArgs(DummyData* allocation)
{
    auto data = new (allocation) DummyData();
    data->val = 777;
}
void freeFunctionWithAdditionalArgs(DummyData* allocation, int, float)
{
    auto data = new (allocation) DummyData();
    data->val = 777;
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfFunctionPointerWithNoAdditionalArguments)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf(freeFunctionNoAdditionalArgs);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishTheResultOfFunctionPointerWithAdditionalArguments)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    auto result = sut.publishResultOf(freeFunctionWithAdditionalArgs, 42, 77.77);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, CanLoanSamplesAndPublishCopiesOfProvidedValues)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    DummyData data(73);
    // ===== Test ===== //
    auto result = sut.publishCopyOf(data);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, LoanFailsAndForwardsAllocationErrorsToCaller)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(
            ByMove(iox::cxx::error<iox::popo::AllocationError>(iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS))));
    // ===== Test ===== //
    auto result = sut.loan();
    // ===== Verify ===== //
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS, result.get_error());
    // ===== Cleanup ===== //
}


TEST_F(PublisherTest, LoanedSamplesContainPointerToChunkHeader)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loan();
    // ===== Verify ===== //
    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(chunkMock.chunkHeader(), result.value().getChunkHeader());
    EXPECT_CALL(portMock, releaseChunk(chunkMock.chunkHeader()));
    // ===== Cleanup ===== //
}

TEST_F(PublisherTest, PublishingSendsUnderlyingMemoryChunkOnPublisherPort)
{
    EXPECT_CALL(portMock, tryAllocateChunk(sizeof(DummyData), _, _, _))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    EXPECT_CALL(portMock, sendChunk(chunkMock.chunkHeader()));
    // ===== Test ===== //
    sut.loan().and_then([](auto& sample) { sample.publish(); });
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

// test whether the BasePublisher methods are called

TEST_F(PublisherTest, OfferDoesOfferServiceOnUnderlyingPort)
{
    EXPECT_CALL(sut, offer).Times(1);
    // ===== Test ===== //
    sut.offer();
}
TEST_F(PublisherTest, StopOfferDoesStopOfferServiceOnUnderlyingPort)
{
    EXPECT_CALL(sut, stopOffer).Times(1);
    sut.stopOffer();
}

TEST_F(PublisherTest, isOfferedDoesCheckIfPortIsOfferedOnUnderlyingPort)
{
    EXPECT_CALL(sut, isOffered).Times(1);
    // ===== Test ===== //
    sut.isOffered();
}

TEST_F(PublisherTest, isOfferedDoesCheckIfUnderylingPortHasSubscribers)
{
    EXPECT_CALL(sut, hasSubscribers).Times(1);
    // ===== Test ===== //
    sut.hasSubscribers();
}

TEST_F(PublisherTest, GetServiceDescriptionCallForwardedToUnderlyingPublisherPort)
{
    EXPECT_CALL(sut, getServiceDescription).Times(1);
    // ===== Test ===== //
    sut.getServiceDescription();
}
