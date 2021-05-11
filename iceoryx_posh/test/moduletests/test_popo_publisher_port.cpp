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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_roudi.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "test.hpp"

#include <memory>

using namespace ::testing;

struct DummySample
{
    uint64_t dummy{42};
};

class PublisherPort_test : public Test
{
  protected:
    PublisherPort_test()
    {
        m_mempoolconf.addMemPool({SMALL_CHUNK, NUM_CHUNKS_IN_POOL});
        m_mempoolconf.addMemPool({BIG_CHUNK, NUM_CHUNKS_IN_POOL});
        m_memoryManager.configureMemoryManager(m_mempoolconf, m_memoryAllocator, m_memoryAllocator);
    }

    ~PublisherPort_test()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    static constexpr size_t MEMORY_SIZE = 1024 * 1024;
    uint8_t m_memory[MEMORY_SIZE];
    static constexpr uint32_t NUM_CHUNKS_IN_POOL = 20;
    static constexpr uint32_t SMALL_CHUNK = 128;
    static constexpr uint32_t BIG_CHUNK = 256;

    static constexpr uint32_t USER_PAYLOAD_ALIGNMENT = iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT;
    static constexpr uint32_t USER_HEADER_SIZE = iox::CHUNK_NO_USER_HEADER_SIZE;
    static constexpr uint32_t USER_HEADER_ALIGNMENT = iox::CHUNK_NO_USER_HEADER_ALIGNMENT;

    using ChunkQueueData_t = iox::popo::ChunkQueueData<iox::DefaultChunkQueueConfig, iox::popo::ThreadSafePolicy>;

    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};

    iox::posix::Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    iox::mepoo::MePooConfig m_mempoolconf;
    iox::mepoo::MemoryManager m_memoryManager;


    // publisher port w/o offer on create
    iox::popo::PublisherOptions m_noOfferOnCreatePublisherOptions{
        0U, iox::NodeName_t{""}, false, iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA};
    iox::popo::PublisherPortData m_publisherPortDataNoOfferOnCreate{
        iox::capro::ServiceDescription("a", "b", "c"), "myApp", &m_memoryManager, m_noOfferOnCreatePublisherOptions};
    iox::popo::PublisherPortRouDi m_sutNoOfferOnCreateRouDiSide{&m_publisherPortDataNoOfferOnCreate};
    iox::popo::PublisherPortUser m_sutNoOfferOnCreateUserSide{&m_publisherPortDataNoOfferOnCreate};

    // publisher port that waits for subscriber when queue is full
    iox::popo::PublisherOptions m_waitForSubscriberPublisherOptions{
        0U, iox::NodeName_t{""}, false, iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER};
    iox::popo::PublisherPortData m_publisherPortDataWaitForSubscriber{
        iox::capro::ServiceDescription("a", "b", "c"), "myApp", &m_memoryManager, m_waitForSubscriberPublisherOptions};
    iox::popo::PublisherPortRouDi m_sutWaitForSubscriberRouDiSide{&m_publisherPortDataWaitForSubscriber};
    iox::popo::PublisherPortUser m_sutWaitForSubscriberUserSide{&m_publisherPortDataWaitForSubscriber};

    // publisher port w/ history
    iox::popo::PublisherOptions m_withHistoryPublisherOptions{
        iox::MAX_PUBLISHER_HISTORY, iox::NodeName_t{""}, true, iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA};
    iox::popo::PublisherPortData m_publisherPortDataHistory{
        iox::capro::ServiceDescription("x", "y", "z"), "myApp", &m_memoryManager, m_withHistoryPublisherOptions};
    iox::popo::PublisherPortUser m_sutWithHistoryUserSide{&m_publisherPortDataHistory};
    iox::popo::PublisherPortRouDi m_sutWithHistoryRouDiSide{&m_publisherPortDataHistory};

    // publisher port w/ default options
    iox::popo::PublisherOptions m_withDefaultPublisherOptions{};
    iox::popo::PublisherPortData m_publisherPortDataDefault{
        iox::capro::ServiceDescription("x", "y", "z"), "myApp", &m_memoryManager, m_withDefaultPublisherOptions};
    iox::popo::PublisherPortUser m_sutWithDefaultOptionsUserSide{&m_publisherPortDataDefault};
    iox::popo::PublisherPortRouDi m_sutWithDefaultOptionsRouDiSide{&m_publisherPortDataDefault};
};

