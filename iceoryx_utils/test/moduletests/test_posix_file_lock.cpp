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

#if !defined(_WIN32) && !defined(__APPLE__)
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/posix_wrapper/file_lock.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::posix;
using namespace iox::cxx;

constexpr char TEST_NAME[] = "TestProcess";
constexpr char ANOTHER_TEST_NAME[] = "AnotherTestProcess";

/// @req
/// @brief This test suite verifies the RAII behaviour of FileLock
/// @pre The file lock for TEST_NAME is acquired
/// @post The file lock for TEST_NAME is released
/// @note This should become a FЯIDA integration test once available, in order to test with two processes
class FileLock_test : public Test
{
  public:
    FileLock_test()
    {
    }

    void SetUp()
    {
        auto maybeFileLock = iox::posix::FileLock::create(TEST_NAME);
        ASSERT_FALSE(maybeFileLock.has_error());
        m_sut.emplace(std::move(maybeFileLock.value()));
        ASSERT_TRUE(m_sut.has_value());
    }

    void TearDown()
    {
        m_sut.reset();
    }

    ~FileLock_test()
    {
    }
    optional<FileLock> m_sut;
};

TEST_F(FileLock_test, EmptyNameLeadsToError)
{
    auto sut2 = iox::posix::FileLock::create("");
    ASSERT_TRUE(sut2.has_error());
    EXPECT_THAT(sut2.get_error(), Eq(FileLockError::NO_FILE_NAME_PROVIDED));
}

TEST_F(FileLock_test, MaxStringWorks)
{
    const FileLock::FileName_t maxString{
        "OeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPalo"
        "emaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlancaOeLaPaloemaBlanc"
        "aOeLaPaloemaBlancaOeLaPaloemaB"};
    auto sut2 = iox::posix::FileLock::create(maxString);
    ASSERT_FALSE(sut2.has_error());
}

TEST_F(FileLock_test, SecondLockWithDifferentNameWorks)
{
    auto sut2 = iox::posix::FileLock::create(ANOTHER_TEST_NAME);
    ASSERT_FALSE(sut2.has_error());
}

TEST_F(FileLock_test, LockAndReleaseWorks)
{
    {
        IOX_DISCARD_RESULT(iox::posix::FileLock::create(ANOTHER_TEST_NAME));
    }
    auto sut2 = iox::posix::FileLock::create(ANOTHER_TEST_NAME);
    ASSERT_FALSE(sut2.has_error());
}

TEST_F(FileLock_test, CreatingSameFileLockAgainFails)
{
    auto sut2 = iox::posix::FileLock::create(TEST_NAME);
    ASSERT_TRUE(sut2.has_error());
    EXPECT_THAT(sut2.get_error(), Eq(FileLockError::LOCKED_BY_OTHER_PROCESS));
}

TEST_F(FileLock_test, MoveCtorInvalidatesRhs)
{
    auto movedSut{std::move(m_sut.value())};
    ASSERT_FALSE(m_sut.value().isInitialized());
    ASSERT_TRUE(movedSut.isInitialized());
}

TEST_F(FileLock_test, MoveCtorTransfersLock)
{
    auto movedSut{std::move(m_sut.value())};
    auto anotherLock = iox::posix::FileLock::create(TEST_NAME);
    ASSERT_TRUE(anotherLock.has_error());
    EXPECT_THAT(anotherLock.get_error(), Eq(FileLockError::LOCKED_BY_OTHER_PROCESS));
}

TEST_F(FileLock_test, MoveAssignInvalidatesRhs)
{
    auto movedSut = std::move(m_sut.value());
    ASSERT_FALSE(m_sut.value().isInitialized());
    ASSERT_TRUE(movedSut.isInitialized());
}

TEST_F(FileLock_test, MoveAssignTransfersLock)
{
    auto movedSut = std::move(m_sut.value());
    auto anotherLock = iox::posix::FileLock::create(TEST_NAME);
    ASSERT_TRUE(anotherLock.has_error());
    EXPECT_THAT(anotherLock.get_error(), Eq(FileLockError::LOCKED_BY_OTHER_PROCESS));
}
} // namespace
#endif
