// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_TESTUTILS_TEST_DEFINITIONS_HPP
#define IOX_UTILS_TESTUTILS_TEST_DEFINITIONS_HPP

/// @note this can be used to enable/disable tests where additional users are needed
/// e.g.: TEST_F(FOO, ADD_TEST_WITH_ADDITIONAL_USER(barTest))
#ifdef TEST_WITH_ADDITIONAL_USER
#define ADD_TEST_WITH_ADDITIONAL_USER(TestName) TestName
#else
#define ADD_TEST_WITH_ADDITIONAL_USER(TestName) DISABLED_##TestName
#endif

#endif // IOX_UTILS_TESTUTILS_TEST_DEFINITIONS_HPP
