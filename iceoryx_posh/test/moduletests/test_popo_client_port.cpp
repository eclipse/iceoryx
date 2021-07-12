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

#include "iceoryx_posh/internal/popo/ports/client_port_data.hpp"
#include "iceoryx_posh/internal/popo/ports/client_port_roudi.hpp"
#include "iceoryx_posh/internal/popo/ports/client_port_user.hpp"

#include "iceoryx_hoofs/testing/watch_dog.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::capro;
using namespace iox::popo;

class ClientPort_test : public Test
{
    // keep this the very first and also private
    iox::cxx::GenericRAII m_uniqueRouDiId{[] { iox::popo::internal::setUniqueRouDiId(0U); },
                                          [] { iox::popo::internal::unsetUniqueRouDiId(); }};

    static constexpr iox::units::Duration DEADLOCK_TIMEOUT{5_s};
    Watchdog m_deadlockWatchdog{DEADLOCK_TIMEOUT};

    struct SutClientPort
    {
        SutClientPort(const ServiceDescription& serviceDescription,
                      const iox::RuntimeName_t& runtimeName,
                      const ClientOptions& clientOptions,
                      iox::mepoo::MemoryManager& memoryManager)
            : portData(serviceDescription, runtimeName, clientOptions, &memoryManager)
        {
        }

        ClientPortData portData;
        ClientPortUser portUser{portData};
        ClientPortRouDi portRouDi{portData};
        ChunkQueuePusher<ClientChunkQueueData_t> responseQueuePusher{&portData.m_chunkReceiverData};
    };

  public:
    ClientPort_test()
    {
        iox::mepoo::MePooConfig mempoolconf;
        mempoolconf.addMemPool({CHUNK_SIZE, NUM_CHUNKS});
        m_memoryManager.configureMemoryManager(mempoolconf, m_memoryAllocator, m_memoryAllocator);
    }

    void SetUp() override
    {
        m_deadlockWatchdog.watchAndActOnFailure([] { std::terminate(); });

        // this is basically what RouDi does when a client is requested
        tryAdvanceToState(clientPortWithConnectOnCreate, iox::ConnectionState::CONNECTED);
        tryAdvanceToState(clientPortWithoutConnectOnCreate, iox::ConnectionState::NOT_CONNECTED);
    }

    void TearDown() override
    {
    }

    void tryAdvanceToState(SutClientPort& clientPort, const iox::ConnectionState targetState)
    {
        auto maybeCaProMessage = clientPort.portRouDi.tryGetCaProMessage();
        if (targetState == iox::ConnectionState::NOT_CONNECTED && clientPort.portData.m_connectionState == targetState)
        {
            return;
        }

        ASSERT_TRUE(maybeCaProMessage.has_value());
        auto& clientMessage = maybeCaProMessage.value();
        ASSERT_THAT(clientMessage.m_type, Eq(CaproMessageType::CONNECT));
        ASSERT_THAT(clientMessage.m_chunkQueueData, Ne(nullptr));
        ASSERT_THAT(clientPort.portData.m_connectionState, Eq(iox::ConnectionState::CONNECT_REQUESTED));
        if (clientPort.portData.m_connectionState == targetState)
        {
            return;
        }

        if (targetState == iox::ConnectionState::WAIT_FOR_OFFER)
        {
            CaproMessage serverMessageNack{CaproMessageType::NACK, m_serviceDescription};
            clientPort.portRouDi.dispatchCaProMessageAndGetPossibleResponse(serverMessageNack);
            ASSERT_THAT(clientPort.portData.m_connectionState, Eq(targetState));
            return;
        }

        CaproMessage serverMessageAck{CaproMessageType::ACK, m_serviceDescription};
        serverMessageAck.m_chunkQueueData = &serverChunkQueueData;
        clientPort.portRouDi.dispatchCaProMessageAndGetPossibleResponse(serverMessageAck);
        ASSERT_THAT(clientPort.portData.m_connectionState, Eq(iox::ConnectionState::CONNECTED));
        if (clientPort.portData.m_connectionState == targetState)
        {
            return;
        }

        CaproMessage serverMessageDisconnect{CaproMessageType::DISCONNECT, m_serviceDescription};
        clientPort.portRouDi.dispatchCaProMessageAndGetPossibleResponse(serverMessageDisconnect);
        ASSERT_THAT(clientPort.portData.m_connectionState, Eq(iox::ConnectionState::DISCONNECT_REQUESTED));
        if (clientPort.portData.m_connectionState == targetState)
        {
            return;
        }

        constexpr bool NOT_IMPLEMENTED{true};
        ASSERT_FALSE(NOT_IMPLEMENTED);
    }

