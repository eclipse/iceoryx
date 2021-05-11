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

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_distributor_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/variant_queue.hpp"
#include "test.hpp"

#include <memory>

using namespace ::testing;
using ::testing::Return;
using namespace iox::popo;
using namespace iox::cxx;
using namespace iox::mepoo;

using ChunkDistributorTestSubjects = Types<ThreadSafePolicy, SingleThreadedPolicy>;
/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(ChunkDistributor_test, ChunkDistributorTestSubjects);
#pragma GCC diagnostic pop

template <typename PolicyType>
class ChunkDistributor_test : public Test
{
  public:
    SharedChunk allocateChunk(uint32_t value)
    {
        ChunkManagement* chunkMgmt = static_cast<ChunkManagement*>(chunkMgmtPool.getChunk());
        auto chunk = mempool.getChunk();

        auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
        EXPECT_FALSE(chunkSettingsResult.has_error());
        if (chunkSettingsResult.has_error())
        {
            return nullptr;
        }
        auto& chunkSettings = chunkSettingsResult.value();

        ChunkHeader* chunkHeader = new (chunk) ChunkHeader(mempool.getChunkSize(), chunkSettings);
        new (chunkMgmt) ChunkManagement{chunkHeader, &mempool, &chunkMgmtPool};
        *static_cast<uint32_t*>(chunkHeader->userPayload()) = value;
        return SharedChunk(chunkMgmt);
    }
    uint32_t getSharedChunkValue(const SharedChunk& chunk)
    {
        return *static_cast<uint32_t*>(chunk.getUserPayload());
    }

    static constexpr uint32_t USER_PAYLOAD_SIZE{128U};
    static constexpr size_t MEGABYTE = 1U << 20U;
    static constexpr size_t MEMORY_SIZE = 1U * MEGABYTE;
    const uint64_t HISTORY_SIZE = 16U;
    static constexpr uint32_t MAX_NUMBER_QUEUES = 128U;
    char memory[MEMORY_SIZE];
    iox::posix::Allocator allocator{memory, MEMORY_SIZE};
    MemPool mempool{sizeof(ChunkHeader) + USER_PAYLOAD_SIZE, 20U, allocator, allocator};
    MemPool chunkMgmtPool{128U, 20U, allocator, allocator};

    struct ChunkDistributorConfig
    {
        static constexpr uint32_t MAX_QUEUES = MAX_NUMBER_QUEUES;
        static constexpr uint64_t MAX_HISTORY_CAPACITY = iox::MAX_PUBLISHER_HISTORY;
    };

    struct ChunkQueueConfig
    {
        static constexpr uint64_t MAX_QUEUE_CAPACITY = MAX_NUMBER_QUEUES;
    };

    using ChunkQueueData_t = ChunkQueueData<ChunkQueueConfig, PolicyType>;
    using ChunkDistributorData_t =
        ChunkDistributorData<ChunkDistributorConfig, PolicyType, ChunkQueuePusher<ChunkQueueData_t>>;
    using ChunkDistributor_t = ChunkDistributor<ChunkDistributorData_t>;

    void SetUp(){};
    void TearDown(){};

    std::shared_ptr<ChunkQueueData_t>
    getChunkQueueData(const QueueFullPolicy policy = QueueFullPolicy::DISCARD_OLDEST_DATA,
                      const VariantQueueTypes queueType = VariantQueueTypes::SoFi_SingleProducerSingleConsumer)
    {
        return std::make_shared<ChunkQueueData_t>(policy, queueType);
    }

    std::shared_ptr<ChunkDistributorData_t>
    getChunkDistributorData(const SubscriberTooSlowPolicy policy = SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA)
    {
        return std::make_shared<ChunkDistributorData_t>(policy, HISTORY_SIZE);
    }

    static constexpr int64_t TIMEOUT_IN_MS = 100;
};
template <typename PolicyType>
constexpr int64_t ChunkDistributor_test<PolicyType>::TIMEOUT_IN_MS;

TYPED_TEST(ChunkDistributor_test, AddingNullptrQueueDoesNotWork)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    EXPECT_DEATH(IOX_DISCARD_RESULT(sut.tryAddQueue(nullptr)), ".*");
}

TYPED_TEST(ChunkDistributor_test, NewChunkDistributorHasNoQueues)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    EXPECT_THAT(sut.hasStoredQueues(), Eq(false));
}

TYPED_TEST(ChunkDistributor_test, AfterAddingQueueChunkDistributorHasQueues)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    auto ret = sut.tryAddQueue(queueData.get());
    EXPECT_FALSE(ret.has_error());
    EXPECT_THAT(sut.hasStoredQueues(), Eq(true));
}

