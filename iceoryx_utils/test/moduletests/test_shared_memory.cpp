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

#include "test.hpp"

#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/shared_memory.hpp"
#include "iceoryx_utils/platform/stat.hpp"

namespace
{
using namespace testing;

class SharedMemory_Test : public Test
{
  public:
    void SetUp() override
    {
        internal::CaptureStderr();
    }

    void TearDown() override
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};

TEST_F(SharedMemory_Test, CTorWithValidArguments)
{
    auto sut = iox::posix::SharedMemory::create("/ignatz",
                                                iox::posix::AccessMode::READ_WRITE,
                                                iox::posix::OwnerShip::MINE,
                                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                128);
    EXPECT_THAT(sut.has_error(), Eq(false));
}

TEST_F(SharedMemory_Test, CTorWithInvalidMessageQueueNames)
{
    EXPECT_THAT(iox::posix::SharedMemory::create("",
                                                 iox::posix::AccessMode::READ_WRITE,
                                                 iox::posix::OwnerShip::MINE,
                                                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                 128)
                    .has_error(),
                Eq(true));

    EXPECT_THAT(iox::posix::SharedMemory::create("ignatz",
                                                 iox::posix::AccessMode::READ_WRITE,
                                                 iox::posix::OwnerShip::MINE,
                                                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                 128)
                    .has_error(),
                Eq(true));
}

TEST_F(SharedMemory_Test, CTorWithInvalidArguments)
{
    auto sut = iox::posix::SharedMemory::create("/schlomo",
                                                iox::posix::AccessMode::READ_WRITE,
                                                iox::posix::OwnerShip::OPEN_EXISTING,
                                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                128);
    EXPECT_THAT(sut.has_error(), Eq(true));
}

TEST_F(SharedMemory_Test, MoveCTorWithValidValues)
{
    int handle;

    auto sut = iox::posix::SharedMemory::create("/ignatz",
                                                iox::posix::AccessMode::READ_WRITE,
                                                iox::posix::OwnerShip::MINE,
                                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                128);
    handle = sut->getHandle();
    {
        iox::posix::SharedMemory sut2(std::move(*sut));
        EXPECT_THAT(handle, Eq(sut2.getHandle()));
        EXPECT_THAT(sut->isInitialized(), Eq(false));
    }
}

TEST_F(SharedMemory_Test, getHandleOfValidObject)
{
    auto sut = iox::posix::SharedMemory::create("/ignatz",
                                                iox::posix::AccessMode::READ_WRITE,
                                                iox::posix::OwnerShip::MINE,
                                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                                                128);
    EXPECT_THAT(sut->getHandle(), Ne(-1));
}
} // namespace
