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

#if !defined(_WIN32)
#include "iceoryx_utils/internal/posix_wrapper/message_queue.hpp"
#include "iceoryx_utils/internal/posix_wrapper/unix_domain_socket.hpp"

#include "test.hpp"

#include <chrono>

namespace
{
using namespace ::testing;
using namespace iox;
using namespace iox::posix;

#if defined(__APPLE__)
using IpcChannelTypes = Types<UnixDomainSocket>;
#else
using IpcChannelTypes = Types<MessageQueue, UnixDomainSocket>;
#endif

constexpr char goodName[] = "channel_test";
constexpr char anotherGoodName[] = "horst";
constexpr char theUnknown[] = "WhoeverYouAre";
constexpr char slashName[] = "/miau";

/// @req
/// @brief This test suite verifies that the abstract interface IpcChannelType is fulfilled by both the UnixDomainSocket
/// class and the MessageQueue class
/// @pre server and client object are allocated and move to the member object of the fixture
/// @post StdErr is capture and outputed to StdCout
/// @note Specific functionality of the underlying implementations of an IpcChannelType are tested in
/// "UnixDomainSocket_test"
template <typename T>
class IpcChannel_test : public Test
{
  public:
    using IpcChannelType = T;

    void SetUp()
    {
        auto serverResult = IpcChannelType::create(
            goodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER, MaxMsgSize, MaxMsgNumber);
        ASSERT_THAT(serverResult.has_error(), Eq(false));
        server = std::move(serverResult.value());
        internal::CaptureStderr();

        auto clientResult = IpcChannelType::create(
            goodName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT, MaxMsgSize, MaxMsgNumber);
        ASSERT_THAT(clientResult.has_error(), Eq(false));
        client = std::move(clientResult.value());
    }

    void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    ~IpcChannel_test()
    {
    }

    static const size_t MaxMsgSize;
    static constexpr uint64_t MaxMsgNumber = 10U;
    IpcChannelType server;
    IpcChannelType client;
};

template <typename T>
const size_t IpcChannel_test<T>::MaxMsgSize = IpcChannelType::MAX_MESSAGE_SIZE;
template <typename T>
constexpr uint64_t IpcChannel_test<T>::MaxMsgNumber;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(IpcChannel_test, IpcChannelTypes);
#pragma GCC diagnostic pop

TYPED_TEST(IpcChannel_test, CreateWithTooLargeMessageSizeLeadsToError)
{
    auto serverResult = TestFixture::IpcChannelType::create(goodName,
                                                            IpcChannelMode::BLOCKING,
                                                            IpcChannelSide::SERVER,
                                                            TestFixture::MaxMsgSize + 1,
                                                            TestFixture::MaxMsgNumber);
    EXPECT_TRUE(serverResult.has_error());
    ASSERT_THAT(serverResult.get_error(), Eq(IpcChannelError::MAX_MESSAGE_SIZE_EXCEEDED));
}

TYPED_TEST(IpcChannel_test, CreateNoNameLeadsToError)
{
    auto serverResult = TestFixture::IpcChannelType::create("", IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_TRUE(serverResult.has_error());
    ASSERT_THAT(serverResult.get_error(), Eq(IpcChannelError::INVALID_CHANNEL_NAME));
}

TYPED_TEST(IpcChannel_test, CreateWithLeadingSlashWorks)
{
    auto serverResult =
        TestFixture::IpcChannelType::create(slashName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(serverResult.has_error());
}

TYPED_TEST(IpcChannel_test, CreateAgainWorks)
{
    // if there is a leftover from a crashed channel, we can create a new one. This is simulated by creating twice
    auto first = TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(first.has_error());
    auto second =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(second.has_error());
}


TYPED_TEST(IpcChannel_test, CreateAgainAndEmptyWorks)
{
    using namespace iox::units;
    using namespace std::chrono;

    auto serverResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(serverResult.has_error());
    auto server = std::move(serverResult.value());

    auto clientResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT);
    EXPECT_FALSE(clientResult.has_error());
    auto client = std::move(clientResult.value());

    // send and receive as usual
    std::string message = "Hey, I'm talking to you";
    bool sent = client.send(message).has_error();
    EXPECT_FALSE(sent);

    auto receivedMessage = server.receive();
    ASSERT_THAT(receivedMessage.has_error(), Eq(false));
    EXPECT_EQ(message, *receivedMessage);

    // send a message, create the server again and there is no message
    std::string newMessage = "I'm still talking to you brother";
    sent = client.send(newMessage).has_error();
    EXPECT_FALSE(sent);

    auto second =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(second.has_error());
    server = std::move(second.value());

    Duration timeout = 100_ms;
    auto received = server.timedReceive(timeout);
    ASSERT_TRUE(received.has_error());
    ASSERT_THAT(received.get_error(), Eq(IpcChannelError::TIMEOUT));
}

TYPED_TEST(IpcChannel_test, ClientWithoutServerLeadsToNoSuchChannelError)
{
    auto clientResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT);
    EXPECT_TRUE(clientResult.has_error());
    ASSERT_THAT(clientResult.get_error(), Eq(IpcChannelError::NO_SUCH_CHANNEL));
}


TYPED_TEST(IpcChannel_test, NotDestroyingServerLeadsToNonOutdatedClient)
{
    auto serverResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(serverResult.has_error());
    auto server = std::move(serverResult.value());

    auto clientResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT);
    EXPECT_FALSE(clientResult.has_error());
    auto client = std::move(clientResult.value());

