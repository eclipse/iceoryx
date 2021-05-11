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

#include "iceoryx_posh/mepoo/chunk_settings.hpp"

#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;

using iox::mepoo::ChunkHeader;
using iox::mepoo::ChunkSettings;
using UserPayloadOffset_t = iox::mepoo::ChunkHeader::UserPayloadOffset_t;

// BEGIN GETTER METHOD TESTS

TEST(ChunkSettings_test, CallingUserPayloadSizeReturnsCorrectValue)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{42U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{128U};
    constexpr uint32_t USER_HEADER_SIZE{64U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{4U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.userPayloadSize(), Eq(USER_PAYLOAD_SIZE));
}

TEST(ChunkSettings_test, CallingUserPayloadAlignmentReturnsCorrectValue)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{42U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{128U};
    constexpr uint32_t USER_HEADER_SIZE{64U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{4U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.userPayloadAlignment(), Eq(USER_PAYLOAD_ALIGNMENT));
}

TEST(ChunkSettings_test, CallingUserHeaderSizeReturnsCorrectValue)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{42U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{128U};
    constexpr uint32_t USER_HEADER_SIZE{64U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{4U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.userHeaderSize(), Eq(USER_HEADER_SIZE));
}

TEST(ChunkSettings_test, CallingUserHeaderAlignmentReturnsCorrectValue)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{42U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{128U};
    constexpr uint32_t USER_HEADER_SIZE{64U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{4U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.userHeaderAlignment(), Eq(USER_HEADER_ALIGNMENT));
}

TEST(ChunkSettings_test, CallingRequiredChunkSizeReturnsCorrectValue)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
    constexpr uint32_t USER_HEADER_SIZE{iox::CHUNK_NO_USER_HEADER_SIZE};
    constexpr uint32_t USER_HEADER_ALIGNMENT{iox::CHUNK_NO_USER_HEADER_ALIGNMENT};

    constexpr uint32_t EXPECTED_SIZE{sizeof(ChunkHeader)};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.requiredChunkSize(), Eq(EXPECTED_SIZE));
}

// END GETTER METHOD TESTS

// BEGIN EXCEEDING CHUNK SIZE TESTS

TEST(ChunkSettings_test, NoCustomUserPayloadAlignmentAndTooLargeUserPayload_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{std::numeric_limits<uint32_t>::max()};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
    constexpr uint32_t USER_HEADER_SIZE{iox::CHUNK_NO_USER_HEADER_SIZE};
    constexpr uint32_t USER_HEADER_ALIGNMENT{iox::CHUNK_NO_USER_HEADER_ALIGNMENT};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::REQUIRED_CHUNK_SIZE_EXCEEDS_MAX_CHUNK_SIZE));
}

TEST(ChunkSettings_test, CustomUserPayloadAlignmentAndTooLargeUserPayload_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{std::numeric_limits<uint32_t>::max()};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{alignof(ChunkHeader) * 2};
    constexpr uint32_t USER_HEADER_SIZE{iox::CHUNK_NO_USER_HEADER_SIZE};
    constexpr uint32_t USER_HEADER_ALIGNMENT{iox::CHUNK_NO_USER_HEADER_ALIGNMENT};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::REQUIRED_CHUNK_SIZE_EXCEEDS_MAX_CHUNK_SIZE));
}

TEST(ChunkSettings_test, UserHeaderAndTooLargeUserPayload_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{std::numeric_limits<uint32_t>::max()};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{alignof(ChunkHeader) * 2};
    constexpr uint32_t USER_HEADER_SIZE{8U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{8U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::REQUIRED_CHUNK_SIZE_EXCEEDS_MAX_CHUNK_SIZE));
}

// END EXCEEDING CHUNK SIZE TESTS

// BEGIN INVALID USER-HEADER AND USER-PAYLOAD ALIGNMENT TESTS

TEST(ChunkSettings_test, UserPayloadAlignmentNotPowerOfTwo_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{13U};
    constexpr uint32_t USER_HEADER_SIZE{0U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{1U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::ALIGNMENT_NOT_POWER_OF_TWO));
}

TEST(ChunkSettings_test, UserHeaderAlignmentNotPowerOfTwo_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{1U};
    constexpr uint32_t USER_HEADER_SIZE{0U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{42};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::ALIGNMENT_NOT_POWER_OF_TWO));
}

TEST(ChunkSettings_test, UserHeaderAlignmentLargerThanChunkHeaderAlignment_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
    constexpr uint32_t USER_HEADER_SIZE{8U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{2 * alignof(ChunkHeader)};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::USER_HEADER_ALIGNMENT_EXCEEDS_CHUNK_HEADER_ALIGNMENT));
}

TEST(ChunkSettings_test, UserHeaderSizeNotMultipleOfAlignment_Fails)
{
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
    constexpr uint32_t USER_HEADER_SIZE{12U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{8U};

    auto sutResult =
        ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);

    ASSERT_TRUE(sutResult.has_error());
    EXPECT_THAT(sutResult.get_error(), Eq(ChunkSettings::Error::USER_HEADER_SIZE_NOT_MULTIPLE_OF_ITS_ALIGNMENT));
}