    SutClientPort& initAndGetClientPortForStateTransitionTests()
    {
        clientPortForStateTransitionTests.reset();
        clientPortForStateTransitionTests.emplace(
            m_serviceDescription, m_runtimeName, m_clientOptionsWithoutConnectOnCreate, m_memoryManager);
        return clientPortForStateTransitionTests.value();
    }

    uint32_t getNumberOfUsedChunks() const
    {
        return m_memoryManager.getMemPoolInfo(0U).m_usedChunks;
    }

    iox::mepoo::SharedChunk getChunkFromMemoryManager(uint32_t userPayloadSize, uint32_t userHeaderSize)
    {
        auto chunkSettingsResult = iox::mepoo::ChunkSettings::create(userPayloadSize,
                                                                     iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT,
                                                                     userHeaderSize,
                                                                     iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
        iox::cxx::Expects(!chunkSettingsResult.has_error());
        return m_memoryManager.getChunk(chunkSettingsResult.value());
    }

    /// @return true if all pushes succeed, false if a push failed and a chunk was lost
    bool pushResponses(ChunkQueuePusher<ClientChunkQueueData_t>& chunkQueuePusher, uint64_t numberOfPushes)
    {
        for (auto i = 0U; i < numberOfPushes; ++i)
        {
            constexpr uint32_t USER_PAYLOAD_SIZE{10};
            auto sharedChunk = getChunkFromMemoryManager(USER_PAYLOAD_SIZE, sizeof(ResponseHeader));
            if (!chunkQueuePusher.push(sharedChunk))
            {
                chunkQueuePusher.lostAChunk();
                return false;
            }
        }
        return true;
    }

    static constexpr uint64_t QUEUE_CAPACITY{4};

  private:
    static constexpr uint32_t NUM_CHUNKS = 1024U;
    static constexpr uint32_t CHUNK_SIZE = 128U;
    static constexpr size_t MEMORY_SIZE = 1024U * 1024U;
    uint8_t m_memory[MEMORY_SIZE];
    iox::posix::Allocator m_memoryAllocator{m_memory, MEMORY_SIZE};
    iox::mepoo::MemoryManager m_memoryManager;

    ServiceDescription m_serviceDescription{"hyp", "no", "toad"};
    iox::RuntimeName_t m_runtimeName{"hypnotoad"};

    ClientOptions m_clientOptionsWithConnectOnCreate = [&] {
        ClientOptions options;
        options.connectOnCreate = true;
        options.responseQueueCapacity = QUEUE_CAPACITY;
        return options;
    }();
    ClientOptions m_clientOptionsWithoutConnectOnCreate = [] {
        ClientOptions options;
        options.connectOnCreate = false;
        options.responseQueueCapacity = QUEUE_CAPACITY;
        return options;
    }();

    ClientOptions m_clientOptionsWithBlockProducerResponseQueueFullPolicy = [&] {
        ClientOptions options;
        options.responseQueueCapacity = QUEUE_CAPACITY;
        options.responseQueueFullPolicy = QueueFullPolicy2::BLOCK_PRODUCER;
        return options;
    }();

    iox::cxx::optional<SutClientPort> clientPortForStateTransitionTests;

  public:
    static constexpr uint32_t USER_PAYLOAD_SIZE{32U};
    static constexpr uint32_t USER_PAYLOAD_ALIGNMENT{8U};

    ServerChunkQueueData_t serverChunkQueueData{iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA,
                                                iox::cxx::VariantQueueTypes::SoFi_MultiProducerSingleConsumer};
    ChunkQueuePopper<ServerChunkQueueData_t> serverRequestQueue{&serverChunkQueueData};

    SutClientPort clientPortWithConnectOnCreate{
        m_serviceDescription, m_runtimeName, m_clientOptionsWithConnectOnCreate, m_memoryManager};
    SutClientPort clientPortWithoutConnectOnCreate{
        m_serviceDescription, m_runtimeName, m_clientOptionsWithoutConnectOnCreate, m_memoryManager};
    SutClientPort clientPortWithBlockProducerResponseQueuePolicy{
        m_serviceDescription, m_runtimeName, m_clientOptionsWithBlockProducerResponseQueueFullPolicy, m_memoryManager};
};
constexpr iox::units::Duration ClientPort_test::DEADLOCK_TIMEOUT;


/// @todo iox-#27 do tests related to QueueFullPolicy in integration test with a real ServerPort and add a note in this
/// file that those tests are part of the integration test

// BEGIN ClientPortUser tests

TEST_F(ClientPort_test, InitialConnectionStateOnPortWithConnectOnCreateIs_CONNECTED)
{
    auto& sut = clientPortWithConnectOnCreate;
    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::CONNECTED));
}