    auto outdated = client.isOutdated();
    EXPECT_FALSE(outdated.has_error());
    EXPECT_FALSE(outdated.value());
}


TYPED_TEST(IpcChannel_test, DestroyingServerLeadsToOutdatedClient)
{
    if (std::is_same<typename TestFixture::IpcChannelType, UnixDomainSocket>::value)
    {
        // isOutdated cannot be realized for unix domain sockets
        return;
    }

    auto serverResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(serverResult.has_error());
    auto server = std::move(serverResult.value());

    auto clientResult =
        TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::CLIENT);
    EXPECT_FALSE(clientResult.has_error());
    auto client = std::move(clientResult.value());

    // destroy the server and the client is outdated
    auto dest = server.destroy();
    ASSERT_FALSE(dest.has_error());

    auto outdated = client.isOutdated();
    EXPECT_FALSE(outdated.has_error());
    EXPECT_TRUE(outdated.value());
}

TYPED_TEST(IpcChannel_test, UnlinkExistingOneWorks)
{
    auto first = TestFixture::IpcChannelType::create(anotherGoodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER);
    EXPECT_FALSE(first.has_error());
    auto ret = TestFixture::IpcChannelType::unlinkIfExists(anotherGoodName);
    ASSERT_FALSE(ret.has_error());
    EXPECT_TRUE(ret.value());
}

TYPED_TEST(IpcChannel_test, UnlinkNonExistingOneWorks)
{
    auto ret = TestFixture::IpcChannelType::unlinkIfExists(theUnknown);
    ASSERT_FALSE(ret.has_error());
    EXPECT_FALSE(ret.value());
}

TYPED_TEST(IpcChannel_test, SendAndReceiveWorks)
{
    std::string message = "Hey, I'm talking to you";
    bool sent = this->client.send(message).has_error();
    EXPECT_FALSE(sent);

    std::string anotherMessage = "This is a message";
    sent = this->client.send(anotherMessage).has_error();
    EXPECT_FALSE(sent);

    auto receivedMessage = this->server.receive();
    ASSERT_THAT(receivedMessage.has_error(), Eq(false));
    EXPECT_EQ(message, *receivedMessage);

    receivedMessage = this->server.receive();
    ASSERT_THAT(receivedMessage.has_error(), Eq(false));
    EXPECT_EQ(anotherMessage, *receivedMessage);
}

TYPED_TEST(IpcChannel_test, InvalidAfterDestroy)
{
    ASSERT_FALSE(this->client.destroy().has_error());
    ASSERT_FALSE(this->client.isInitialized());
    ASSERT_FALSE(this->server.destroy().has_error());
    ASSERT_FALSE(this->server.isInitialized());
}

TYPED_TEST(IpcChannel_test, SendAfterClientDestroyLeadsToError)
{
    auto dest = this->client.destroy();
    ASSERT_FALSE(dest.has_error());

    std::string message = "Should never be sent";
    bool sendError = this->client.send(message).has_error();
    EXPECT_TRUE(sendError);
}

