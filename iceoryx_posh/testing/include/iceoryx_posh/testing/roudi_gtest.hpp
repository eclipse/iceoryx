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
#ifndef IOX_UTILS_TESTUTILS_ROUDI_GTEST_HPP
#define IOX_UTILS_TESTUTILS_ROUDI_GTEST_HPP

#include "iceoryx_posh/testing/roudi_environment/roudi_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using ::testing::Return;

class RouDi_GTest : public iox::roudi::RouDiEnvironment, public Test
{
  public:
    RouDi_GTest() = default;
    RouDi_GTest(iox::RouDiConfig_t& roudiConfig)
        : iox::roudi::RouDiEnvironment(roudiConfig)
    {
    }
};

#endif // IOX_UTILS_TESTUTILS_ROUDI_GTEST_HPP
