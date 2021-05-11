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

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/mepoo/typed_mem_pool.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_pusher.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_listener.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

using namespace iox::popo;
using namespace iox::mepoo;
using namespace iox::units::duration_literals;

class ChunkQueue_testBase
{
  public:
    SharedChunk allocateChunk()
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
        return SharedChunk(chunkMgmt);
    }

    static constexpr uint32_t USER_PAYLOAD_SIZE{128U};
    static constexpr size_t MEGABYTE = 1U << 20U;
    static constexpr size_t MEMORY_SIZE = 4U * MEGABYTE;
    std::unique_ptr<char[]> memory{new char[MEMORY_SIZE]};
    iox::posix::Allocator allocator{memory.get(), MEMORY_SIZE};
    MemPool mempool{
        sizeof(ChunkHeader) + USER_PAYLOAD_SIZE, 2U * iox::MAX_SUBSCRIBER_QUEUE_CAPACITY, allocator, allocator};
    MemPool chunkMgmtPool{128U, 2U * iox::MAX_SUBSCRIBER_QUEUE_CAPACITY, allocator, allocator};

    static constexpr uint32_t RESIZED_CAPACITY{5U};
};

template <typename PolicyType, iox::cxx::VariantQueueTypes VariantQueueType>
struct TypeDefinitions
{
    using PolicyType_t = PolicyType;
    static const iox::cxx::VariantQueueTypes variantQueueType{VariantQueueType};
};

using ChunkQueueSubjects =
    Types<TypeDefinitions<ThreadSafePolicy, iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer>,
          TypeDefinitions<ThreadSafePolicy, iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer>,
          TypeDefinitions<SingleThreadedPolicy, iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer>,
          TypeDefinitions<SingleThreadedPolicy, iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer>>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(ChunkQueue_test, ChunkQueueSubjects);
#pragma GCC diagnostic pop

template <typename TestTypes>
class ChunkQueue_test : public Test, public ChunkQueue_testBase
{
  public:
    void SetUp() override{};
    void TearDown() override{};

    using ChunkQueueData_t = ChunkQueueData<iox::DefaultChunkQueueConfig, typename TestTypes::PolicyType_t>;

    iox::cxx::VariantQueueTypes m_variantQueueType{TestTypes::variantQueueType};
    ChunkQueueData_t m_chunkData{QueueFullPolicy::DISCARD_OLDEST_DATA, m_variantQueueType};
    ChunkQueuePopper<ChunkQueueData_t> m_popper{&m_chunkData};
    ChunkQueuePusher<ChunkQueueData_t> m_pusher{&m_chunkData};
};

TYPED_TEST(ChunkQueue_test, InitialEmpty)
{
    EXPECT_THAT(this->m_popper.empty(), Eq(true));
}

TYPED_TEST(ChunkQueue_test, InitialConditionVariableAttached)
{
    EXPECT_THAT(this->m_popper.isConditionVariableSet(), Eq(false));
}

TYPED_TEST(ChunkQueue_test, PushOneChunk)
{
    auto chunk = this->allocateChunk();
    this->m_pusher.push(chunk);
    EXPECT_THAT(this->m_popper.empty(), Eq(false));
    /// @note size not implemented on FIFO
    if (this->m_variantQueueType != iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer)
    {
        EXPECT_THAT(this->m_popper.size(), Eq(1U));
    }
}

TYPED_TEST(ChunkQueue_test, PopOneChunk)
{
    auto chunk = this->allocateChunk();
    this->m_pusher.push(chunk);

    EXPECT_THAT(this->m_popper.tryPop().has_value(), Eq(true));
    EXPECT_THAT(this->m_popper.empty(), Eq(true));
    /// @note size not implemented on FIFO
    if (this->m_variantQueueType != iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer)
    {
        EXPECT_THAT(this->m_popper.size(), Eq(0U));
    }
}

TYPED_TEST(ChunkQueue_test, PushedChunksMustBePoppedInTheSameOrder)
{
    constexpr int32_t NUMBER_CHUNKS{5};
    for (int i = 0; i < NUMBER_CHUNKS; ++i)
    {
        auto chunk = this->allocateChunk();
        *reinterpret_cast<int32_t*>(chunk.getUserPayload()) = i;
        this->m_pusher.push(chunk);
    }

    for (int i = 0; i < NUMBER_CHUNKS; ++i)
    {
        auto maybeSharedChunk = this->m_popper.tryPop();
        ASSERT_THAT(maybeSharedChunk.has_value(), Eq(true));
        auto data = *reinterpret_cast<int32_t*>((*maybeSharedChunk).getUserPayload());
        EXPECT_THAT(data, Eq(i));
    }
}

