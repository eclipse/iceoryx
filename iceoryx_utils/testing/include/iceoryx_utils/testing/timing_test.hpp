// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_TESTUTILS_TIMING_TEST_HPP
#define IOX_UTILS_TESTUTILS_TIMING_TEST_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

/// @brief This headers provides TIMING_TEST unit test infrastructure. The idea
///         is that a timing test is running multiple times and if in one of the
///         repetitions all results of the test are successful then the timing
///         test itself is successful.
///
///        You can get to know them in praxis in the test_posix_timer.cpp unit test
///        for instance.
///
/// @code
/// class MyClass_test : public Test {};
///
/// TIMING_TEST_F(MyClass_test, WaitForSleep, Repeat(3), [&]{
///   std::atomic_bool threadFinished{false};
///   std::thread t([&]{ sleep(2); threadFinished = true; });
///
///   TIMING_TEST_EXPECT_FALSE(threadFinished.load());
///   sleep(1);
///   TIMING_TEST_EXPECT_FALSE(threadFinished.load());
///   sleep(2);
///   TIMING_TEST_EXPECT_TRUE(threadFinished.load());
///   t.join();
/// });
/// @endcode
///
/// @details
///     Available testing verificators
///        TIMING_TEST_EXPECT_TRUE(value);
///        TIMING_TEST_EXPECT_FALSE(value);
///        TIMING_TEST_ASSERT_TRUE(value);
///        TIMING_TEST_ASSERT_FALSE(value);
///
///     Available test types
///         TIMING_TEST_F -> maps to TEST_F
///         TIMING_TEST_P -> maps to TEST_P
///
///     If you would like to disable timing tests you can start your unit
///     test like:
///      # ./myUnitTest --gtest_filter="-*TimingTest*"
///
///     Or if you would like only timing test you can use the following
///     approach:
///      # ./myUnitTest --gtest_filter="*TimingTest*"

#define TIMING_TEST_CONSTRUCT(Name, Case, Repetitions, Test, GTestType)                                                \
    GTestType(Name, TimingTest_##Case)                                                                                 \
    {                                                                                                                  \
        std::atomic_bool timingTestResult{true};                                                                       \
        std::string errorMessages;                                                                                     \
        bool testResult = iox::utils::testing::performingTimingTest(Test, Repetitions, timingTestResult);              \
        EXPECT_TRUE(testResult);                                                                                       \
        if (!testResult)                                                                                               \
        {                                                                                                              \
            std::cout << "\n" << errorMessages << std::endl;                                                           \
        }                                                                                                              \
    }

#define TIMING_TEST_F(Name, Case, Repetitions, Test) TIMING_TEST_CONSTRUCT(Name, Case, Repetitions, Test, TEST_F)
#define TIMING_TEST_P(Name, Case, Repetitions, Test) TIMING_TEST_CONSTRUCT(Name, Case, Repetitions, Test, TEST_P)

#define TIMING_TEST_EXPECT_ALWAYS_TRUE(value) EXPECT_TRUE(value)
#define TIMING_TEST_EXPECT_ALWAYS_FALSE(value) EXPECT_FALSE(value)
#define TIMING_TEST_EXPECT_TRUE(value)                                                                                 \
    errorMessages +=                                                                                                   \
        iox::utils::testing::verifyTimingTestResult(__FILE__, __LINE__, #value, value, true, timingTestResult)
#define TIMING_TEST_EXPECT_FALSE(value)                                                                                \
    errorMessages +=                                                                                                   \
        iox::utils::testing::verifyTimingTestResult(__FILE__, __LINE__, #value, value, false, timingTestResult)
#define TIMING_TEST_ASSERT_TRUE(value)                                                                                 \
    TIMING_TEST_EXPECT_TRUE(value);                                                                                    \
    if (!timingTestResult.load())                                                                                      \
    {                                                                                                                  \
        return;                                                                                                        \
    }
#define TIMING_TEST_ASSERT_FALSE(value)                                                                                \
    TIMING_TEST_EXPECT_FALSE(value);                                                                                   \
    if (!timingTestResult.load())                                                                                      \
    {                                                                                                                  \
        return;                                                                                                        \
    }

#define Repeat(n) n

namespace iox
{
namespace utils
{
namespace testing
{
bool performingTimingTest(const std::function<void()>& testCallback,
                          const uint64_t repetitions,
                          std::atomic_bool& testResult) noexcept;


std::string verifyTimingTestResult(const char* file,
                                   const int line,
                                   const char* valueStr,
                                   const bool value,
                                   const bool expected,
                                   std::atomic_bool& result) noexcept;

} // namespace testing
} // namespace utils
} // namespace iox

#endif // IOX_UTILS_TESTUTILS_TIMING_TEST_HPP