// END INVALID USER-HEADER AND USER-PAYLOAD ALIGNMENT TESTS

// BEGIN PARAMETERIZED TESTS FOR REQUIRED CHUNK SIZE

struct PayloadParams
{
    uint32_t size{0U};
    uint32_t alignment{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};

    static constexpr uint32_t MAX_ALIGNMENT{1ULL << 31};
};

class ChunkSettings_AlteringUserPayloadWithoutUserHeader : public ::testing::TestWithParam<PayloadParams>
{
};

// without a user-header, the user-payload is located right after the ChunkHeader, therefore the user-payload size and
// alignment parameters are made dependant on the ChunkHeader
INSTANTIATE_TEST_CASE_P(ChunkSettings_test,
                        ChunkSettings_AlteringUserPayloadWithoutUserHeader,
                        ::testing::Values(
                            // alignment = 0
                            PayloadParams{0U, 0U},
                            PayloadParams{1U, 0U},
                            PayloadParams{sizeof(ChunkHeader), 0U},
                            PayloadParams{sizeof(ChunkHeader) * 42U, 0U},
                            // alignment = 1
                            PayloadParams{0U, 1U},
                            PayloadParams{1U, 1U},
                            PayloadParams{sizeof(ChunkHeader), 1U},
                            PayloadParams{sizeof(ChunkHeader) * 42U, 1U},
                            // alignment = alignof(ChunkHeader) / 2
                            PayloadParams{0U, alignof(ChunkHeader) / 2},
                            PayloadParams{1U, alignof(ChunkHeader) / 2},
                            PayloadParams{sizeof(ChunkHeader), alignof(ChunkHeader) / 2},
                            PayloadParams{sizeof(ChunkHeader) * 42U, alignof(ChunkHeader) / 2},
                            // alignment = alignof(ChunkHeader)
                            PayloadParams{0U, alignof(ChunkHeader)},
                            PayloadParams{1U, alignof(ChunkHeader)},
                            PayloadParams{sizeof(ChunkHeader), alignof(ChunkHeader)},
                            PayloadParams{sizeof(ChunkHeader) * 42U, alignof(ChunkHeader)},
                            // alignment = alignof(ChunkHeader) * 2
                            PayloadParams{0U, alignof(ChunkHeader) * 2},
                            PayloadParams{1U, alignof(ChunkHeader) * 2},
                            PayloadParams{sizeof(ChunkHeader), alignof(ChunkHeader) * 2},
                            PayloadParams{sizeof(ChunkHeader) * 42U, alignof(ChunkHeader) * 2},
                            // alignment = PayloadParams::MAX_ALIGNMENT
                            PayloadParams{0U, PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{1U, PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{sizeof(ChunkHeader), PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{sizeof(ChunkHeader) * 42U, PayloadParams::MAX_ALIGNMENT}));

TEST_P(ChunkSettings_AlteringUserPayloadWithoutUserHeader, RequiredChunkSizeIsCorrect)
{
    const auto userPayload = GetParam();

    SCOPED_TRACE(std::string("User-Payload: size = ") + std::to_string(userPayload.size) + std::string("; alignment = ")
                 + std::to_string(userPayload.alignment));

    const uint32_t expectedSize = [&userPayload] {
        if (userPayload.alignment <= alignof(ChunkHeader))
        {
            // user-payload is always adjacent
            return static_cast<uint32_t>(sizeof(ChunkHeader)) + userPayload.size;
        }
        else
        {
            // user-payload is not necessarily adjacent
            auto preUserPayloadAlignmentOverhangOfChunkHeder = sizeof(ChunkHeader) - alignof(ChunkHeader);
            return static_cast<uint32_t>(preUserPayloadAlignmentOverhangOfChunkHeder) + userPayload.alignment
                   + userPayload.size;
        }
    }();

    auto sutResult = ChunkSettings::create(userPayload.size, userPayload.alignment);
    ASSERT_FALSE(sutResult.has_error());
    auto& sut = sutResult.value();

    EXPECT_THAT(sut.requiredChunkSize(), Eq(expectedSize));
}

class ChunkSettings_AlteringUserPayloadWithUserHeader : public ::testing::TestWithParam<PayloadParams>
{
};

// with a user-header, the user-payload is located right after the UserPayloadOffset_t, therefore the user-payload size
// and alignment parameters are made dependant on the UserPayloadOffset_t
INSTANTIATE_TEST_CASE_P(ChunkSettings_test,
                        ChunkSettings_AlteringUserPayloadWithUserHeader,
                        ::testing::Values(
                            // alignment = 0
                            PayloadParams{0U, 0U},
                            PayloadParams{1U, 0U},
                            PayloadParams{sizeof(UserPayloadOffset_t), 0U},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, 0U},
                            // alignment = 1
                            PayloadParams{0U, 1U},
                            PayloadParams{1U, 1U},
                            PayloadParams{sizeof(UserPayloadOffset_t), 1U},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, 1U},
                            // alignment = alignof(UserPayloadOffset_t) / 2
                            PayloadParams{0U, alignof(UserPayloadOffset_t) / 2},
                            PayloadParams{1U, alignof(UserPayloadOffset_t) / 2},
                            PayloadParams{sizeof(UserPayloadOffset_t), alignof(UserPayloadOffset_t) / 2},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, alignof(UserPayloadOffset_t) / 2},
                            // alignment = alignof(UserPayloadOffset_t)
                            PayloadParams{0U, alignof(UserPayloadOffset_t)},
                            PayloadParams{1U, alignof(UserPayloadOffset_t)},
                            PayloadParams{sizeof(UserPayloadOffset_t), alignof(UserPayloadOffset_t)},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, alignof(UserPayloadOffset_t)},
                            // alignment = alignof(UserPayloadOffset_t) * 2
                            PayloadParams{0U, alignof(UserPayloadOffset_t) * 2},
                            PayloadParams{1U, alignof(UserPayloadOffset_t) * 2},
                            PayloadParams{sizeof(UserPayloadOffset_t), alignof(UserPayloadOffset_t) * 2},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, alignof(UserPayloadOffset_t) * 2},
                            // alignment = PayloadParams::MAX_ALIGNMENT
                            PayloadParams{0U, PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{1U, PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{sizeof(UserPayloadOffset_t), PayloadParams::MAX_ALIGNMENT},
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, PayloadParams::MAX_ALIGNMENT}));