TYPED_TEST(ChunkQueue_test, PopChunkWithIncompatibleChunkHeaderCallsErrorHandler)
{
    auto chunk = this->allocateChunk();
    // this is currently the only possibility to test an invalid CHUNK_HEADER_VERSION
    auto chunkHeaderAddress = reinterpret_cast<uint64_t>(chunk.getChunkHeader());
    auto chunkHeaderVersionAddress = chunkHeaderAddress + sizeof(uint32_t);
    auto chunkHeaderVersionPointer = reinterpret_cast<uint8_t*>(chunkHeaderVersionAddress);
    *chunkHeaderVersionPointer = std::numeric_limits<uint8_t>::max();

    this->m_pusher.push(chunk);

    iox::Error receivedError{iox::Error::kNO_ERROR};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            receivedError = error;
            EXPECT_EQ(errorLevel, iox::ErrorLevel::SEVERE);
        });

    EXPECT_FALSE(this->m_popper.tryPop().has_value());
    EXPECT_EQ(receivedError, iox::Error::kPOPO__CHUNK_QUEUE_POPPER_CHUNK_WITH_INCOMPATIBLE_CHUNK_HEADER_VERSION);
}

TYPED_TEST(ChunkQueue_test, ClearOnEmpty)
{
    this->m_popper.clear();
    EXPECT_THAT(this->m_popper.empty(), Eq(true));
}

TYPED_TEST(ChunkQueue_test, ClearWithData)
{
    auto chunk = this->allocateChunk();
    this->m_pusher.push(chunk);
    this->m_popper.clear();
    EXPECT_THAT(this->m_popper.empty(), Eq(true));
}

TYPED_TEST(ChunkQueue_test, AttachConditionVariable)
{
    ConditionVariableData condVar("Horscht");

    this->m_popper.setConditionVariable(condVar, 0U);

    EXPECT_THAT(this->m_popper.isConditionVariableSet(), Eq(true));
}

TYPED_TEST(ChunkQueue_test, PushAndNotifyConditionVariable)
{
    ConditionVariableData condVar("Horscht");
    ConditionListener condVarWaiter{condVar};

    this->m_popper.setConditionVariable(condVar, 0U);

    auto chunk = this->allocateChunk();
    this->m_pusher.push(chunk);

    EXPECT_THAT(condVarWaiter.timedWait(1_ns).empty(), Eq(false));
    EXPECT_THAT(condVarWaiter.timedWait(1_ns).empty(), Eq(true)); // shouldn't trigger a second time
}

TYPED_TEST(ChunkQueue_test, AttachSecondConditionVariable)
{
    ConditionVariableData condVar1("Horscht");
    ConditionVariableData condVar2("Schnuppi");
    ConditionListener condVarWaiter1{condVar1};
    ConditionListener condVarWaiter2{condVar2};

    this->m_popper.setConditionVariable(condVar1, 0U);
    this->m_popper.setConditionVariable(condVar2, 1U);

    EXPECT_THAT(condVarWaiter1.timedWait(1_ns).empty(), Eq(true));
    EXPECT_THAT(condVarWaiter2.timedWait(1_ns).empty(), Eq(true));

    auto chunk = this->allocateChunk();
    this->m_pusher.push(chunk);

    EXPECT_THAT(condVarWaiter1.timedWait(1_ms).empty(), Eq(true));
    EXPECT_THAT(condVarWaiter2.timedWait(1_ms).empty(), Eq(false));
}

/// @note this could be changed to a parameterized ChunkQueueSaturatingFIFO_test when there are more FIFOs available
using ChunkQueueFiFoTestSubjects = Types<ThreadSafePolicy, SingleThreadedPolicy>;
/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(ChunkQueueFiFo_test, ChunkQueueFiFoTestSubjects);
#pragma GCC diagnostic pop

template <typename PolicyType>
class ChunkQueueFiFo_test : public Test, public ChunkQueue_testBase
{
  public:
    void SetUp() override{};
    void TearDown() override{};

    using ChunkQueueData_t = ChunkQueueData<iox::DefaultChunkQueueConfig, PolicyType>;

    ChunkQueueData_t m_chunkData{QueueFullPolicy::DISCARD_OLDEST_DATA,
                                 iox::cxx::VariantQueueTypes::FiFo_SingleProducerSingleConsumer};
    ChunkQueuePopper<ChunkQueueData_t> m_popper{&m_chunkData};
    ChunkQueuePusher<ChunkQueueData_t> m_pusher{&m_chunkData};
};

