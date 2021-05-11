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

#include "iceoryx_utils/posix_wrapper/posix_call.hpp"
#include "test.hpp"

using namespace ::testing;

namespace
{
int testFunction(int returnValue, int errnoValue)
{
    errno = errnoValue;
    return returnValue;
}

int32_t eintrRepetition = 0;
int testEintr()
{
    if (0 < --eintrRepetition)
    {
        errno = EINTR;
        return 1;
    }
    return 0;
}

class PosixCall_test : public Test
{
  public:
};
} // namespace

TEST_F(PosixCall_test, CallingFunctionWithSuccessReturnValue_GoodCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 1;
    constexpr int ERRNO_VALUE = 2;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(RETURN_VALUE)
        .evaluate()
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithSuccessReturnValue_BadCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 3;
    constexpr int ERRNO_VALUE = 4;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(RETURN_VALUE + 1)
        .evaluate()
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        });

    // we expect an error message via stderr to the console, details are not
    // verified since it depends on the target and where the source code is
    // stored
    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithFailureReturnValue_GoodCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 5;
    constexpr int ERRNO_VALUE = 6;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .failureReturnValue(RETURN_VALUE + 1)
        .evaluate()
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithFailureReturnValue_BadCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 7;
    constexpr int ERRNO_VALUE = 8;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .failureReturnValue(RETURN_VALUE)
        .evaluate()
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        });

    // we expect an error message via stderr to the console, details are not
    // verified since it depends on the target and where the source code is
    // stored
    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithSuccessReturnValueAndIgnoredErrno_GoodCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 9;
    constexpr int ERRNO_VALUE = 10;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(RETURN_VALUE + 1)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE)
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithSuccessReturnValueAndIgnoredErrno_BadCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 11;
    constexpr int ERRNO_VALUE = 12;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(RETURN_VALUE + 1)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE + 1)
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        });

    // we expect an error message via stderr to the console, details are not
    // verified since it depends on the target and where the source code is
    // stored
    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithFailureReturnValueAndIgnoredErrno_GoodCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 13;
    constexpr int ERRNO_VALUE = 14;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .failureReturnValue(RETURN_VALUE)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE)
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, CallingFunctionWithFailureReturnValueAndIgnoredErrno_BadCase)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 15;
    constexpr int ERRNO_VALUE = 16;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .failureReturnValue(RETURN_VALUE)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE + 1)
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        });

    // we expect an error message via stderr to the console, details are not
    // verified since it depends on the target and where the source code is
    // stored
    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, IgnoringMultipleErrnosWorks)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 17;
    constexpr int ERRNO_VALUE = 18;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(1)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE - 10, ERRNO_VALUE, ERRNO_VALUE + 17)
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, IgnoringMultipleErrnosWhereOccurringErrnoIsNotListedFails)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 19;
    constexpr int ERRNO_VALUE = 20;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(1)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE - 10, ERRNO_VALUE + 17, ERRNO_VALUE + 1337, ERRNO_VALUE - 2)
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        });

    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, IgnoringMultipleErrnosWhereOccurringErrnoIsFirstInListSucceeds)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 21;
    constexpr int ERRNO_VALUE = 22;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(1)
        .evaluateWithIgnoredErrnos(ERRNO_VALUE, ERRNO_VALUE - 91, ERRNO_VALUE + 137, ERRNO_VALUE + 17, ERRNO_VALUE - 29)
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, IgnoringMultipleErrnosWhereOccurringErrnoIsLastInListSucceeds)
{
    internal::CaptureStderr();

    constexpr int RETURN_VALUE = 23;
    constexpr int ERRNO_VALUE = 24;

    iox::posix::posixCall(testFunction)(RETURN_VALUE, ERRNO_VALUE)
        .successReturnValue(1)
        .evaluateWithIgnoredErrnos(
            ERRNO_VALUE - 918, ERRNO_VALUE + 8137, ERRNO_VALUE + 187, ERRNO_VALUE - 289, ERRNO_VALUE)
        .and_then([&](auto& r) {
            EXPECT_THAT(r.value, Eq(RETURN_VALUE));
            EXPECT_THAT(r.errnum, Eq(ERRNO_VALUE));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}

TEST_F(PosixCall_test, RecallingFunctionWithEintrWorks)
{
    internal::CaptureStderr();

    eintrRepetition = iox::posix::POSIX_CALL_EINTR_REPETITIONS;
    iox::posix::posixCall(testEintr)()
        .successReturnValue(0)
        .evaluate()
        .and_then([](auto& r) {
            EXPECT_THAT(r.value, Eq(0));
            EXPECT_THAT(r.errnum, Eq(0));
        })
        .or_else([](auto&) { EXPECT_TRUE(false); });

    EXPECT_THAT(eintrRepetition, Eq(0));
    EXPECT_TRUE(internal::GetCapturedStderr().empty());
}


TEST_F(PosixCall_test, FunctionReturnsEINTRTooOftenResultsInFailure)
{
    internal::CaptureStderr();

    eintrRepetition = iox::posix::POSIX_CALL_EINTR_REPETITIONS + 1;
    iox::posix::posixCall(testEintr)()
        .successReturnValue(0)
        .evaluate()
        .and_then([](auto&) { EXPECT_TRUE(false); })
        .or_else([](auto& r) {
            EXPECT_THAT(r.value, Eq(1));
            EXPECT_THAT(r.errnum, Eq(EINTR));
        });

    EXPECT_THAT(eintrRepetition, Eq(1));
    EXPECT_FALSE(internal::GetCapturedStderr().empty());
}
