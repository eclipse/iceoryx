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

#include "example_base_class.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using ::testing::Return;

namespace example
{
namespace test
{
/// @req IOX_SWRS_112, IOX_SWRS_200
/// @brief Test goal: "This test suite verifies that the BaseClass function is verified"
/// @pre describe what needs to be done in setup()
/// @post describe what needs to be done in teardown()
/// @note name of the Testfixture should match to the Class you want to test
class ExampleBaseClass_test : public Test
{
  public:
    example::ExampleBaseClass<uint32_t> sut;
};

/// @note name of the Testcase shall describe the test case in detail to avoid additional comments
TEST_F(ExampleBaseClass_test, FirstUnitTestCase)
{
    EXPECT_THAT(sut.simplerMethod(), Eq(99U));
}

/// @note name of the Testcase shall describe the test case in detail to avoid additional comments
TEST_F(ExampleBaseClass_test, GetMemberVariableFromCtor)
{
    example::ExampleBaseClass<uint32_t> sut2(100);
    EXPECT_THAT(sut2.getMemberVariable(), Eq(99U));
}

} // namespace test
} // namespace example