TYPED_TEST(IpcChannel_test, SendAfterServerDestroyLeadsToError)
{
    if (std::is_same<typename TestFixture::IpcChannelType, MessageQueue>::value)
    {
        // We still can send to the message queue is we destroy the server
        // it would be outdated, this is checked in another test
        return;
    }

    auto dest = this->server.destroy();
    ASSERT_FALSE(dest.has_error());

    std::string message = "Try to send me";
    auto sendResult = this->client.send(message);
    EXPECT_TRUE(sendResult.has_error());
}


TYPED_TEST(IpcChannel_test, ReceiveAfterServerDestroyLeadsToError)
{
    std::string message = "hello world!";
    bool sendError = this->client.send(message).has_error();
    EXPECT_FALSE(sendError);

    auto dest = this->server.destroy();
    ASSERT_FALSE(dest.has_error());

    bool receiveError = this->server.receive().has_error();
    EXPECT_THAT(receiveError, Eq(true));
}

TYPED_TEST(IpcChannel_test, SendMoreThanAllowedLeadsToError)
{
    std::string shortMessage = "Iceoryx rules.";
    ASSERT_THAT(this->client.send(shortMessage).has_error(), Eq(false));

    std::string longMessage(this->server.MAX_MESSAGE_SIZE + 8, 'x');
    ASSERT_THAT(this->client.send(longMessage).has_error(), Eq(true));

    auto receivedMessage = this->server.receive();
    ASSERT_THAT(receivedMessage.has_error(), Eq(false));
    EXPECT_EQ(shortMessage, receivedMessage.value());
}

TYPED_TEST(IpcChannel_test, SendMaxMessageSizeWorks)
{
    std::string message(this->MaxMsgSize - 1, 'x');
    auto clientReturn = this->client.send(message);
    ASSERT_THAT(clientReturn.has_error(), Eq(false));

    auto receivedMessage = this->server.receive();
    ASSERT_THAT(receivedMessage.has_error(), Eq(false));
    EXPECT_EQ(message, receivedMessage.value());
}

TYPED_TEST(IpcChannel_test, wildCreate)
{
    auto result = TestFixture::IpcChannelType::create();
    ASSERT_THAT(result.has_error(), Eq(true));
}

#if !defined(__APPLE__)
TYPED_TEST(IpcChannel_test, TimedSendWorks)
{
    using namespace iox::units;
    using namespace std::chrono;

    std::string msg = "ISG rules. And some more                                                                        "
                      "data to have a bit                                                                              "
                      "longer message";

    Duration maxTimeout = 100_ms;
    Duration minTimeoutTolerance = 10_ms;
    Duration maxTimeoutTolerance = 20_ms;

    // send till it breaks
    for (;;)
    {
        auto before = system_clock::now();
        auto result = this->client.timedSend(msg, maxTimeout);
        auto after = system_clock::now();
        if (result.has_error())
        {
            ASSERT_THAT(result.get_error(), Eq(IpcChannelError::TIMEOUT));
            // Do not exceed timeout
            auto timeDiff = units::Duration(after - before);
            EXPECT_LT(timeDiff, maxTimeout + maxTimeoutTolerance);

            // Check if timedSend has blocked for ~maxTimeout and has not returned immediately
            EXPECT_GT(timeDiff, maxTimeout - minTimeoutTolerance);

            break;
        }
    }
}
#endif

TYPED_TEST(IpcChannel_test, TimedReceiveWorks)
{
    using namespace iox::units;
    using namespace std::chrono;

    std::string msg = "very useful text for tranmission";
    Duration timeout = 100_ms;
    Duration minTimeoutTolerance = 10_ms;
    Duration maxTimeoutTolerance = 20_ms;

    ASSERT_FALSE(this->client.send(msg).has_error());

    auto received = this->server.timedReceive(timeout);
    ASSERT_FALSE(received.has_error());

    EXPECT_EQ(received.value(), msg);

    auto before = system_clock::now();
    received = this->server.timedReceive(timeout);
    auto after = system_clock::now();

    ASSERT_TRUE(received.has_error());
    ASSERT_THAT(received.get_error(), Eq(IpcChannelError::TIMEOUT));

    // Do not exceed timeout
    auto timeDiff = units::Duration(after - before);
    EXPECT_LT(timeDiff, timeout + maxTimeoutTolerance);

    // Check if timedReceive has blocked for ~timeout and has not returned immediately
    EXPECT_GT(timeDiff, timeout - minTimeoutTolerance);
}
} // namespace
#endif
