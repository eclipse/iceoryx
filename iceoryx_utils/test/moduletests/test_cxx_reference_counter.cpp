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

#include "iceoryx_utils/internal/cxx/reference_counter.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::cxx;

class ReferenceCounter_test : public Test
{
  public:
    uint64_t var{0};
    ReferenceCounter<uint64_t> sut{&var};
};

TEST_F(ReferenceCounter_test, CTor)
{
    EXPECT_THAT(var, Eq(1));
}

TEST_F(ReferenceCounter_test, CopyCTor)
{
    ReferenceCounter<uint64_t> sut2(sut);
    EXPECT_THAT(var, Eq(2));
}

TEST_F(ReferenceCounter_test, MoveCTor)
{
    ReferenceCounter<uint64_t> sut2(std::move(sut));
    EXPECT_THAT(var, Eq(1));
}

TEST_F(ReferenceCounter_test, CopyAssignment)
{
    uint64_t var2{0};
    ReferenceCounter<uint64_t> sut2(&var);
    sut2 = sut;

    EXPECT_THAT(var, Eq(2));
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, MoveAssignment)
{
    uint64_t var2{0};
    ReferenceCounter<uint64_t> sut2(&var);
    sut2 = std::move(sut);

    EXPECT_THAT(var, Eq(1));
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, Destructor)
{
    uint64_t var2{0};
    {
        ReferenceCounter<uint64_t> sut2(&var2);
    }
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, DestructorAfterCopyCTor)
{
    uint64_t var2{0};
    {
        ReferenceCounter<uint64_t> sut2(&var2);
        ReferenceCounter<uint64_t> sut3(sut2);
    }
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, DestructorAfterMoveCTor)
{
    uint64_t var2{0};
    {
        ReferenceCounter<uint64_t> sut2(&var2);
        ReferenceCounter<uint64_t> sut3(std::move(sut2));
    }
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, DestructorAfterCopyAssignment)
{
    uint64_t var2{0};
    {
        uint64_t var3{0};
        ReferenceCounter<uint64_t> sut2(&var2);
        {
            ReferenceCounter<uint64_t> sut3(&var3);
            sut3 = sut2;
            EXPECT_THAT(var2, Eq(2));
        }
        EXPECT_THAT(var2, Eq(1));
    }
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, DestructorAfterMoveAssignment)
{
    uint64_t var2{0};
    {
        uint64_t var3{0};
        ReferenceCounter<uint64_t> sut2(&var2);
        {
            ReferenceCounter<uint64_t> sut3(&var3);
            sut3 = std::move(sut2);
            EXPECT_THAT(var2, Eq(1));
        }
        EXPECT_THAT(var2, Eq(0));
    }
    EXPECT_THAT(var2, Eq(0));
}

TEST_F(ReferenceCounter_test, GetValue)
{
    EXPECT_THAT(sut.getValue(), Eq(1));
}
} // namespace