TEST_F(ClientPort_test, InitialConnectionStateOnPortWithoutConnectOnCreateIs_NOT_CONNECTED)
{
    auto& sut = clientPortWithoutConnectOnCreate;
    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::NOT_CONNECTED));
}

TEST_F(ClientPort_test, AllocateRequestDoesNotFailAndUsesTheMempool)
{
    auto& sut = clientPortWithConnectOnCreate;
    EXPECT_THAT(getNumberOfUsedChunks(), Eq(0U));

    auto maybeRequest = sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(maybeRequest.has_error());

    EXPECT_THAT(getNumberOfUsedChunks(), Eq(1U));
}

TEST_F(ClientPort_test, FreeRequestWithNullptrCallsErrorHandler)
{
    auto& sut = clientPortWithConnectOnCreate;

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_EQ(errorLevel, iox::ErrorLevel::SEVERE);
        });

    sut.portUser.freeRequest(nullptr);

    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOPO__CLIENT_PORT_INVALID_REQUEST_TO_FREE_FROM_USER);
}

TEST_F(ClientPort_test, FreeRequestWithValidRequestWorksAndReleasesTheChunkToTheMempool)
{
    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto& requestHeader) {
            EXPECT_THAT(this->getNumberOfUsedChunks(), Eq(1U));
            sut.portUser.freeRequest(requestHeader);
            EXPECT_THAT(this->getNumberOfUsedChunks(), Eq(0U));
        })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });
}

TEST_F(ClientPort_test, SendRequestWithNullptrOnConnectedClientPortTerminates)
{
    auto& sut = clientPortWithConnectOnCreate;

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_EQ(errorLevel, iox::ErrorLevel::SEVERE);
        });

    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto&) { sut.portUser.sendRequest(nullptr); })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });

    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOPO__CLIENT_PORT_INVALID_REQUEST_TO_SEND_FROM_USER);
}

TEST_F(ClientPort_test, SendRequestOnConnectedClientPortEnqueuesRequestToServerQueue)
{
    constexpr int64_t SEQUENCE_ID{42U};
    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto& requestHeader) {
            requestHeader->setSequenceId(SEQUENCE_ID);
            sut.portUser.sendRequest(requestHeader);
        })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });

    serverRequestQueue.tryPop()
        .and_then([&](auto& sharedChunk) {
            auto requestHeader = static_cast<RequestHeader*>(sharedChunk.getChunkHeader()->userHeader());
            EXPECT_THAT(requestHeader->getSequenceId(), Eq(SEQUENCE_ID));
        })
        .or_else([&] {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });
}

