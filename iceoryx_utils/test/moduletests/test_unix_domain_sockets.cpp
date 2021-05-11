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
using namespace iox::units::duration_literals;
using namespace iox::units;

constexpr char invalidName[] = "x";
constexpr char goodName[] = "channel_test";

/// @req
/// @brief This test suite verifies the functionality which is specific to the UnixDomainSocket class
/// @pre server and client object are allocated and move to the member object of the fixture
/// @post StdErr is capture and outputed to StdCout
/// @note Most of the UnixDomainSocket functionality is tested in "IpcChannel_test"
class UnixDomainSocket_test : public Test
{
  public:
    void SetUp()
    {
        auto serverResult = UnixDomainSocket::create(
            goodName, IpcChannelMode::BLOCKING, IpcChannelSide::SERVER, MaxMsgSize, MaxMsgNumber);
        ASSERT_THAT(serverResult.has_error(), Eq(false));
        server = std::move(serverResult.value());
        internal::CaptureStderr();

        auto clientResult = UnixDomainSocket::create(
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

    ~UnixDomainSocket_test()
    {
    }

    static const size_t MaxMsgSize;
    static constexpr uint64_t MaxMsgNumber = 10U;
    UnixDomainSocket server;
    UnixDomainSocket client;
};

const size_t UnixDomainSocket_test::MaxMsgSize = UnixDomainSocket::MAX_MESSAGE_SIZE;
constexpr uint64_t UnixDomainSocket_test::MaxMsgNumber;

TEST_F(UnixDomainSocket_test, NonBlockingModeNotSupported)
{
    auto result = UnixDomainSocket::create(
        goodName, IpcChannelMode::NON_BLOCKING, IpcChannelSide::SERVER, MaxMsgSize, MaxMsgNumber);
    EXPECT_TRUE(result.has_error());
    ASSERT_THAT(result.get_error(), Eq(IpcChannelError::INVALID_ARGUMENTS));
}

TEST_F(UnixDomainSocket_test, UnlinkNonExistingWithInvalidNameLeadsToError)
{
    auto ret = UnixDomainSocket::unlinkIfExists(UnixDomainSocket::NoPathPrefix, invalidName);
    EXPECT_TRUE(ret.has_error());
    ASSERT_THAT(ret.get_error(), Eq(IpcChannelError::INVALID_CHANNEL_NAME));
}

TEST_F(UnixDomainSocket_test, SendingOnServerLeadsToError)
{
    cxx::string<10> message{"Foo"};
    Duration duration{1_ms};
    auto result = server.timedSend(message, duration);
    EXPECT_TRUE(result.has_error());
    ASSERT_THAT(result.get_error(), Eq(IpcChannelError::INTERNAL_LOGIC_ERROR));
}

TEST_F(UnixDomainSocket_test, ReceivingOnClientLeadsToError)
{
    Duration duration{1_ms};
    auto result = client.timedReceive(duration);
    EXPECT_TRUE(result.has_error());
    ASSERT_THAT(result.get_error(), Eq(IpcChannelError::INTERNAL_LOGIC_ERROR));
}
} // namespace
#endif
