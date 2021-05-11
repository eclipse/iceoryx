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

#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/unique_ptr.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::mepoo;

using UserPayloadOffset_t = ChunkHeader::UserPayloadOffset_t;

TEST(ChunkHeader_test, ChunkHeaderHasInitializedMembers)
{
    constexpr uint32_t CHUNK_SIZE{753U};
    constexpr uint32_t USER_PAYLOAD_SIZE{8U};
    constexpr uint32_t USER_PAYLOAD_ALIGNMENT{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    ChunkHeader sut{CHUNK_SIZE, chunkSettings};

    EXPECT_THAT(sut.chunkSize(), Eq(CHUNK_SIZE));

    // deliberately used a magic number to make the test fail when CHUNK_HEADER_VERSION changes
    EXPECT_THAT(sut.chunkHeaderVersion(), Eq(1U));

    EXPECT_THAT(sut.originId(), Eq(iox::UniquePortId(iox::popo::InvalidId)));

    EXPECT_THAT(sut.sequenceNumber(), Eq(0U));

    EXPECT_THAT(sut.userHeaderId(), Eq(ChunkHeader::NO_USER_HEADER));
    EXPECT_THAT(sut.userHeaderSize(), Eq(0U));
    EXPECT_THAT(sut.userPayloadSize(), Eq(USER_PAYLOAD_SIZE));
    EXPECT_THAT(sut.userPayloadAlignment(), Eq(USER_PAYLOAD_ALIGNMENT));

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};
    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, ChunkHeaderUserPayloadSizeTypeIsLargeEnoughForMempoolChunk)
{
    using ChunkSize_t = std::result_of<decltype (&MemPool::getChunkSize)(MemPool)>::type;

    auto maxOfChunkSizeType = std::numeric_limits<ChunkSize_t>::max();
    auto maxOfUserPayloadSizeType = std::numeric_limits<decltype(std::declval<ChunkHeader>().userPayloadSize())>::max();

    // the user-payload will never be larger than the chunk
    // if the user-payload type can hold at least the maximum chunk size there will never be an overflow
    EXPECT_THAT(maxOfUserPayloadSizeType, Ge(maxOfChunkSizeType));
}

TEST(ChunkHeader_test, UserPayloadFunctionCalledFromNonConstChunkHeaderWorks)
{
    constexpr uint32_t CHUNK_SIZE{753U};
    constexpr uint32_t USER_PAYLOAD_SIZE{8U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    ChunkHeader sut{CHUNK_SIZE, chunkSettings};

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};
    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, UserPayloadFunctionCalledFromConstChunkHeaderWorks)
{
    constexpr uint32_t CHUNK_SIZE{753U};
    constexpr uint32_t USER_PAYLOAD_SIZE{8U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    const ChunkHeader sut{CHUNK_SIZE, chunkSettings};

    // a default created ChunkHeader has always an adjacent user-payload
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};
    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, UserPayloadFunctionCalledFromNonConstChunkHeaderReturnsNonConstType)
{
    auto isNonConstReturn = std::is_same<decltype(std::declval<ChunkHeader>().userPayload()), void*>::value;
    EXPECT_TRUE(isNonConstReturn);
}

TEST(ChunkHeader_test, UserPayloadFunctionCalledFromConstChunkHeaderReturnsConstType)
{
    auto isConstReturn = std::is_same<decltype(std::declval<const ChunkHeader>().userPayload()), const void*>::value;
    EXPECT_TRUE(isConstReturn);
}

