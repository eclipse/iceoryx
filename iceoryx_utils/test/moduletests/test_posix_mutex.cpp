// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by ApexAI Inc. All rights reserved.
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

#include "iceoryx_utils/cxx/deadline_timer.hpp"
#include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_utils/testing/test.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"

#include <atomic>
#include <thread>

namespace
{
using namespace ::testing;
using namespace iox::units::duration_literals;

class Mutex_test : public Test
{
  public:
    void SetUp() override
    {
        internal::CaptureStderr();
        m_deadlockWatchdog.watchAndActOnFailure([] { std::terminate(); });
    }

    void TearDown() override
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    iox::posix::mutex sutNonRecursive{false};
    iox::posix::mutex sutRecursive{true};
    iox::units::Duration m_watchdogTimeout = 5_s;
    Watchdog m_deadlockWatchdog{m_watchdogTimeout};
};

TEST_F(Mutex_test, TryLockAndUnlockWithNonRecursiveMutexReturnTrue)
{
    EXPECT_THAT(sutNonRecursive.try_lock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.unlock(), Eq(true));
}


#ifndef _WIN32
TEST_F(Mutex_test, TryLockWithNonRecursiveMutexReturnsFalseAfterLock)
{
    EXPECT_THAT(sutNonRecursive.lock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.try_lock(), Eq(false));
    EXPECT_THAT(sutNonRecursive.unlock(), Eq(true));
}
#endif

TEST_F(Mutex_test, LockAndUnlockWithNonRecursiveMutexReturnsTrue)
{
    EXPECT_THAT(sutNonRecursive.lock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.unlock(), Eq(true));
}

TEST_F(Mutex_test, RepeatedLockAndUnlockWithNonRecursiveMutexReturnsTrue)
{
    EXPECT_THAT(sutNonRecursive.lock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.unlock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.lock(), Eq(true));
    EXPECT_THAT(sutNonRecursive.unlock(), Eq(true));
}

#if !defined(_WIN32) && !defined(QNX) && !defined(QNX__) && !defined(__QNX__) && !defined(__APPLE__)
// in qnx you can destroy a locked mutex, without error if the thread holding the lock is destructing it.
TEST_F(Mutex_test, CallingDestructorOnLockedMutexLeadsToTermination)
{
    std::string output = internal::GetCapturedStderr();
    std::set_terminate([]() { std::cout << "", std::abort(); });

    EXPECT_DEATH(
        {
            iox::posix::mutex mtx{false};
            mtx.lock();
        },
        ".*");

    internal::CaptureStderr();
}
#endif

TEST_F(Mutex_test, LockedMutexBlocksCallingThreadExecution)
{
    std::atomic_bool isLockFinished{false};
    sutNonRecursive.lock();

    std::thread lockThread([&] {
        sutNonRecursive.lock();
        isLockFinished.store(true);
        sutNonRecursive.unlock();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_THAT(isLockFinished.load(), Eq(false));

    sutNonRecursive.unlock();
    lockThread.join();

    EXPECT_THAT(isLockFinished.load(), Eq(true));
}

TEST_F(Mutex_test, TryLockWithRecursiveMutexReturnsFalseWhenMutexLockedInOtherThread)
{
    std::atomic_bool isTryLockSuccessful{true};
    sutRecursive.lock();

    std::thread lockThread([&] { isTryLockSuccessful = sutRecursive.try_lock(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_THAT(isTryLockSuccessful.load(), Eq(false));

    sutRecursive.unlock();
    lockThread.join();
}
} // namespace