TEST_F(PublisherPort_test, initialStateIsOfferedWithDefaultOptions)
{
    EXPECT_TRUE(m_sutWithDefaultOptionsUserSide.isOffered());
}

TEST_F(PublisherPort_test, initialStateIsNotOfferedWhenNoOfferOnCreate)
{
    EXPECT_FALSE(m_sutNoOfferOnCreateUserSide.isOffered());
}

TEST_F(PublisherPort_test, initialStateIsNoSubscribers)
{
    EXPECT_FALSE(m_sutNoOfferOnCreateUserSide.hasSubscribers());
}

TEST_F(PublisherPort_test, noWaitingForSubscriberWithDefaultOptions)
{
    EXPECT_THAT(m_sutWithDefaultOptionsRouDiSide.getSubscriberTooSlowPolicy(),
                Eq(iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA));
}

TEST_F(PublisherPort_test, initialStateReturnsOfferCaProMessageWithDefaultOptions)
{
    auto maybeCaproMessage = m_sutWithDefaultOptionsRouDiSide.tryGetCaProMessage();

    ASSERT_TRUE(maybeCaproMessage.has_value());
    auto caproMessage = maybeCaproMessage.value();
    EXPECT_THAT(caproMessage.m_type, Eq(iox::capro::CaproMessageType::OFFER));
}

TEST_F(PublisherPort_test, initialStateReturnsNoCaProMessageWhenNoOfferOnCreate)
{
    auto maybeCaproMessage = m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();

    EXPECT_FALSE(maybeCaproMessage.has_value());
}

TEST_F(PublisherPort_test, waitingForSubscriberWhenDesired)
{
    EXPECT_THAT(m_sutWaitForSubscriberRouDiSide.getSubscriberTooSlowPolicy(),
                Eq(iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER));
}

TEST_F(PublisherPort_test, offerCallResultsInOfferedState)
{
    m_sutNoOfferOnCreateUserSide.offer();

    EXPECT_TRUE(m_sutNoOfferOnCreateUserSide.isOffered());
}

TEST_F(PublisherPort_test, offerCallResultsInOfferCaProMessage)
{
    m_sutNoOfferOnCreateUserSide.offer();

    auto maybeCaproMessage = m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();

    ASSERT_TRUE(maybeCaproMessage.has_value());
    auto caproMessage = maybeCaproMessage.value();
    EXPECT_THAT(caproMessage.m_type, Eq(iox::capro::CaproMessageType::OFFER));
    EXPECT_THAT(caproMessage.m_serviceDescription, Eq(iox::capro::ServiceDescription("a", "b", "c")));
    EXPECT_THAT(caproMessage.m_subType, Eq(iox::capro::CaproMessageSubType::EVENT));
    EXPECT_THAT(caproMessage.m_historyCapacity, Eq(0U));
}

TEST_F(PublisherPort_test, stopOfferCallResultsInNotOfferedState)
{
    m_sutNoOfferOnCreateUserSide.offer();

    m_sutNoOfferOnCreateUserSide.stopOffer();

    EXPECT_FALSE(m_sutNoOfferOnCreateUserSide.isOffered());
}

TEST_F(PublisherPort_test, stopOfferCallResultsInStopOfferCaProMessage)
{
    // arrange, we need a transition from offer to stop offer, also form a RouDi point of view
    // therefore we must also get the offer CapPro message (but ignore it here)
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    m_sutNoOfferOnCreateUserSide.stopOffer();

    auto maybeCaproMessage = m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();

    ASSERT_TRUE(maybeCaproMessage.has_value());
    auto caproMessage = maybeCaproMessage.value();
    EXPECT_THAT(caproMessage.m_type, Eq(iox::capro::CaproMessageType::STOP_OFFER));
    EXPECT_THAT(caproMessage.m_serviceDescription, Eq(iox::capro::ServiceDescription("a", "b", "c")));
}