TEST(ChunkHeader_test, UserHeaderFunctionCalledFromNonConstChunkHeaderWorks)
{
    alignas(ChunkHeader) static uint8_t storage[1024 * 1024];

    constexpr uint32_t CHUNK_SIZE{753U};
    constexpr uint32_t USER_PAYLOAD_SIZE{8U};
    constexpr uint32_t USER_HEADER_SIZE{16U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{8U};

    auto chunkSettingsResult = ChunkSettings::create(
        USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    iox::cxx::unique_ptr<ChunkHeader> sut{new (storage) ChunkHeader(CHUNK_SIZE, chunkSettings), [](ChunkHeader*) {}};

    // the user-header is always adjacent to the ChunkHeader
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(sut.get())};
    const uint64_t userHeaderStartAddress{reinterpret_cast<uint64_t>(sut->userHeader())};
    EXPECT_THAT(userHeaderStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, UserHeaderFunctionCalledFromConstChunkHeaderWorks)
{
    alignas(ChunkHeader) static uint8_t storage[1024 * 1024];

    constexpr uint32_t CHUNK_SIZE{753U};
    constexpr uint32_t USER_PAYLOAD_SIZE{8U};
    constexpr uint32_t USER_HEADER_SIZE{16U};
    constexpr uint32_t USER_HEADER_ALIGNMENT{8U};

    auto chunkSettingsResult = ChunkSettings::create(
        USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    iox::cxx::unique_ptr<const ChunkHeader> sut{new (storage) ChunkHeader(CHUNK_SIZE, chunkSettings),
                                                [](const ChunkHeader*) {}};

    // the user-header is always adjacent to the ChunkHeader
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(sut.get())};
    const uint64_t userHeaderStartAddress{reinterpret_cast<uint64_t>(sut->userHeader())};
    EXPECT_THAT(userHeaderStartAddress - chunkStartAddress, Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, UserHeaderFunctionCalledFromNonConstChunkHeaderReturnsNonConstType)
{
    auto isNonConstReturn = std::is_same<decltype(std::declval<ChunkHeader>().userHeader()), void*>::value;
    EXPECT_TRUE(isNonConstReturn);
}

TEST(ChunkHeader_test, UserHeaderFunctionCalledFromConstChunkHeaderReturnsConstType)
{
    auto isConstReturn = std::is_same<decltype(std::declval<const ChunkHeader>().userHeader()), const void*>::value;
    EXPECT_TRUE(isConstReturn);
}

TEST(ChunkHeader_test, FromUserPayloadFunctionCalledWithNullptrReturnsNullptr)
{
    constexpr void* USER_PAYLOAD{nullptr};
    auto chunkHeader = ChunkHeader::fromUserPayload(USER_PAYLOAD);
    EXPECT_THAT(chunkHeader, Eq(nullptr));
}

TEST(ChunkHeader_test, FromUserPayloadFunctionCalledWithConstNullptrReturnsNullptr)
{
    constexpr const void* USER_PAYLOAD{nullptr};
    auto chunkHeader = ChunkHeader::fromUserPayload(USER_PAYLOAD);
    EXPECT_THAT(chunkHeader, Eq(nullptr));
}

TEST(ChunkHeader_test, FromUserPayloadFunctionCalledWithNonConstParamReturnsNonConstType)
{
    auto isNonConstReturn =
        std::is_same<decltype(ChunkHeader::fromUserPayload(std::declval<void*>())), ChunkHeader*>::value;
    EXPECT_TRUE(isNonConstReturn);
}

TEST(ChunkHeader_test, FromUserPayloadFunctionCalledWithConstParamReturnsConstType)
{
    auto isConstReturn =
        std::is_same<decltype(ChunkHeader::fromUserPayload(std::declval<const void*>())), const ChunkHeader*>::value;
    EXPECT_TRUE(isConstReturn);
}

TEST(ChunkHeader_test, UsedChunkSizeIsSizeOfChunkHeaderWhenUserPayloadIsZero)
{
    constexpr uint32_t CHUNK_SIZE{2 * sizeof(ChunkHeader)};
    constexpr uint32_t USER_PAYLOAD_SIZE{0U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    ChunkHeader sut{CHUNK_SIZE, chunkSettings};

    EXPECT_THAT(sut.usedSizeOfChunk(), Eq(sizeof(ChunkHeader)));
}

TEST(ChunkHeader_test, UsedChunkSizeIsSizeOfChunkHeaderPlusOneWhenUserPayloadIsOne)
{
    constexpr uint32_t CHUNK_SIZE{2 * sizeof(ChunkHeader)};
    constexpr uint32_t USER_PAYLOAD_SIZE{1U};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    ChunkHeader sut{CHUNK_SIZE, chunkSettings};

    EXPECT_THAT(sut.usedSizeOfChunk(), Eq(sizeof(ChunkHeader) + USER_PAYLOAD_SIZE));
}

TEST(ChunkHeader_test, ConstructorTerminatesWhenUserPayloadSizeExceedsChunkSize)
{
    constexpr uint32_t CHUNK_SIZE{128U};
    constexpr uint32_t USER_PAYLOAD_SIZE{2U * CHUNK_SIZE};

    auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(chunkSettingsResult.has_error());
    auto& chunkSettings = chunkSettingsResult.value();

    EXPECT_DEATH({ ChunkHeader sut(CHUNK_SIZE, chunkSettings); }, ".*");
}

// BEGIN PARAMETERIZED TESTS FOR CHUNK HEADER

struct PayloadParams
{
    uint32_t size{0U};
    uint32_t alignment{iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT};
};


void createChunksOnMultipleAddresses(const PayloadParams& userPayloadParams,
                                     const uint32_t userHeaderSize,
                                     const uint32_t userHeaderAlignment,
                                     const std::function<void(ChunkHeader& chunkHeader)> testHook)
{
    ASSERT_TRUE(testHook);

    constexpr size_t MAX_USER_PAYLOAD_ALIGNMENT_FOR_TEST{512U};
    ASSERT_THAT(MAX_USER_PAYLOAD_ALIGNMENT_FOR_TEST, Gt(alignof(ChunkHeader)));

    constexpr size_t STORAGE_ALIGNMENT{2 * MAX_USER_PAYLOAD_ALIGNMENT_FOR_TEST};
    alignas(STORAGE_ALIGNMENT) static uint8_t storage[1024 * 1024];
    ASSERT_THAT(reinterpret_cast<uint64_t>(storage) % STORAGE_ALIGNMENT, Eq(0U));

    // storage alignment boundaries                    -> ⊥               ⊥               ⊥               ⊥
    // max user-payload alignment for test boundaries  -> ⊥       ⊥       ⊥       ⊥       ⊥       ⊥       ⊥
    // ChunkHeader alignment boundaries                -> ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥ ⊥

    // the test creates ChunkHeader on multiple address boundaries in order to have all possible scenarios up to a
    // user-payload boundary of 512; this boundary is more than large enough since a user-payload alignment of 2 times
    // the ChunkHeader alignment would already be sufficient to test all corner cases
    for (auto alignedChunkAddress = alignof(ChunkHeader); alignedChunkAddress <= MAX_USER_PAYLOAD_ALIGNMENT_FOR_TEST;
         alignedChunkAddress += alignof(ChunkHeader))
    {
        SCOPED_TRACE(std::string("Chunk placed on address ") + std::to_string(alignedChunkAddress));

        auto chunkSettingsResult = ChunkSettings::create(
            userPayloadParams.size, userPayloadParams.alignment, userHeaderSize, userHeaderAlignment);
        ASSERT_FALSE(chunkSettingsResult.has_error());
        auto& chunkSettings = chunkSettingsResult.value();
        auto chunkSize = chunkSettings.requiredChunkSize();

        auto chunkHeader = new (&storage[alignedChunkAddress]) ChunkHeader(chunkSize, chunkSettings);

        testHook(*chunkHeader);

        chunkHeader->~ChunkHeader();
    }
}

void checkUserHeaderIdAndSizeAndPayloadSizeAndAlignmentIsSet(const ChunkHeader& sut,
                                                             const PayloadParams& userPayloadParams,
                                                             const uint16_t userHeaderId,
                                                             const uint32_t userHeaderSize)
{
    SCOPED_TRACE(std::string("Check user-header id and size and user-payload alignment ist correctly set"));
    EXPECT_EQ(sut.userPayloadSize(), userPayloadParams.size);
    // a user-payload alignment of zero will internally be set to one
    auto adjustedAlignment = userPayloadParams.alignment == 0U ? 1U : userPayloadParams.alignment;
    EXPECT_EQ(sut.userPayloadAlignment(), adjustedAlignment);
    EXPECT_EQ(sut.userHeaderId(), userHeaderId);
    EXPECT_EQ(sut.userHeaderSize(), userHeaderSize);
}

void checkUserPayloadNotOverlappingWithChunkHeader(const ChunkHeader& sut)
{
    SCOPED_TRACE(std::string("Check user-payload not overlapping with ChunkHeader"));
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};

    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Ge(sizeof(ChunkHeader)));
}

void checkUserPayloadNotOverlappingWithUserHeader(const ChunkHeader& sut, const uint32_t userHeaderSize)
{
    SCOPED_TRACE(std::string("Check user-payload not overlapping with user-header"));
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};
    const uint64_t userHeaderSizeAndPadding{
        iox::algorithm::max(userHeaderSize, static_cast<uint32_t>(alignof(UserPayloadOffset_t)))};
    constexpr uint64_t BACK_OFFSET_SIZE{sizeof(UserPayloadOffset_t)};
    const uint64_t expectedRequiredSpace{sizeof(ChunkHeader) + userHeaderSizeAndPadding + BACK_OFFSET_SIZE};

    EXPECT_THAT(userPayloadStartAddress - chunkStartAddress, Ge(expectedRequiredSpace));
}

void checkUserHeaderIsAdjacentToChunkHeader(const ChunkHeader& sut)
{
    SCOPED_TRACE(std::string("Check user-header is adjacent to ChunkHeader"));
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userHeaderStartAddress{reinterpret_cast<uint64_t>(sut.userHeader())};

    EXPECT_EQ(userHeaderStartAddress - chunkStartAddress, sizeof(ChunkHeader));
}

void checkUserPayloadSize(const ChunkHeader& sut, const PayloadParams& userPayloadParams)
{
    SCOPED_TRACE(std::string("Check user-payload size"));
    EXPECT_EQ(sut.userPayloadSize(), userPayloadParams.size);
}

void checkUserPayloadAlignment(const ChunkHeader& sut, const PayloadParams& userPayloadParams)
{
    SCOPED_TRACE(std::string("Check user-payload alignment"));
    // a user-payload alignment of zero will internally be set to one
    auto adjustedAlignment = userPayloadParams.alignment == 0U ? 1U : userPayloadParams.alignment;
    EXPECT_EQ(reinterpret_cast<uint64_t>(sut.userPayload()) % adjustedAlignment, 0U);
}

void checkUsedSizeOfChunk(const ChunkHeader& sut, const PayloadParams& userPayloadParams)
{
    SCOPED_TRACE(std::string("Check used size of chunk"));
    const uint64_t chunkStartAddress{reinterpret_cast<uint64_t>(&sut)};
    const uint64_t userPayloadStartAddress{reinterpret_cast<uint64_t>(sut.userPayload())};
    const uint64_t expectedUsedSizeOfChunk{userPayloadStartAddress + userPayloadParams.size - chunkStartAddress};

    EXPECT_EQ(sut.usedSizeOfChunk(), expectedUsedSizeOfChunk);
    EXPECT_THAT(sut.usedSizeOfChunk(), Le(sut.chunkSize()));
}

void checkConversionOfUserPayloadPointerToChunkHeader(const ChunkHeader& sut)
{
    SCOPED_TRACE(std::string("Check conversion of user-payload pointer to ChunkHeader pointer"));
    const auto userPayload = sut.userPayload();
    EXPECT_EQ(ChunkHeader::fromUserPayload(userPayload), &sut);
}

class ChunkHeader_AlteringUserPayloadWithoutUserHeader : public ::testing::TestWithParam<PayloadParams>
{
};

// without a user-header, the user-payload is located right after the ChunkHeader, therefore the payload size and
// alignment parameters are made dependant on the ChunkHeader
INSTANTIATE_TEST_CASE_P(ChunkHeader_test,
                        ChunkHeader_AlteringUserPayloadWithoutUserHeader,
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
                            PayloadParams{sizeof(ChunkHeader) * 42U, alignof(ChunkHeader) * 2}));

