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

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "mocks/subscriber_mock.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::_;

// anonymous namespace to prevent linker issues or sanitizer false positives
// if a struct with the same name is used in other tests
namespace
{
struct DummyData
{
    uint64_t val = 42;
};
} // namespace

template <typename T, typename H, typename BaseSubscriber>
class StubbedSubscriber : public iox::popo::SubscriberImpl<T, H, BaseSubscriber>
{
  public:
    using SubscriberParent = iox::popo::SubscriberImpl<T, H, BaseSubscriber>;

    StubbedSubscriber(const iox::capro::ServiceDescription& service,
                      const iox::popo::SubscriberOptions& subscriberOptions = iox::popo::SubscriberOptions())
        : SubscriberParent(service, subscriberOptions)
    {
    }

    using SubscriberParent::port;
};

using TestSubscriber = StubbedSubscriber<DummyData, iox::mepoo::NoUserHeader, MockBaseSubscriber<DummyData>>;

class SubscriberTest : public Test
{
  public:
    SubscriberTest()
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
    TestSubscriber sut{{"", "", ""}, iox::popo::SubscriberOptions()};
};

TEST_F(SubscriberTest, GetsUIDViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, getUid).Times(1);
    // ===== Test ===== //
    sut.getUid();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, GetsServiceDescriptionViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, getServiceDescription).Times(1);
    // ===== Test ===== //
    sut.getServiceDescription();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, GetsSubscriptionStateViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, getSubscriptionState).Times(1);
    // ===== Test ===== //
    sut.getSubscriptionState();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, SubscribesViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, subscribe).Times(1);
    // ===== Test ===== //
    sut.subscribe(1);
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, UnsubscribesViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, unsubscribe).Times(1);
    // ===== Test ===== //
    sut.unsubscribe();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, ChecksForNewSamplesViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, hasData).Times(1);
    // ===== Test ===== //
    sut.hasData();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, ChecksForMissedSamplesViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, hasMissedData).Times(1);
    // ===== Test ===== //
    sut.hasMissedData();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, TakeReturnsAllocatedMemoryChunksWrappedInSample)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, takeChunk)
        .Times(1)
        .WillOnce(Return(ByMove(iox::cxx::success<const iox::mepoo::ChunkHeader*>(
            const_cast<const iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader())))));
    // ===== Test ===== //
    auto maybeSample = sut.take();
    // ===== Verify ===== //
    ASSERT_FALSE(maybeSample.has_error());
    EXPECT_EQ(maybeSample.value().get(), chunkMock.chunkHeader()->userPayload());
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, ReceivedSamplesAreAutomaticallyDeletedWhenOutOfScope)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, takeChunk)
        .Times(1)
        .WillOnce(Return(ByMove(iox::cxx::success<const iox::mepoo::ChunkHeader*>(
            const_cast<const iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader())))));
    EXPECT_CALL(sut.port(), releaseChunk).Times(AtLeast(1));
    // ===== Test ===== //
    {
        EXPECT_FALSE(sut.take().has_error());
    }
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

TEST_F(SubscriberTest, ReleasesQueuedDataViaBaseSubscriber)
{
    // ===== Setup ===== //
    EXPECT_CALL(sut, releaseQueuedData).Times(1);
    // ===== Test ===== //
    sut.releaseQueuedData();
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}