TEST_F(ClientPort_test, SendRequestOnNotConnectedClientPortDoesNotEnqueuesRequestToServerQueue)
{
    auto& sut = clientPortWithoutConnectOnCreate;
    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto& requestHeader) { sut.portUser.sendRequest(requestHeader); })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });

    EXPECT_FALSE(serverRequestQueue.tryPop().has_value());
}

TEST_F(ClientPort_test, ConnectAfterPreviousSendRequestCallDoesNotEnqueuesRequestToServerQueue)
{
    auto& sut = clientPortWithoutConnectOnCreate;
    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto& requestHeader) { sut.portUser.sendRequest(requestHeader); })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });

    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::CONNECTED);

    EXPECT_FALSE(serverRequestQueue.tryPop().has_value());
}

TEST_F(ClientPort_test, GetResponseOnNotConnectedClientPortHasNoResponse)
{
    auto& sut = clientPortWithoutConnectOnCreate;
    sut.portUser.getResponse()
        .and_then([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        })
        .or_else([&](auto& err) { EXPECT_THAT(err, Eq(iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)); });
}

TEST_F(ClientPort_test, GetResponseOnConnectedClientPortWithEmptyResponseQueueHasNoResponse)
{
    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.getResponse()
        .and_then([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        })
        .or_else([&](auto& err) { EXPECT_THAT(err, Eq(iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)); });
}

TEST_F(ClientPort_test, GetResponseOnConnectedClientPortWithNonEmptyResponseQueueHasResponse)
{
    constexpr int64_t SEQUENCE_ID{13U};
    auto& sut = clientPortWithConnectOnCreate;

    constexpr uint32_t USER_PAYLOAD_SIZE{10};
    auto sharedChunk = getChunkFromMemoryManager(USER_PAYLOAD_SIZE, sizeof(ResponseHeader));
    new (sharedChunk.getChunkHeader()->userHeader())
        ResponseHeader(iox::UniquePortId(), RpcBaseHeader::UNKNOWN_CLIENT_QUEUE_INDEX, SEQUENCE_ID);
    sut.responseQueuePusher.push(sharedChunk);

    sut.portUser.getResponse()
        .and_then([&](auto& responseHeader) { EXPECT_THAT(responseHeader->getSequenceId(), Eq(SEQUENCE_ID)); })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });
}

TEST_F(ClientPort_test, ReleaseResponseWithNullptrIsTerminating)
{
    auto& sut = clientPortWithConnectOnCreate;

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_EQ(errorLevel, iox::ErrorLevel::SEVERE);
        });

    sut.portUser.releaseResponse(nullptr);

    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOPO__CLIENT_PORT_INVALID_RESPONSE_TO_RELEASE_FROM_USER);
}

TEST_F(ClientPort_test, ReleaseResponseWithValidResponseReleasesChunkToTheMempool)
{
    auto& sut = clientPortWithConnectOnCreate;

    constexpr uint32_t USER_PAYLOAD_SIZE{10};

    iox::cxx::optional<iox::mepoo::SharedChunk> sharedChunk{
        getChunkFromMemoryManager(USER_PAYLOAD_SIZE, sizeof(ResponseHeader))};
    sut.responseQueuePusher.push(sharedChunk.value());
    sharedChunk.reset();

    sut.portUser.getResponse()
        .and_then([&](auto& responseHeader) {
            EXPECT_THAT(this->getNumberOfUsedChunks(), Eq(1U));
            sut.portUser.releaseResponse(responseHeader);
            EXPECT_THAT(this->getNumberOfUsedChunks(), Eq(0U));
        })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });
}

TEST_F(ClientPort_test, HasNewResponseOnEmptyResponseQueueReturnsFalse)
{
    auto& sut = clientPortWithConnectOnCreate;
    EXPECT_FALSE(sut.portUser.hasNewResponses());
}

TEST_F(ClientPort_test, HasNewResponseOnNonEmptyResponseQueueReturnsTrue)
{
    auto& sut = clientPortWithConnectOnCreate;

    constexpr uint32_t USER_PAYLOAD_SIZE{10};
    auto sharedChunk = getChunkFromMemoryManager(USER_PAYLOAD_SIZE, sizeof(ResponseHeader));
    sut.responseQueuePusher.push(sharedChunk);

    EXPECT_TRUE(sut.portUser.hasNewResponses());
}