TEST_P(ChunkHeader_AlteringUserPayloadWithoutUserHeader, CheckIntegrityOfChunkHeaderWithoutUserHeader)
{
    const auto userPayloadParams = GetParam();

    SCOPED_TRACE(std::string("User-Payload: size = ") + std::to_string(userPayloadParams.size)
                 + std::string("; alignment = ") + std::to_string(userPayloadParams.alignment));

    constexpr uint32_t USER_HEADER_SIZE{iox::CHUNK_NO_USER_HEADER_SIZE};
    constexpr uint32_t USER_HEADER_ALIGNMENT{iox::CHUNK_NO_USER_HEADER_ALIGNMENT};

    createChunksOnMultipleAddresses(userPayloadParams, USER_HEADER_SIZE, USER_HEADER_ALIGNMENT, [&](ChunkHeader& sut) {
        checkUserHeaderIdAndSizeAndPayloadSizeAndAlignmentIsSet(
            sut, userPayloadParams, ChunkHeader::NO_USER_HEADER, USER_HEADER_SIZE);
        checkUserPayloadNotOverlappingWithChunkHeader(sut);
        checkUserPayloadSize(sut, userPayloadParams);
        checkUserPayloadAlignment(sut, userPayloadParams);
        checkUsedSizeOfChunk(sut, userPayloadParams);
        checkConversionOfUserPayloadPointerToChunkHeader(sut);
    });
}