TYPED_TEST(ChunkDistributor_test, QueueOverflow)
{
    std::vector<std::shared_ptr<typename TestFixture::ChunkQueueData_t>> queueVector;
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto errorHandlerCalled{false};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
        });

    for (uint32_t i = 0; i < this->MAX_NUMBER_QUEUES; ++i)
    {
        auto queueData = this->getChunkQueueData();
        ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
        queueVector.push_back(queueData);
    }

    EXPECT_FALSE(errorHandlerCalled);

    auto queueData = this->getChunkQueueData();
    auto ret = sut.tryAddQueue(queueData.get());
    EXPECT_TRUE(ret.has_error());
    EXPECT_THAT(ret.get_error(), Eq(iox::popo::ChunkDistributorError::QUEUE_CONTAINER_OVERFLOW));
    EXPECT_TRUE(errorHandlerCalled);
}

TYPED_TEST(ChunkDistributor_test, RemovingExistingQueueWorks)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
    auto ret = sut.tryRemoveQueue(queueData.get());
    EXPECT_FALSE(ret.has_error());
    EXPECT_THAT(sut.hasStoredQueues(), Eq(false));
}

TYPED_TEST(ChunkDistributor_test, RemovingNonExistingQueueChangesNothing)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto queueData2 = this->getChunkQueueData();
    auto ret = sut.tryRemoveQueue(queueData2.get());
    EXPECT_TRUE(ret.has_error());
    EXPECT_THAT(ret.get_error(), Eq(iox::popo::ChunkDistributorError::QUEUE_NOT_IN_CONTAINER));
    EXPECT_THAT(sut.hasStoredQueues(), Eq(true));
}

TYPED_TEST(ChunkDistributor_test, RemoveAllQueuesWhenContainingOne)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
    sut.removeAllQueues();

    EXPECT_THAT(sut.hasStoredQueues(), Eq(false));
}

TYPED_TEST(ChunkDistributor_test, RemoveAllQueuesWhenContainingMultipleQueues)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
    auto queueData2 = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData2.get()).has_error());
    sut.removeAllQueues();

    EXPECT_THAT(sut.hasStoredQueues(), Eq(false));
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithOneQueue)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto chunk = this->allocateChunk(4451);
    sut.deliverToAllStoredQueues(chunk);

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    auto maybeSharedChunk = queue.tryPop();

    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(4451u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithOneQueueDeliversOneChunk)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto chunk = this->allocateChunk(4451);
    sut.deliverToAllStoredQueues(chunk);

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    EXPECT_THAT(queue.size(), Eq(1u));
    EXPECT_THAT(sut.getHistorySize(), Eq(1u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithDuplicatedQueueDeliversOneChunk)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto chunk = this->allocateChunk(4451);
    sut.deliverToAllStoredQueues(chunk);

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    EXPECT_THAT(queue.size(), Eq(1u));
    EXPECT_THAT(sut.getHistorySize(), Eq(1u));
}


TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithOneQueueMultipleChunks)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto limit = 10;
    for (auto i = 0; i < limit; ++i)
        sut.deliverToAllStoredQueues(this->allocateChunk(i * 123));

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    for (auto i = 0; i < limit; ++i)
    {
        auto maybeSharedChunk = queue.tryPop();

        ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
        EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(i * 123u));
    }
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithOneQueueDeliverMultipleChunks)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto limit = 10u;
    for (auto i = 0u; i < limit; ++i)
        sut.deliverToAllStoredQueues(this->allocateChunk(i * 123));

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    EXPECT_THAT(queue.size(), Eq(limit));
    EXPECT_THAT(sut.getHistorySize(), Eq(limit));
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithMultipleQueues)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto limit = 10;
    std::vector<std::shared_ptr<typename TestFixture::ChunkQueueData_t>> queueData;
    for (auto i = 0; i < limit; ++i)
    {
        queueData.emplace_back(this->getChunkQueueData());
        ASSERT_FALSE(sut.tryAddQueue(queueData.back().get()).has_error());
    }

    auto chunk = this->allocateChunk(24451);
    sut.deliverToAllStoredQueues(chunk);

    for (auto i = 0; i < limit; ++i)
    {
        ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData[i].get());
        auto maybeSharedChunk = queue.tryPop();
        ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
        EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(24451u));
    }
    EXPECT_THAT(sut.getHistorySize(), Eq(1u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToAllStoredQueuesWithMultipleQueuesMultipleChunks)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto limit = 10u;
    std::vector<std::shared_ptr<typename TestFixture::ChunkQueueData_t>> queueData;
    for (auto i = 0u; i < limit; ++i)
    {
        queueData.emplace_back(this->getChunkQueueData());
        ASSERT_FALSE(sut.tryAddQueue(queueData.back().get()).has_error());
    }

    for (auto i = 0u; i < limit; ++i)
        sut.deliverToAllStoredQueues(this->allocateChunk(i * 34));

    for (auto i = 0u; i < limit; ++i)
    {
        for (auto k = 0u; k < limit; ++k)
        {
            ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData[i].get());
            auto maybeSharedChunk = queue.tryPop();
            ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
            EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(k * 34u));
        }
    }
    EXPECT_THAT(sut.getHistorySize(), Eq(limit));
}

