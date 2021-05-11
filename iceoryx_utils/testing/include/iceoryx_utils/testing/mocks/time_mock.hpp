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
#ifndef IOX_UTILS_MOCKS_TIME_MOCK_HPP
#define IOX_UTILS_MOCKS_TIME_MOCK_HPP

#if !defined(_WIN32) && !defined(__APPLE__)

#include "iceoryx_utils/testing/test.hpp"
#include <time.h>

using namespace ::testing;
using ::testing::Return;

class time_MOCK
{
  public:
    time_MOCK();
    virtual ~time_MOCK()
    {
    }

    MOCK_METHOD2(clock_getres, int(clockid_t, struct timespec*));
    MOCK_METHOD2(clock_gettime, int(clockid_t, struct timespec*));
    MOCK_METHOD2(clock_settime, int(clockid_t, const struct timespec*));

    static std::unique_ptr<time_MOCK> mock;
    static bool doUseMock; // = true;
};

#if defined(QNX) || defined(QNX__) || defined(__QNX__)
int clock_getres(clockid_t clk_id, struct timespec* res);
int clock_gettime(clockid_t clk_id, struct timespec* res);
int clock_settime(clockid_t clk_id, const struct timespec* res);
#else
int clock_getres(clockid_t clk_id, struct timespec* res) noexcept;
int clock_gettime(clockid_t clk_id, struct timespec* res) noexcept;
int clock_settime(clockid_t clk_id, const struct timespec* res) noexcept;
#endif
#endif

#endif // IOX_UTILS_MOCKS_TIME_MOCK_HPP