TEST_F(ClientPort_test, HasNewResponseOnEmptyResponseQueueAfterPreviouslyNotEmptyReturnsFalse)
{
    auto& sut = clientPortWithConnectOnCreate;

    constexpr uint32_t USER_PAYLOAD_SIZE{10};
    auto sharedChunk = getChunkFromMemoryManager(USER_PAYLOAD_SIZE, sizeof(ResponseHeader));
    sut.responseQueuePusher.push(sharedChunk);

    EXPECT_FALSE(sut.portUser.getResponse().has_error());

    EXPECT_FALSE(sut.portUser.hasNewResponses());
}

TEST_F(ClientPort_test, HasLostResponsesSinceLastCallWithoutLosingResponsesReturnsFalse)
{
    auto& sut = clientPortWithConnectOnCreate;
    EXPECT_FALSE(sut.portUser.hasLostResponsesSinceLastCall());
}

TEST_F(ClientPort_test, HasLostResponsesSinceLastCallWithoutLosingResponsesAndQueueFullReturnsFalse)
{
    auto& sut = clientPortWithConnectOnCreate;

    EXPECT_TRUE(pushResponses(sut.responseQueuePusher, QUEUE_CAPACITY));
    EXPECT_FALSE(sut.portUser.hasLostResponsesSinceLastCall());
}

TEST_F(ClientPort_test, HasLostResponsesSinceLastCallWithLosingResponsesReturnsTrue)
{
    auto& sut = clientPortWithConnectOnCreate;

    EXPECT_FALSE(pushResponses(sut.responseQueuePusher, QUEUE_CAPACITY + 1U));
    EXPECT_TRUE(sut.portUser.hasLostResponsesSinceLastCall());
}

TEST_F(ClientPort_test, HasLostResponsesSinceLastCallReturnsFalseAfterPreviouslyReturningTrue)
{
    auto& sut = clientPortWithConnectOnCreate;

    EXPECT_FALSE(pushResponses(sut.responseQueuePusher, QUEUE_CAPACITY + 1U));
    EXPECT_TRUE(sut.portUser.hasLostResponsesSinceLastCall());
    EXPECT_FALSE(sut.portUser.hasLostResponsesSinceLastCall());
}

TEST_F(ClientPort_test, ConditionVariableInitiallyNotSet)
{
    auto& sut = clientPortWithConnectOnCreate;
    EXPECT_FALSE(sut.portUser.isConditionVariableSet());
}

TEST_F(ClientPort_test, SettingConditionVariableWithoutConditionVariablePresentWorks)
{
    iox::popo::ConditionVariableData condVar{"hypnotoad"};
    constexpr uint32_t NOTIFICATION_INDEX{1};

    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.setConditionVariable(condVar, NOTIFICATION_INDEX);

    EXPECT_TRUE(sut.portUser.isConditionVariableSet());
}

TEST_F(ClientPort_test, UnsettingConditionVariableWithConditionVariablePresentWorks)
{
    iox::popo::ConditionVariableData condVar{"brain slug"};
    constexpr uint32_t NOTIFICATION_INDEX{2};

    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.setConditionVariable(condVar, NOTIFICATION_INDEX);

    sut.portUser.unsetConditionVariable();

    EXPECT_FALSE(sut.portUser.isConditionVariableSet());
}

TEST_F(ClientPort_test, UnsettingConditionVariableWithoutConditionVariablePresentIsHandledGracefully)
{
    auto& sut = clientPortWithConnectOnCreate;
    sut.portUser.unsetConditionVariable();

    EXPECT_FALSE(sut.portUser.isConditionVariableSet());
}

TEST_F(ClientPort_test, ConnectOnNotConnectedClientPortResultsInStateChange)
{
    auto& sut = clientPortWithoutConnectOnCreate;

    sut.portUser.connect();

    EXPECT_TRUE(sut.portRouDi.tryGetCaProMessage().has_value());
}

