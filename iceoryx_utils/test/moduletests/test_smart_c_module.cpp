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

#include "iceoryx_utils/cxx/smart_c.hpp"
#include "test.hpp"

#include <errno.h>
#include <string>

namespace
{
using namespace ::testing;

static int SomeFunction(int a, int b, int c)
{
    errno = a * b * c + 10;
    return a * b * c;
}

int remainingErrnoCounter{3};
static int SetErrno(int errnoValue)
{
    if (remainingErrnoCounter > 0)
    {
        errno = errnoValue;
        remainingErrnoCounter--;
    }
    else
    {
        errno = 0;
    }
    return remainingErrnoCounter;
}

class smart_c_test : public Test
{
  public:
    void SetUp()
    {
        internal::CaptureStderr();
    };

    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    };
};

TEST_F(smart_c_test, SimpleFunctionWithErrorCode)
{
    auto call = iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {0}, {}, 1, 2, 3);
    EXPECT_THAT(call.hasErrors(), Eq(false));
    EXPECT_THAT(call.getReturnValue(), Eq(6));
    EXPECT_THAT(call.getErrNum(), Eq(0));
}

TEST_F(smart_c_test, SimpleFunctionWithErrorCodeOneError)
{
    auto call = iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {0}, {}, 1, 0, 3);

    EXPECT_THAT(call.hasErrors(), Eq(true));
    EXPECT_THAT(call.getReturnValue(), Eq(0));
    EXPECT_THAT(call.getErrNum(), Eq(10));
    std::string errortext = call.getErrorString();
    EXPECT_THAT(errortext.size(), Ne(0u));
}

TEST_F(smart_c_test, SimpleFunctionWithErrorCodeMultipleErrors)
{
    auto call = iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {1, -1}, {}, 1, 1, 1);

    ASSERT_THAT(call.hasErrors(), Eq(true));
    EXPECT_THAT(call.getReturnValue(), Eq(1));
}

TEST_F(smart_c_test, SimpleFunctionWithErrorCodeErrorIgnored)
{
    auto call =
        iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {1, -1}, {11}, 1, 1, 1);

    ASSERT_THAT(call.hasErrors(), Eq(false));
    EXPECT_THAT(call.getReturnValue(), Eq(1));
}

TEST_F(smart_c_test, SimpleFunctionWithSuccessCode)
{
    auto call = iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {6}, {}, 1, 2, 3);
    EXPECT_THAT(call.hasErrors(), Eq(false));
    EXPECT_THAT(call.getReturnValue(), Eq(6));
    EXPECT_THAT(call.getErrNum(), Eq(0));
}

TEST_F(smart_c_test, SimpleFunctionWithSuccessCodeOnError)
{
    auto call = iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {6}, {}, 4, 2, 3);

    EXPECT_THAT(call.hasErrors(), Eq(true));
    EXPECT_THAT(call.getReturnValue(), Eq(24));
    EXPECT_THAT(call.getErrNum(), Eq(34));
}

TEST_F(smart_c_test, SimpleFunctionWithSuccessMultipleSuccessCodes)
{
    auto call =
        iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {6, 24}, {}, 4, 2, 3);
    EXPECT_THAT(call.hasErrors(), Eq(false));
    EXPECT_THAT(call.getReturnValue(), Eq(24));
    EXPECT_THAT(call.getErrNum(), Eq(0));
}

TEST_F(smart_c_test, SimpleFunctionWithSuccessCodeAndIgnoredErrorCode)
{
    auto call =
        iox::cxx::makeSmartC(SomeFunction, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {6, 24}, {10}, 0, 2, 3);

    EXPECT_THAT(call.hasErrors(), Eq(false));
    EXPECT_THAT(call.getReturnValue(), Eq(0));
}

TEST_F(smart_c_test, SimpleFunctionWithFailedEINTRRepitition)
{
    remainingErrnoCounter = 10;
    auto call = iox::cxx::makeSmartC(SetErrno, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, EINTR);

    EXPECT_THAT(call.hasErrors(), Eq(true));
}

TEST_F(smart_c_test, SimpleFunctionWithSuccessfulEINTRRepitition)
{
    remainingErrnoCounter = 3;
    auto call = iox::cxx::makeSmartC(SetErrno, iox::cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, EINTR);

    EXPECT_THAT(call.hasErrors(), Eq(false));
}
} // namespace
