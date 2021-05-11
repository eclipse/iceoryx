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

extern "C" {
#include "iceoryx_binding_c/chunk.h"
#include "iceoryx_binding_c/publisher.h"
#include "iceoryx_binding_c/runtime.h"
}

#include "iceoryx_posh/testing/roudi_gtest.hpp"

namespace
{
using namespace iox;
using namespace iox::mepoo;


class Chunk_test : public RouDi_GTest
{
  public:
    void SetUp() override
    {
        iox_runtime_init("hypnotoad");
        publisher = iox_pub_init(&publisherStorage, "All", "Glory", "Hypnotoad", nullptr);
        ASSERT_NE(publisher, nullptr);
    }

    void TearDown() override
    {
    }

    iox_pub_storage_t publisherStorage;
    iox_pub_t publisher{nullptr};
};

TEST_F(Chunk_test, GettingChunkHeaderFromNonConstUserPayloadWorks)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_chunk(publisher, &userPayload, USER_PAYLOAD_SIZE), AllocationResult_SUCCESS);

    auto chunkHeader = iox_chunk_header_from_user_payload(userPayload);
    ASSERT_NE(chunkHeader, nullptr);

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(chunkHeader)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(userPayload)};
    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST_F(Chunk_test, GettingChunkHeaderFromConstUserPayloadWorks)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_chunk(publisher, &userPayload, USER_PAYLOAD_SIZE), AllocationResult_SUCCESS);
    const void* constUserPayload = userPayload;

    auto chunkHeader = iox_chunk_header_from_user_payload_const(constUserPayload);
    ASSERT_NE(chunkHeader, nullptr);

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(chunkHeader)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(userPayload)};
    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST_F(Chunk_test, UserPayloadChunkHeaderUserPayloadRoundtripWorksForNonConst)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_chunk(publisher, &userPayload, USER_PAYLOAD_SIZE), AllocationResult_SUCCESS);
    const void* constUserPayload = userPayload;

    auto chunkHeader = iox_chunk_header_from_user_payload_const(constUserPayload);
    auto userPayloadRoundtrip = iox_chunk_header_to_user_payload_const(chunkHeader);

    EXPECT_EQ(userPayloadRoundtrip, userPayload);
}

TEST_F(Chunk_test, UserPayloadChunkHeaderUserPayloadRoundtripWorksForConst)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_chunk(publisher, &userPayload, USER_PAYLOAD_SIZE), AllocationResult_SUCCESS);

    auto chunkHeader = iox_chunk_header_from_user_payload(userPayload);
    auto userPayloadRoundtrip = iox_chunk_header_to_user_payload(chunkHeader);

    EXPECT_EQ(userPayloadRoundtrip, userPayload);
}

TEST_F(Chunk_test, GettingUserHeaderFromNonConstChunkHeaderWorks)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT(64U);
    constexpr uint32_t USER_HEADER_SIZE = 16U;
    constexpr uint32_t USER_HEADER_ALIGNMENT = 8U;
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_aligned_chunk_with_user_header(publisher,
                                                          &userPayload,
                                                          USER_PAYLOAD_SIZE,
                                                          USER_PAYLOAD_ALIGNMENT,
                                                          USER_HEADER_SIZE,
                                                          USER_HEADER_ALIGNMENT),
              AllocationResult_SUCCESS);

    auto chunkHeader = iox_chunk_header_from_user_payload(userPayload);

    auto userHeader = iox_chunk_header_to_user_header(chunkHeader);

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(chunkHeader)};
    const uint64_t userHeaderStartAddress{reinterpret_cast<uint64_t>(userHeader)};
    EXPECT_THAT(userHeaderStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST_F(Chunk_test, GettingUserHeaderFromConstChunkHeaderWorks)
{
    constexpr uint32_t USER_PAYLOAD_SIZE(42U);
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT(64U);
    constexpr uint32_t USER_HEADER_SIZE = 16U;
    constexpr uint32_t USER_HEADER_ALIGNMENT = 8U;
    void* userPayload{nullptr};
    ASSERT_EQ(iox_pub_loan_aligned_chunk_with_user_header(publisher,
                                                          &userPayload,
                                                          USER_PAYLOAD_SIZE,
                                                          USER_PAYLOAD_ALIGNMENT,
                                                          USER_HEADER_SIZE,
                                                          USER_HEADER_ALIGNMENT),
              AllocationResult_SUCCESS);

    const iox_chunk_header_t* chunkHeader = iox_chunk_header_from_user_payload(userPayload);

    auto userHeader = iox_chunk_header_to_user_header_const(chunkHeader);

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(chunkHeader)};
    const uint64_t userHeaderStartAddress{reinterpret_cast<uint64_t>(userHeader)};
    EXPECT_THAT(userHeaderStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

} // namespace