TEST_F(ClientPort_test, ConnectOnConnectedClientPortResultsInNoStateChange)
{
    auto& sut = clientPortWithConnectOnCreate;

    sut.portUser.connect();

    EXPECT_FALSE(sut.portRouDi.tryGetCaProMessage().has_value());
}

TEST_F(ClientPort_test, DisconnectOnConnectedClientPortResultsInStateChange)
{
    auto& sut = clientPortWithConnectOnCreate;

    sut.portUser.disconnect();

    EXPECT_TRUE(sut.portRouDi.tryGetCaProMessage().has_value());
}

TEST_F(ClientPort_test, DisconnectOnNotConnectedClientPortResultsInNoStateChange)
{
    auto& sut = clientPortWithoutConnectOnCreate;

    sut.portUser.disconnect();

    EXPECT_FALSE(sut.portRouDi.tryGetCaProMessage().has_value());
}

// END ClientPortUser tests


// BEGIN ClientPortRouDi tests

TEST_F(ClientPort_test, GetResponseQueueFullPolicyOnPortWithDefaultOptionIsDiscardOldesData)
{
    auto& sut = clientPortWithConnectOnCreate;

    EXPECT_THAT(sut.portRouDi.getResponseQueueFullPolicy(), Eq(QueueFullPolicy2::DISCARD_OLDEST_DATA));
}

TEST_F(ClientPort_test, GetResponseQueueFullPolicyOnPortWithBlockProducerOptionIsBlockProducer)
{
    auto& sut = clientPortWithBlockProducerResponseQueuePolicy;

    EXPECT_THAT(sut.portRouDi.getResponseQueueFullPolicy(), Eq(QueueFullPolicy2::BLOCK_PRODUCER));
}

TEST_F(ClientPort_test, TryGetCaProMessageOnConnectHasCaProMessageTypeConnect)
{
    auto& sut = clientPortWithoutConnectOnCreate;

    sut.portUser.connect();

    auto caproMessage = sut.portRouDi.tryGetCaProMessage();

    ASSERT_TRUE(caproMessage.has_value());
    EXPECT_THAT(caproMessage->m_type, Eq(CaproMessageType::CONNECT));
}

TEST_F(ClientPort_test, TryGetCaProMessageOnDisconnectHasCaProMessageTypeDisconnect)
{
    auto& sut = clientPortWithConnectOnCreate;

    sut.portUser.disconnect();

    auto caproMessage = sut.portRouDi.tryGetCaProMessage();

    ASSERT_TRUE(caproMessage.has_value());
    EXPECT_THAT(caproMessage->m_type, Eq(CaproMessageType::DISCONNECT));
}

TEST_F(ClientPort_test, ReleaseAllChunksWorks)
{
    auto& sut = clientPortWithConnectOnCreate;

    // produce chunks for the chunk sender
    sut.portUser.allocateRequest(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT)
        .and_then([&](auto& requestHeader) { sut.portUser.sendRequest(requestHeader); })
        .or_else([&](auto&) {
            constexpr bool UNREACHABLE{false};
            EXPECT_TRUE(UNREACHABLE);
        });

    // produce chunks for the chunk receiver
    EXPECT_TRUE(pushResponses(sut.responseQueuePusher, QUEUE_CAPACITY));

    EXPECT_THAT(getNumberOfUsedChunks(), Ne(0U));

    sut.portRouDi.releaseAllChunks();

    // this is not part of the client port but holds the chunk from `sendRequest`
    serverRequestQueue.clear();

    EXPECT_THAT(getNumberOfUsedChunks(), Eq(0U));
}

// BEGIN Valid transitions