class ChunkHeader_AlteringUserPayloadWithUserHeader : public ::testing::TestWithParam<PayloadParams>
{
};

// with a user-header, the user-payload is located right after the UserPayloadOffset_t, therefore the user-payload size
// and alignment parameters are made dependant on the UserPayloadOffset_t
INSTANTIATE_TEST_CASE_P(ChunkHeader_test,
                        ChunkHeader_AlteringUserPayloadWithUserHeader,
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
                            PayloadParams{sizeof(UserPayloadOffset_t) * 42U, alignof(UserPayloadOffset_t) * 2}));

TEST_P(ChunkHeader_AlteringUserPayloadWithUserHeader, CheckIntegrityOfChunkHeaderWithUserHeader)
{
    const auto userPayloadParams = GetParam();

    SCOPED_TRACE(std::string("User-Payload: size = ") + std::to_string(userPayloadParams.size)
                 + std::string("; alignment = ") + std::to_string(userPayloadParams.alignment));

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

            createChunksOnMultipleAddresses(
                userPayloadParams, userHeaderSize, userHeaderAlignment, [&](ChunkHeader& sut) {
                    checkUserHeaderIdAndSizeAndPayloadSizeAndAlignmentIsSet(
                        sut, userPayloadParams, ChunkHeader::UNKNOWN_USER_HEADER, userHeaderSize);
                    checkUserHeaderIsAdjacentToChunkHeader(sut);
                    checkUserPayloadNotOverlappingWithUserHeader(sut, userHeaderSize);
                    checkUserPayloadSize(sut, userPayloadParams);
                    checkUserPayloadAlignment(sut, userPayloadParams);
                    checkUsedSizeOfChunk(sut, userPayloadParams);
                    checkConversionOfUserPayloadPointerToChunkHeader(sut);
                });
        }
    }
}

// END PARAMETERIZED TESTS FOR CHUNK HEADER

} // namespace