TEST_F(PublisherPort_test, offerStateChangesThatEndUpInTheSameStateDoNotReturnACaProMessage)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateUserSide.stopOffer();

    auto maybeCaproMessage = m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();

    EXPECT_FALSE(maybeCaproMessage.has_value());
}

TEST_F(PublisherPort_test,
       offerCallWhenHavingHistoryResultsInOfferCaProMessageWithSubTypeFieldAndCorrectHistoryCapacity)
{
    m_sutWithHistoryUserSide.offer();

    auto maybeCaproMessage = m_sutWithHistoryRouDiSide.tryGetCaProMessage();
    ASSERT_TRUE(maybeCaproMessage.has_value());
    auto caproMessage = maybeCaproMessage.value();
    EXPECT_THAT(caproMessage.m_type, Eq(iox::capro::CaproMessageType::OFFER));
    EXPECT_THAT(caproMessage.m_serviceDescription, Eq(iox::capro::ServiceDescription("x", "y", "z")));
    EXPECT_THAT(caproMessage.m_subType, Eq(iox::capro::CaproMessageSubType::FIELD));
    EXPECT_THAT(caproMessage.m_historyCapacity, Eq(iox::MAX_PUBLISHER_HISTORY));
}

TEST_F(PublisherPort_test, allocatingAChunkWithoutUserHeaderAndSmallUserPayloadAlignmentResultsInSmallChunk)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{SMALL_CHUNK / 2};
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(PublisherPort_test, allocatingAChunkWithoutUserHeaderAndLargeUserPayloadAlignmentResultsInLargeChunk)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{SMALL_CHUNK / 2};
    constexpr uint32_t LARGE_USER_PAYLOAD_ALIGNMENT{SMALL_CHUNK};
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        USER_PAYLOAD_SIZE, LARGE_USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));
}

TEST_F(PublisherPort_test, allocatingAChunkWithLargeUserHeaderResultsInLargeChunk)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{SMALL_CHUNK / 2};
    constexpr uint32_t LARGE_USER_HEADER_SIZE{SMALL_CHUNK};
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, LARGE_USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    EXPECT_FALSE(maybeChunkHeader.has_error());
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(1).m_usedChunks, Eq(1U));
}

TEST_F(PublisherPort_test, releasingAnAllocatedChunkReleasesTheMemory)
{
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();

    m_sutNoOfferOnCreateUserSide.releaseChunk(chunkHeader);

    // this one is not stored in the last chunk, so all chunks must be free again
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}

TEST_F(PublisherPort_test, allocatedChunkContainsPublisherIdAsOriginId)
{
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();

    EXPECT_THAT(chunkHeader->originId(), Eq(m_sutNoOfferOnCreateUserSide.getUniqueID()));
    m_sutNoOfferOnCreateUserSide.releaseChunk(chunkHeader);
}

TEST_F(PublisherPort_test, allocateAndSendAChunkWithoutSubscriberHoldsTheLast)
{
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();

    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);

    // this one is stored in the last chunk, so this chunk is still in use
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(PublisherPort_test, allocateAndSendMultipleChunksWithoutSubscriberHoldsOnlyTheLast)
{
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();
    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);
    maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    chunkHeader = maybeChunkHeader.value();
    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);
    maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    chunkHeader = maybeChunkHeader.value();
    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);

    // the last is stored in the last chunk, so one chunk is still in use
    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(1U));
}

TEST_F(PublisherPort_test, subscribeWhenNotOfferedReturnsNACK)
{
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 0U;

    auto maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    ASSERT_TRUE(maybeCaProMessage.has_value());
    auto caproMessageResponse = maybeCaProMessage.value();
    EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::NACK));
}

TEST_F(PublisherPort_test, unsubscribeWhenNotSubscribedReturnsNACK)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::UNSUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 0U;

    auto maybeCaproMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    ASSERT_TRUE(maybeCaproMessage.has_value());
    auto caproMessageResponse = maybeCaproMessage.value();
    EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::NACK));
}

