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

#include "iceoryx_utils/testing/timing_test.hpp"

namespace iox
{
namespace utils
{
namespace testing
{
bool performingTimingTest(const std::function<void()>& testCallback,
                          const uint64_t repetitions,
                          std::atomic_bool& testResult) noexcept
{
    for (uint64_t i = 0u; i < repetitions; ++i)
    {
        // new test run therefore we have to reset the testResult
        testResult.store(true);
        // testResult will be set to false if a test failes
        testCallback();

        if (testResult.load())
        {
            return true;
        }
    }
    return false;
}

std::string verifyTimingTestResult(const char* file,
                                   const int line,
                                   const char* valueStr,
                                   const bool value,
                                   const bool expected,
                                   std::atomic_bool& result) noexcept
{
    std::string errorMessage;
    if (value != expected)
    {
        errorMessage += "Timing Test failure in:\n";
        errorMessage += std::string(file) + ":" + std::to_string(line) + "\n";
        errorMessage += "Value of: " + std::string(valueStr) + " should be true\n";
        result.store(false);
    }
    return errorMessage;
}
} // namespace testing
} // namespace utils
} // namespace iox