TEST_F(ClientPort_test, StateNotConnectedWithCaProMessageTypeOfferRemainsInStateNotConnected)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();

    auto caproMessage = CaproMessage{CaproMessageType::OFFER, sut.portData.m_serviceDescription};
    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::NOT_CONNECTED));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateNotConnectedWithCaProMessageTypeConnectTransitionsToStateConnectRequested)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();

    auto caproMessage = CaproMessage{CaproMessageType::CONNECT, sut.portData.m_serviceDescription};
    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::CONNECT_REQUESTED));
    ASSERT_TRUE(responseCaproMessage.has_value());
    EXPECT_THAT(responseCaproMessage->m_serviceDescription, Eq(sut.portData.m_serviceDescription));
    EXPECT_THAT(responseCaproMessage->m_type, Eq(iox::capro::CaproMessageType::CONNECT));
    EXPECT_THAT(responseCaproMessage->m_chunkQueueData, Eq(&sut.portData.m_chunkReceiverData));
}

TEST_F(ClientPort_test, StateConnectRequestedWithCaProMessageTypeNackTransitionsToStateWaitForOffer)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::CONNECT_REQUESTED);

    auto caproMessage = CaproMessage{CaproMessageType::NACK, sut.portData.m_serviceDescription};
    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::WAIT_FOR_OFFER));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateConnectRequestedWithCaProMessageTypeAckTransitionsToStateConnected)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::CONNECT_REQUESTED);

    auto caproMessage = CaproMessage{CaproMessageType::ACK, sut.portData.m_serviceDescription};
    caproMessage.m_chunkQueueData = &serverChunkQueueData;

    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::CONNECTED));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateWaitForOfferWithCaProMessageTypeDisconnetTransitionsToStateNotConnected)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::WAIT_FOR_OFFER);

    auto caproMessage = CaproMessage{CaproMessageType::DISCONNECT, sut.portData.m_serviceDescription};
    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::NOT_CONNECTED));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateWaitForOfferWithCaProMessageTypeOfferTransitionsToStateConnectRequested)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::WAIT_FOR_OFFER);

    auto caproMessage = CaproMessage{CaproMessageType::OFFER, sut.portData.m_serviceDescription};
    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::CONNECT_REQUESTED));
    ASSERT_TRUE(responseCaproMessage.has_value());
    EXPECT_THAT(responseCaproMessage->m_serviceDescription, Eq(sut.portData.m_serviceDescription));
    EXPECT_THAT(responseCaproMessage->m_type, Eq(iox::capro::CaproMessageType::CONNECT));
    EXPECT_THAT(responseCaproMessage->m_chunkQueueData, Eq(&sut.portData.m_chunkReceiverData));
}

TEST_F(ClientPort_test, StateConnectedWithCaProMessageTypeStopOfferTransitionsToStateWaitForOffer)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::CONNECTED);

    auto caproMessage = CaproMessage{CaproMessageType::STOP_OFFER, sut.portData.m_serviceDescription};
    caproMessage.m_chunkQueueData = &serverChunkQueueData;

    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::WAIT_FOR_OFFER));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateConnectedWithCaProMessageTypeDisconnectTransitionsToStateDisconnectRequested)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::CONNECTED);

    auto caproMessage = CaproMessage{CaproMessageType::DISCONNECT, sut.portData.m_serviceDescription};
    caproMessage.m_chunkQueueData = &serverChunkQueueData;

    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::DISCONNECT_REQUESTED));
    ASSERT_TRUE(responseCaproMessage.has_value());
    EXPECT_THAT(responseCaproMessage->m_serviceDescription, Eq(sut.portData.m_serviceDescription));
    EXPECT_THAT(responseCaproMessage->m_type, Eq(iox::capro::CaproMessageType::DISCONNECT));
    EXPECT_THAT(responseCaproMessage->m_chunkQueueData, Eq(&sut.portData.m_chunkReceiverData));
}

TEST_F(ClientPort_test, StateDisconnectRequestedWithCaProMessageTypeAckTransitionsToStateNotConnected)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::DISCONNECT_REQUESTED);

    auto caproMessage = CaproMessage{CaproMessageType::ACK, sut.portData.m_serviceDescription};
    caproMessage.m_chunkQueueData = &serverChunkQueueData;

    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::NOT_CONNECTED));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