TEST_F(PublisherPort_test, subscribeWhenOfferedReturnsACKAndWeHaveSubscribers)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 0U;

    auto maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_TRUE(maybeCaProMessage.has_value());
    auto caproMessageResponse = maybeCaProMessage.value();
    EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::ACK));
    EXPECT_TRUE(m_sutNoOfferOnCreateUserSide.hasSubscribers());
}

TEST_F(PublisherPort_test, unsubscribeWhenSubscribedReturnsACKAndWeHaveNoMoreSubscribers)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 0U;
    auto maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    // set CaPro message to UNSUB, the other members are reused
    caproMessage.m_type = iox::capro::CaproMessageType::UNSUB;

    maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    ASSERT_TRUE(maybeCaProMessage.has_value());
    auto caproMessageResponse = maybeCaProMessage.value();
    EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::ACK));
    EXPECT_FALSE(m_sutNoOfferOnCreateUserSide.hasSubscribers());
}

TEST_F(PublisherPort_test, subscribeManyIsFine)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    // using dummy pointers for the provided chunk queue data
    uint64_t dummy;
    uint64_t* dummyPtr = &dummy;
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = reinterpret_cast<ChunkQueueData_t*>(dummyPtr);
    caproMessage.m_historyCapacity = 0U;

    for (size_t i = 0; i < iox::MAX_SUBSCRIBERS_PER_PUBLISHER; i++)
    {
        auto maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
        ASSERT_TRUE(maybeCaProMessage.has_value());
        auto caproMessageResponse = maybeCaProMessage.value();
        EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::ACK));
        dummyPtr++;
        caproMessage.m_chunkQueueData = reinterpret_cast<ChunkQueueData_t*>(dummyPtr);
    }
}

TEST_F(PublisherPort_test, subscribeTillOverflowReturnsNACK)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    // using dummy pointers for the provided chunk queue data
    uint64_t dummy;
    uint64_t* dummyPtr = &dummy;
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = reinterpret_cast<ChunkQueueData_t*>(dummyPtr);
    caproMessage.m_historyCapacity = 0U;
    for (size_t i = 0; i < iox::MAX_SUBSCRIBERS_PER_PUBLISHER; i++)
    {
        m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
        dummyPtr++;
        caproMessage.m_chunkQueueData = reinterpret_cast<ChunkQueueData_t*>(dummyPtr);
    }

    auto maybeCaProMessage = m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    ASSERT_TRUE(maybeCaProMessage.has_value());
    auto caproMessageResponse = maybeCaProMessage.value();
    EXPECT_THAT(caproMessageResponse.m_type, Eq(iox::capro::CaproMessageType::NACK));
}

TEST_F(PublisherPort_test, sendWhenSubscribedDeliversAChunk)
{
    m_sutNoOfferOnCreateUserSide.offer();
    m_sutNoOfferOnCreateRouDiSide.tryGetCaProMessage();
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 0U;
    m_sutNoOfferOnCreateRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();
    auto sample = chunkHeader->userPayload();
    new (sample) DummySample();
    static_cast<DummySample*>(sample)->dummy = 17;
    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);
    iox::popo::ChunkQueuePopper<ChunkQueueData_t> m_chunkQueuePopper(&m_chunkQueueData);

    auto maybeSharedChunk = m_chunkQueuePopper.tryPop();

    ASSERT_TRUE(maybeSharedChunk.has_value());
    auto sharedChunk = maybeSharedChunk.value();
    auto dummySample = *reinterpret_cast<DummySample*>(sharedChunk.getUserPayload());
    EXPECT_THAT(dummySample.dummy, Eq(17U));
}