TYPED_TEST(ChunkDistributor_test, AddToHistoryWithoutQueues)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());
    auto limit = 8u;
    for (auto i = 0u; i < limit; ++i)
        sut.deliverToAllStoredQueues(this->allocateChunk(34));

    EXPECT_THAT(sut.getHistorySize(), Eq(limit));
}

TYPED_TEST(ChunkDistributor_test, HistoryEmptyWhenCreated)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());
    EXPECT_THAT(sut.getHistorySize(), Eq(0u));
}

TYPED_TEST(ChunkDistributor_test, HistoryEmptyAfterClear)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());
    auto limit = 8;
    for (auto i = 0; i < limit; ++i)
        sut.deliverToAllStoredQueues(this->allocateChunk(34));
    sut.clearHistory();

    EXPECT_THAT(sut.getHistorySize(), Eq(0u));
}

TYPED_TEST(ChunkDistributor_test, addToHistoryWithoutDelivery)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());
    auto limit = 7u;
    for (auto i = 0u; i < limit; ++i)
        sut.addToHistoryWithoutDelivery(this->allocateChunk(34));

    EXPECT_THAT(sut.getHistorySize(), Eq(limit));
}

TYPED_TEST(ChunkDistributor_test, DeliverToQueueDirectlyWhenNotAdded)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();

    auto chunk = this->allocateChunk(4451);
    sut.deliverToQueue(queueData.get(), chunk);

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    auto maybeSharedChunk = queue.tryPop();

    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(4451u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToQueueDirectlyWhenAdded)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto chunk = this->allocateChunk(451);
    sut.deliverToQueue(queueData.get(), chunk);

    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    auto maybeSharedChunk = queue.tryPop();

    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(451u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToQueueDirectlyWhenNotAddedDoesNotChangeHistory)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();

    auto chunk = this->allocateChunk(4451);
    sut.deliverToQueue(queueData.get(), chunk);

    EXPECT_THAT(sut.getHistorySize(), Eq(0u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToQueueDirectlyWhenAddedDoesNotChangeHistory)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData = this->getChunkQueueData();
    ASSERT_FALSE(sut.tryAddQueue(queueData.get()).has_error());

    auto chunk = this->allocateChunk(4451);
    sut.deliverToQueue(queueData.get(), chunk);

    EXPECT_THAT(sut.getHistorySize(), Eq(0u));
}

TYPED_TEST(ChunkDistributor_test, DeliverHistoryOnAddWithLessThanAvailable)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    sut.deliverToAllStoredQueues(this->allocateChunk(1));
    sut.deliverToAllStoredQueues(this->allocateChunk(2));
    sut.deliverToAllStoredQueues(this->allocateChunk(3));

    EXPECT_THAT(sut.getHistorySize(), Eq(3u));

    // add a queue with a requested history of one must deliver the latest sample
    auto queueData = this->getChunkQueueData();
    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    ASSERT_FALSE(sut.tryAddQueue(queueData.get(), 1).has_error());

    EXPECT_THAT(queue.size(), Eq(1u));
    auto maybeSharedChunk = queue.tryPop();

    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(3u));
}

TYPED_TEST(ChunkDistributor_test, DeliverHistoryOnAddWithExactAvailable)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    sut.deliverToAllStoredQueues(this->allocateChunk(1));
    sut.deliverToAllStoredQueues(this->allocateChunk(2));
    sut.deliverToAllStoredQueues(this->allocateChunk(3));

    EXPECT_THAT(sut.getHistorySize(), Eq(3u));

    // add a queue with a requested history of 3 must deliver all three in the order oldest to newest
    auto queueData = this->getChunkQueueData();
    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    ASSERT_FALSE(sut.tryAddQueue(queueData.get(), 3).has_error());

    EXPECT_THAT(queue.size(), Eq(3u));
    auto maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(1u));
    maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(2u));
    maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(3u));
}

