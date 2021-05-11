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

#include "iceoryx_utils/cxx/generic_raii.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::cxx;

class GenericRAII_test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(GenericRAII_test, InitFunctionIsCalledInCtorWhenSet)
{
    int hasCalledInit = 0;
    GenericRAII sut([&] { ++hasCalledInit; }, std::function<void()>());
    EXPECT_THAT(hasCalledInit, Eq(1));
}

TEST_F(GenericRAII_test, InitFunctionIsCalledInCtorWhenSetWithCleanupFunction)
{
    int hasCalledInit = 0;
    int hasCalledCleanup = 0;
    GenericRAII sut([&] { ++hasCalledInit; }, [&] { ++hasCalledCleanup; });
    EXPECT_THAT(hasCalledInit, Eq(1));
}

TEST_F(GenericRAII_test, CleanupFunctionIsCalledInDtor)
{
    int hasCalledInit = 0;
    int hasCalledCleanup = 0;

    {
        GenericRAII sut([&] { ++hasCalledInit; }, [&] { ++hasCalledCleanup; });
    }

    EXPECT_THAT(hasCalledInit, Eq(1));
    EXPECT_THAT(hasCalledCleanup, Eq(1));
}

TEST_F(GenericRAII_test, CleanupFunctionIsCalledInDtorWhenUsingCleanupOnlyCTor)
{
    int hasCalledCleanup = 0;

    {
        GenericRAII sut([&] { hasCalledCleanup = true; });
    }

    EXPECT_THAT(hasCalledCleanup, Eq(1));
}

TEST_F(GenericRAII_test, CleanupFunctionIsCalledInDtorWithEmptyInitFunction)
{
    int hasCalledCleanup = 0;

    {
        GenericRAII sut(function_ref<void()>(), [&] { ++hasCalledCleanup; });
    }

    EXPECT_THAT(hasCalledCleanup, Eq(1));
}

TEST_F(GenericRAII_test, MoveCTorDoesNotCallCleanupFunctionOfOrigin)
{
    int hasCalledCleanup = 0;

    GenericRAII sut([&] { ++hasCalledCleanup; });
    GenericRAII sut2(std::move(sut));

    EXPECT_THAT(hasCalledCleanup, Eq(0));
}

TEST_F(GenericRAII_test, MoveConstructedDoesCallCleanupFunctionWhenDestroyed)
{
    int hasCalledCleanup = 0;

    {
        iox::cxx::optional<GenericRAII> sut(GenericRAII([&] { ++hasCalledCleanup; }));
        GenericRAII sut2(std::move(*sut));
        sut.reset();
        EXPECT_THAT(hasCalledCleanup, Eq(0));
    }
    EXPECT_THAT(hasCalledCleanup, Eq(1));
}

TEST_F(GenericRAII_test, MoveAssignmentCallsCleanup)
{
    int hasCalledCleanup = 0;
    int hasCalledCleanup2 = 0;

    GenericRAII sut([&] { ++hasCalledCleanup; });
    GenericRAII sut2([&] { ++hasCalledCleanup2; });

    sut = std::move(sut2);

    EXPECT_THAT(hasCalledCleanup, Eq(1));
    EXPECT_THAT(hasCalledCleanup2, Eq(0));
}

TEST_F(GenericRAII_test, MoveAssignedCallsCleanupWhenOutOfScope)
{
    int hasCalledCleanup = 0;
    int hasCalledCleanup2 = 0;

    {
        GenericRAII sut([&] { ++hasCalledCleanup; });
        GenericRAII sut2([&] { ++hasCalledCleanup2; });

        sut = std::move(sut2);
        EXPECT_THAT(hasCalledCleanup, Eq(1));
        EXPECT_THAT(hasCalledCleanup2, Eq(0));
    }

    EXPECT_THAT(hasCalledCleanup2, Eq(1));
}
} // namespace