uint32_t expectedChunkSizeWithUserHeader(const PayloadParams& userPayload, uint32_t userHeaderSize)
{
    const uint32_t userHeaderSizeAndPaddingToBackOffset =
        iox::algorithm::max(userHeaderSize, static_cast<uint32_t>(alignof(UserPayloadOffset_t)));

    if (userPayload.alignment <= alignof(UserPayloadOffset_t))
    {
        // back-offset is always adjacent to the user-header (as much as possible with the alignment constraints)
        constexpr uint32_t BACK_OFFSET_SIZE{sizeof(UserPayloadOffset_t)};
        return static_cast<uint32_t>(sizeof(ChunkHeader)) + userHeaderSizeAndPaddingToBackOffset + BACK_OFFSET_SIZE
               + userPayload.size;
    }
    else
    {
        // back-offset is not necessarily adjacent to the user-header
        const uint32_t paddingBytesAndBackOffsetSize = userPayload.alignment;
        return static_cast<uint32_t>(sizeof(ChunkHeader)) + userHeaderSizeAndPaddingToBackOffset
               + paddingBytesAndBackOffsetSize + userPayload.size;
    }
}

TEST_P(ChunkSettings_AlteringUserPayloadWithUserHeader,
       MultipleUserHeaderSizesAndAlignments_ResultsIn_RequiredChunkSizeIsCorrect)
{
    const auto userPayload = GetParam();

    SCOPED_TRACE(std::string("User-Payload: size = ") + std::to_string(userPayload.size) + std::string("; alignment = ")
                 + std::to_string(userPayload.alignment));

    constexpr uint32_t SMALL_USER_HEADER{alignof(ChunkHeader)};
    static_assert(SMALL_USER_HEADER < sizeof(ChunkHeader), "For this test the size must be smaller than ChunkHeader");
    constexpr uint32_t USER_HEADER_SIZES[]{1U, SMALL_USER_HEADER, sizeof(ChunkHeader), sizeof(ChunkHeader) * 2U};
    constexpr uint32_t USER_HEADER_ALIGNMENTS[]{0U, 1U, alignof(ChunkHeader) / 2U, alignof(ChunkHeader)};

    for (const auto userHeaderAlignment : USER_HEADER_ALIGNMENTS)
    {
        SCOPED_TRACE(std::string("User-Header alignment = ") + std::to_string(userHeaderAlignment));

        for (const auto userHeaderSize : USER_HEADER_SIZES)
        {
            SCOPED_TRACE(std::string("User-Header size = ") + std::to_string(userHeaderSize));

            if (userHeaderSize < userHeaderAlignment)
            {
                // the size must always be a multiple of the alignment
                continue;
            }

            const uint32_t expectedSize = expectedChunkSizeWithUserHeader(userPayload, userHeaderSize);

            auto sutResult =
                ChunkSettings::create(userPayload.size, userPayload.alignment, userHeaderSize, userHeaderAlignment);
            ASSERT_FALSE(sutResult.has_error());
            auto& sut = sutResult.value();

            EXPECT_THAT(sut.requiredChunkSize(), Eq(expectedSize));
        }
    }
}

// END PARAMETERIZED TESTS FOR REQUIRED CHUNK SIZE

} // namespace