TYPED_TEST(ChunkQueueFiFo_test, InitialSize)
{
    EXPECT_THAT(this->m_popper.size(), Eq(0U));
}

TYPED_TEST(ChunkQueueFiFo_test, Capacity)
{
    EXPECT_THAT(this->m_popper.getCurrentCapacity(), Eq(iox::MAX_SUBSCRIBER_QUEUE_CAPACITY));
}

/// @note API currently not supported
TYPED_TEST(ChunkQueueFiFo_test, DISABLED_SetCapacity)
{
    this->m_popper.setCapacity(this->RESIZED_CAPACITY);
    EXPECT_THAT(this->m_popper.getCurrentCapacity(), Eq(this->RESIZED_CAPACITY));
}

TYPED_TEST(ChunkQueueFiFo_test, PushFull)
{
    for (auto i = 0U; i < iox::MAX_SUBSCRIBER_QUEUE_CAPACITY; ++i)
    {
        auto chunk = this->allocateChunk();
        EXPECT_TRUE(this->m_pusher.push(chunk));
    }

    {
        auto chunk = this->allocateChunk();
        EXPECT_FALSE(this->m_pusher.push(chunk));
    }

    // get all the chunks in the queue
    while (this->m_popper.tryPop().has_value())
    {
    }

    // all chunks must be released
    EXPECT_THAT(this->mempool.getUsedChunks(), Eq(0U));
}

/// @note this could be changed to a parameterized ChunkQueueOverflowingFIFO_test when there are more FIFOs available
using ChunkQueueSoFiSubjects = Types<ThreadSafePolicy, SingleThreadedPolicy>;
/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(ChunkQueueSoFi_test, ChunkQueueSoFiSubjects);
#pragma GCC diagnostic pop

template <typename PolicyType>
class ChunkQueueSoFi_test : public Test, public ChunkQueue_testBase
{
  public:
    void SetUp() override{};
    void TearDown() override{};

    using ChunkQueueData_t = ChunkQueueData<iox::DefaultChunkQueueConfig, PolicyType>;

    ChunkQueueData_t m_chunkData{QueueFullPolicy::DISCARD_OLDEST_DATA,
                                 iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    ChunkQueuePopper<ChunkQueueData_t> m_popper{&m_chunkData};
    ChunkQueuePusher<ChunkQueueData_t> m_pusher{&m_chunkData};
};

TYPED_TEST(ChunkQueueSoFi_test, InitialSize)
{
    EXPECT_THAT(this->m_popper.size(), Eq(0U));
}

TYPED_TEST(ChunkQueueSoFi_test, Capacity)
{
    EXPECT_THAT(this->m_popper.getCurrentCapacity(), Eq(iox::MAX_SUBSCRIBER_QUEUE_CAPACITY));
}


TYPED_TEST(ChunkQueueSoFi_test, SetCapacity)
{
    this->m_popper.setCapacity(this->RESIZED_CAPACITY);
    EXPECT_THAT(this->m_popper.getCurrentCapacity(), Eq(this->RESIZED_CAPACITY));
}

TYPED_TEST(ChunkQueueSoFi_test, PushFull)
{
    for (auto i = 0u; i < iox::MAX_SUBSCRIBER_QUEUE_CAPACITY; ++i)
    {
        auto chunk = this->allocateChunk();
        EXPECT_TRUE(this->m_pusher.push(chunk));
    }

    for (auto i = 0U; i < iox::MAX_SUBSCRIBER_QUEUE_CAPACITY; ++i)
    {
        auto chunk = this->allocateChunk();
        EXPECT_FALSE(this->m_pusher.push(chunk));
    }

    // get all the chunks in the queue
    while (this->m_popper.tryPop().has_value())
    {
    }

    // all chunks must be released
    EXPECT_THAT(this->mempool.getUsedChunks(), Eq(0U));
}


TYPED_TEST(ChunkQueueSoFi_test, InitialNoLostChunks)
{
    EXPECT_FALSE(this->m_popper.hasLostChunks());
}

TYPED_TEST(ChunkQueueSoFi_test, IndicateALostChunk)
{
    this->m_pusher.lostAChunk();

    EXPECT_TRUE(this->m_popper.hasLostChunks());
}

TYPED_TEST(ChunkQueueSoFi_test, LostChunkInfoIsResetAfterRead)
{
    this->m_pusher.lostAChunk();
    this->m_popper.hasLostChunks();

    EXPECT_FALSE(this->m_popper.hasLostChunks());
}