TEST_F(PublisherPort_test, subscribeWithHistoryLikeTheARAField)
{
    iox::popo::PublisherOptions options;
    options.historyCapacity = 1U;
    iox::popo::PublisherPortData publisherPortDataHistory{
        iox::capro::ServiceDescription("x", "y", "z"), "myApp", &m_memoryManager, options};
    iox::popo::PublisherPortUser sutWithHistoryUseriSide{&publisherPortDataHistory};
    iox::popo::PublisherPortRouDi sutWithHistoryRouDiSide{&publisherPortDataHistory};
    // do it the ara field like way
    // 1. publish a chunk to a not yet offered publisher
    auto maybeChunkHeader = sutWithHistoryUseriSide.tryAllocateChunk(
        sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();
    auto sample = chunkHeader->userPayload();
    new (sample) DummySample();
    static_cast<DummySample*>(sample)->dummy = 17;
    sutWithHistoryUseriSide.sendChunk(chunkHeader);
    // 2. offer
    sutWithHistoryUseriSide.offer();
    sutWithHistoryRouDiSide.tryGetCaProMessage();
    // 3. subscribe with a history request
    ChunkQueueData_t m_chunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                      iox::cxx::VariantQueueTypes::SoFi_SingleProducerSingleConsumer};
    iox::capro::CaproMessage caproMessage(iox::capro::CaproMessageType::SUB,
                                          iox::capro::ServiceDescription("a", "b", "c"));
    caproMessage.m_chunkQueueData = &m_chunkQueueData;
    caproMessage.m_historyCapacity = 1U; // request history of 1
    sutWithHistoryRouDiSide.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
    iox::popo::ChunkQueuePopper<ChunkQueueData_t> m_chunkQueuePopper(&m_chunkQueueData);

    // 4. We get the history value on subscribe
    auto maybeSharedChunk = m_chunkQueuePopper.tryPop();

    ASSERT_TRUE(maybeSharedChunk.has_value());
    auto sharedChunk = maybeSharedChunk.value();
    auto dummySample = *reinterpret_cast<DummySample*>(sharedChunk.getUserPayload());
    EXPECT_THAT(dummySample.dummy, Eq(17U));
}

TEST_F(PublisherPort_test, noLastChunkWhenNothingSent)
{
    auto maybeLastChunkHeader = m_sutNoOfferOnCreateUserSide.tryGetPreviousChunk();

    EXPECT_FALSE(maybeLastChunkHeader.has_value());
}

TEST_F(PublisherPort_test, lastChunkAvailableAfterSend)
{
    auto maybeChunkHeader = m_sutNoOfferOnCreateUserSide.tryAllocateChunk(
        10U, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    auto chunkHeader = maybeChunkHeader.value();
    auto firstPayloadPtr = chunkHeader->userPayload();
    m_sutNoOfferOnCreateUserSide.sendChunk(chunkHeader);

    auto maybeLastChunkHeader = m_sutNoOfferOnCreateUserSide.tryGetPreviousChunk();

    ASSERT_TRUE(maybeLastChunkHeader.has_value());
    EXPECT_THAT(maybeLastChunkHeader.value()->userPayload(), Eq(firstPayloadPtr));
}

TEST_F(PublisherPort_test, cleanupReleasesAllChunks)
{
    // push some chunks to history
    for (size_t i = 0; i < iox::MAX_PUBLISHER_HISTORY; i++)
    {
        auto maybeChunkHeader = m_sutWithHistoryUserSide.tryAllocateChunk(
            sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
        auto chunkHeader = maybeChunkHeader.value();
        m_sutWithHistoryUserSide.sendChunk(chunkHeader);
    }
    // allocate some samples
    auto maybeChunkHeader1 = m_sutWithHistoryUserSide.tryAllocateChunk(
        sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader1.has_error());
    auto maybeChunkHeader2 = m_sutWithHistoryUserSide.tryAllocateChunk(
        sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader2.has_error());
    auto maybeChunkHeader3 = m_sutWithHistoryUserSide.tryAllocateChunk(
        sizeof(DummySample), alignof(DummySample), USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    EXPECT_FALSE(maybeChunkHeader3.has_error());

    m_sutWithHistoryRouDiSide.releaseAllChunks();

    EXPECT_THAT(m_memoryManager.getMemPoolInfo(0).m_usedChunks, Eq(0U));
}
