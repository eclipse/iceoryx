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

#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer_data.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::rp;

TEST(RealtivePointerData_test, DefaultConstructedResultsInNullptrIdAndOffset)
{
    RelativePointerData sut;

    EXPECT_EQ(sut.id(), RelativePointerData::NULL_POINTER_ID);
    EXPECT_EQ(sut.offset(), RelativePointerData::NULL_POINTER_OFFSET);
}

TEST(RealtivePointerData_test, DefaultConstructedResultsInLogicallyNullptr)
{
    RelativePointerData sut;

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ResetOnDefaultConstructedResultsInLogicallyNullptr)
{
    RelativePointerData sut;

    sut.reset();

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ConstructedWithNonZeroIdAndOffsetResultsInSameIdAndOffset)
{
    constexpr uint16_t ID{13U};
    constexpr uint64_t OFFSET{42U};

    RelativePointerData sut{ID, OFFSET};

    EXPECT_EQ(sut.id(), ID);
    EXPECT_EQ(sut.offset(), OFFSET);
}

TEST(RealtivePointerData_test, ConstructedWithZeroIdAndOffsetResultsNotInLogicallyNullptr)
{
    constexpr uint16_t ID{0U};
    constexpr uint64_t OFFSET{0U};

    RelativePointerData sut{ID, OFFSET};

    EXPECT_FALSE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ConstructedWithNonZeroIdAndOffsetResultsNotInLogicallyNullptr)
{
    constexpr uint16_t ID{13U};
    constexpr uint64_t OFFSET{42U};

    RelativePointerData sut{ID, OFFSET};

    EXPECT_FALSE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ConstructedWithMaxIdAndOffsetResultsNotInLogicallyNullptr)
{
    constexpr uint16_t ID{RelativePointerData::MAX_VALID_ID};
    constexpr uint64_t OFFSET{RelativePointerData::MAX_VALID_OFFSET};

    RelativePointerData sut{ID, OFFSET};

    EXPECT_FALSE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ResetOnConstructedWithNonZeroIdAndOffsetResultsInLogicallyNullptr)
{
    constexpr uint16_t ID{13U};
    constexpr uint64_t OFFSET{42U};

    RelativePointerData sut{ID, OFFSET};
    sut.reset();

    EXPECT_TRUE(sut.isLogicalNullptr());
}

TEST(RealtivePointerData_test, ConstructedWithNullPointerIdResultsInTermination)
{
    constexpr uint16_t ID{RelativePointerData::NULL_POINTER_ID};
    constexpr uint64_t OFFSET{RelativePointerData::MAX_VALID_OFFSET};

    EXPECT_DEATH({ RelativePointerData sut(ID, OFFSET); }, ".*");
}

TEST(RealtivePointerData_test, ConstructedWithNullPointerOffsetResultsInTermination)
{
    constexpr uint16_t ID{RelativePointerData::MAX_VALID_ID};
    constexpr uint64_t OFFSET{RelativePointerData::NULL_POINTER_OFFSET};

    EXPECT_DEATH({ RelativePointerData sut(ID, OFFSET); }, ".*");
}

} // namespace