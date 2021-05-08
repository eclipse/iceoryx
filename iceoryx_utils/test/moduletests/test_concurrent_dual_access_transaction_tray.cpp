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

#include "iceoryx_utils/internal/concurrent/dual_access_transaction_tray.hpp"

#include "iceoryx_utils/posix_wrapper/semaphore.hpp"
#include "test.hpp"

#include <mutex>
#include <thread>

namespace
{
using namespace ::testing;
using namespace iox::concurrent;

TEST(DualAccessTransactionTray_test, StressNoContention)
{
    DualAccessTransactionTray transactionTray;
    uint64_t counter = 0U;
    constexpr uint64_t NUMBER_OF_LOOPS{1000000U};

    auto now = std::chrono::system_clock::now();
    auto threadSynchronization = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    ASSERT_FALSE(threadSynchronization.has_error());

    std::thread left([&] {
        ASSERT_FALSE(threadSynchronization->post().has_error());
        for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
        {
            DualAccessTransactionTray::AccessGuard guard(transactionTray, DualAccessTransactionTray::LEFT);
            ++counter;
        }
    });
    left.join();

    ASSERT_FALSE(threadSynchronization->wait().has_error());

    for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
    {
        DualAccessTransactionTray::AccessGuard guard(transactionTray, DualAccessTransactionTray::RIGHT);
        ++counter;
    }

    auto finish = std::chrono::system_clock::now();
    auto averageTimeInNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - now).count() / (NUMBER_OF_LOOPS * 2);
    std::cout << "Average locking time: " << averageTimeInNs / 1000. << "µs" << std::endl;

    EXPECT_EQ(counter, 2 * NUMBER_OF_LOOPS);
}

TEST(DualAccessTransactionTray_test, StressContention)
{
    DualAccessTransactionTray transactionTray;
    uint64_t counter = 0U;
    constexpr uint64_t NUMBER_OF_LOOPS{1000000U};

    auto threadSynchronization = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    ASSERT_FALSE(threadSynchronization.has_error());

    auto now = std::chrono::system_clock::now();
    std::thread left([&] {
        ASSERT_FALSE(threadSynchronization->post().has_error());
        for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
        {
            DualAccessTransactionTray::AccessGuard guard(transactionTray, DualAccessTransactionTray::LEFT);
            ++counter;
        }
    });

    ASSERT_FALSE(threadSynchronization->wait().has_error());

    for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
    {
        DualAccessTransactionTray::AccessGuard guard(transactionTray, DualAccessTransactionTray::RIGHT);
        ++counter;
    }

    left.join();
    auto finish = std::chrono::system_clock::now();
    auto averageTimeInNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - now).count() / (NUMBER_OF_LOOPS * 2);
    std::cout << "Average locking time: " << averageTimeInNs / 1000. << "µs" << std::endl;

    EXPECT_EQ(counter, 2 * NUMBER_OF_LOOPS);
}

TEST(DualAccessTransactionTray_test, StressWithMutexNoContention)
{
    std::mutex mtx;
    uint64_t counter = 0U;
    constexpr uint64_t NUMBER_OF_LOOPS{1000000U};

    auto threadSynchronization = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    ASSERT_FALSE(threadSynchronization.has_error());

    auto now = std::chrono::system_clock::now();
    std::thread left([&] {
        ASSERT_FALSE(threadSynchronization->post().has_error());
        for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
        {
            std::lock_guard<std::mutex> g(mtx);
            ++counter;
        }
    });
    left.join();

    ASSERT_FALSE(threadSynchronization->wait().has_error());

    for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
    {
        std::lock_guard<std::mutex> g(mtx);
        ++counter;
    }

    auto finish = std::chrono::system_clock::now();
    auto averageTimeInNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - now).count() / (NUMBER_OF_LOOPS * 2);
    std::cout << "Average locking time: " << averageTimeInNs / 1000. << "µs" << std::endl;

    EXPECT_EQ(counter, 2 * NUMBER_OF_LOOPS);
}

TEST(DualAccessTransactionTray_test, StressWithMutexContention)
{
    std::mutex mtx;
    uint64_t counter = 0U;
    constexpr uint64_t NUMBER_OF_LOOPS{1000000U};

    auto threadSynchronization = iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U);
    ASSERT_FALSE(threadSynchronization.has_error());

    auto now = std::chrono::system_clock::now();
    std::thread left([&] {
        ASSERT_FALSE(threadSynchronization->post().has_error());
        for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
        {
            std::lock_guard<std::mutex> g(mtx);
            ++counter;
        }
    });

    ASSERT_FALSE(threadSynchronization->wait().has_error());

    for (uint64_t i = 0; i < NUMBER_OF_LOOPS; ++i)
    {
        std::lock_guard<std::mutex> g(mtx);
        ++counter;
    }

    left.join();
    auto finish = std::chrono::system_clock::now();
    auto averageTimeInNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - now).count() / (NUMBER_OF_LOOPS * 2);
    std::cout << "Average locking time: " << averageTimeInNs / 1000. << "µs" << std::endl;

    EXPECT_EQ(counter, 2 * NUMBER_OF_LOOPS);
}
} // namespace