TYPED_TEST(ChunkDistributor_test, DeliverHistoryOnAddWithMoreThanAvailable)
{
    auto sutData = this->getChunkDistributorData();
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    sut.deliverToAllStoredQueues(this->allocateChunk(1));
    sut.deliverToAllStoredQueues(this->allocateChunk(2));
    sut.deliverToAllStoredQueues(this->allocateChunk(3));

    EXPECT_THAT(sut.getHistorySize(), Eq(3u));

    // add a queue with a requested history of 5 must deliver only the three available in the order oldest to newest
    auto queueData = this->getChunkQueueData();
    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    ASSERT_FALSE(sut.tryAddQueue(queueData.get(), 5).has_error());

    EXPECT_THAT(queue.size(), Eq(3u));
    auto maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(1u));
    maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(2u));
    maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(3u));
}

TYPED_TEST(ChunkDistributor_test, DeliverToSingleQueueBlocksWhenOptionsAreSetToBlocking)
{
    auto sutData = this->getChunkDistributorData(SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER);
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    auto queueData =
        this->getChunkQueueData(QueueFullPolicy::BLOCK_PUBLISHER, VariantQueueTypes::FiFo_MultiProducerSingleConsumer);
    ChunkQueuePopper<typename TestFixture::ChunkQueueData_t> queue(queueData.get());
    queue.setCapacity(1U);

    ASSERT_FALSE(sut.tryAddQueue(queueData.get(), 0U).has_error());
    sut.deliverToAllStoredQueues(this->allocateChunk(155U));

    auto threadSyncSemaphore = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    std::atomic_bool wasChunkDelivered{false};
    std::thread t1([&] {
        ASSERT_FALSE(threadSyncSemaphore->post().has_error());
        sut.deliverToAllStoredQueues(this->allocateChunk(152U));
        wasChunkDelivered = true;
    });

    ASSERT_FALSE(threadSyncSemaphore->wait().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(this->TIMEOUT_IN_MS));
    EXPECT_THAT(wasChunkDelivered.load(), Eq(false));

    auto maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(155U));

    t1.join(); // join needs to be before the load to ensure the wasChunkDelivered store happens before the read
    EXPECT_THAT(wasChunkDelivered.load(), Eq(true));

    maybeSharedChunk = queue.tryPop();
    ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
    EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(152U));
}

TYPED_TEST(ChunkDistributor_test, MultipleBlockingQueuesWillBeFilledWhenThereBecomesSpaceAvailable)
{
    auto sutData = this->getChunkDistributorData(SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER);
    typename TestFixture::ChunkDistributor_t sut(sutData.get());

    std::vector<std::shared_ptr<typename TestFixture::ChunkQueueData_t>> queueDatas;
    std::vector<ChunkQueuePopper<typename TestFixture::ChunkQueueData_t>> queues;

    constexpr uint64_t NUMBER_OF_QUEUES = 4U;

    for (uint64_t i = 0U; i < NUMBER_OF_QUEUES; ++i)
    {
        queueDatas.emplace_back(this->getChunkQueueData(QueueFullPolicy::BLOCK_PUBLISHER,
                                                        VariantQueueTypes::FiFo_MultiProducerSingleConsumer));
        queues.emplace_back(queueDatas.back().get());
        queues.back().setCapacity(1U);
        ASSERT_FALSE(sut.tryAddQueue(queueDatas.back().get(), 0U).has_error());
    }

    sut.deliverToAllStoredQueues(this->allocateChunk(425U));

    auto threadSyncSemaphore = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    std::atomic_bool wasChunkDelivered{false};
    std::thread t1([&] {
        ASSERT_FALSE(threadSyncSemaphore->post().has_error());
        sut.deliverToAllStoredQueues(this->allocateChunk(1152U));
        wasChunkDelivered = true;
    });

    ASSERT_FALSE(threadSyncSemaphore->wait().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(this->TIMEOUT_IN_MS));
    EXPECT_THAT(wasChunkDelivered.load(), Eq(false));

    for (uint64_t i = 0U; i < NUMBER_OF_QUEUES; ++i)
    {
        auto maybeSharedChunk = queues[i].tryPop();
        ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
        EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(425U));

        if (i + 1U == NUMBER_OF_QUEUES)
        {
            // join needs to be before the load to ensure the wasChunkDelivered store happens before the read
            t1.join();
            EXPECT_THAT(wasChunkDelivered.load(), Eq(true));
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->TIMEOUT_IN_MS));
            EXPECT_THAT(wasChunkDelivered.load(), Eq(false));
        }

        maybeSharedChunk = queues[i].tryPop();
        ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
        EXPECT_THAT(this->getSharedChunkValue(*maybeSharedChunk), Eq(1152U));
    }
}