TEST_F(ClientPort_test, StateDisconnectRequestedWithCaProMessageTypeNackTransitionsToStateNotConnected)
{
    auto& sut = initAndGetClientPortForStateTransitionTests();
    sut.portUser.connect();
    tryAdvanceToState(sut, iox::ConnectionState::DISCONNECT_REQUESTED);

    auto caproMessage = CaproMessage{CaproMessageType::NACK, sut.portData.m_serviceDescription};
    caproMessage.m_chunkQueueData = &serverChunkQueueData;

    auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);

    EXPECT_THAT(sut.portUser.getConnectionState(), Eq(iox::ConnectionState::NOT_CONNECTED));
    ASSERT_FALSE(responseCaproMessage.has_value());
}

// END Valid transitions


// BEGIN Invalid transitions

TEST_F(ClientPort_test, InvalidStateTransitionsCallErrorHandler)
{
    constexpr iox::ConnectionState ALL_STATES[]{iox::ConnectionState::NOT_CONNECTED,
                                                iox::ConnectionState::CONNECT_REQUESTED,
                                                iox::ConnectionState::WAIT_FOR_OFFER,
                                                iox::ConnectionState::CONNECTED,
                                                iox::ConnectionState::DISCONNECT_REQUESTED};

    // disable logging to prevent spamming the console with LogFatal outputs
    auto logLevelScopeGuard = iox::LoggerPosh().SetLogLevelForScope(iox::log::LogLevel::kOff);

    for (auto targetState : ALL_STATES)
    {
        for (int32_t i = 0; i < static_cast<int32_t>(CaproMessageType::MESSGAGE_TYPE_END); ++i)
        {
            auto caproMessageType = static_cast<CaproMessageType>(i);
            SCOPED_TRACE(std::string("Invalid transition test from ")
                         + iox::cxx::convert::toString(asStringLiteral(targetState)) + std::string(" with ")
                         + iox::cxx::convert::toString(asStringLiteral(caproMessageType)));

            // skip for valid transitions
            switch (targetState)
            {
            case iox::ConnectionState::NOT_CONNECTED:
                if (caproMessageType == CaproMessageType::CONNECT || caproMessageType == CaproMessageType::OFFER)
                {
                    continue;
                }
                break;
            case iox::ConnectionState::CONNECT_REQUESTED:
                if (caproMessageType == CaproMessageType::ACK || caproMessageType == CaproMessageType::NACK)
                {
                    continue;
                }
                break;
            case iox::ConnectionState::WAIT_FOR_OFFER:
                if (caproMessageType == CaproMessageType::DISCONNECT || caproMessageType == CaproMessageType::OFFER)
                {
                    continue;
                }
                break;
            case iox::ConnectionState::CONNECTED:
                if (caproMessageType == CaproMessageType::STOP_OFFER
                    || caproMessageType == CaproMessageType::DISCONNECT)
                {
                    continue;
                }
                break;
            case iox::ConnectionState::DISCONNECT_REQUESTED:
                if (caproMessageType == CaproMessageType::ACK || caproMessageType == CaproMessageType::NACK)
                {
                    continue;
                }
                break;
            }

            auto& sut = initAndGetClientPortForStateTransitionTests();
            if (targetState != iox::ConnectionState::NOT_CONNECTED)
            {
                sut.portUser.connect();
                tryAdvanceToState(sut, targetState);
            }

            iox::cxx::optional<iox::Error> detectedError;
            auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
                [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
                    detectedError.emplace(error);
                    EXPECT_EQ(errorLevel, iox::ErrorLevel::SEVERE);
                });

            auto caproMessage = CaproMessage{caproMessageType, sut.portData.m_serviceDescription};
            auto responseCaproMessage = sut.portRouDi.dispatchCaProMessageAndGetPossibleResponse(caproMessage);
            ASSERT_FALSE(responseCaproMessage.has_value());
            ASSERT_TRUE(detectedError.has_value());
            EXPECT_EQ(detectedError.value(), iox::Error::kPOPO__CAPRO_PROTOCOL_ERROR);
        }
    }
}

// END Invalid transitions

// END ClientPortRouDi tests

} // namespace
