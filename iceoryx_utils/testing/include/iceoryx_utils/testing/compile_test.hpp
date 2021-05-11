// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_TESTUTILS_COMPILE_TEST_HPP
#define IOX_UTILS_TESTUTILS_COMPILE_TEST_HPP

#include "iceoryx_utils/platform/wait.hpp"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>


/// @brief Let's say you have implemented a template with certain restrictions
///         like for instance that certain types are not allowed. How would you
///         verify those restrictions. You cannot put those tests into unit test since
///         they would not be able to compile cause of the compile time error.
///         CompileTests was written to solve that problem.
///
///         Class which is testing if code is compiling or not. To realize this
///         task we use the system call to compile the code, fetch the exit code
///         of the compiler and if it is not zero we have a compilation error.
///
/// @code
///    CompileTest test(R"(
///         #include "myImportantHeader.hpp"
///         #include <type_traits>
///
///         template<typename T>
///         struct Sut {
///             static_assert(std::is_same<T, int>, "only ints");
///         };
///    )", {"myIncludeDir", "anotherIncludeDir"});
///
///    EXPECT_TRUE( test.verify(R"(Sut<int> a;)") );    // compiles since T is an int
///    EXPECT_FALSE( test.verify(R"(Sut<float> a;)") ); // does not compile since T is not an int
/// @endcode
class CompileTest
{
  public:
    /// @brief Constructs an object which verifies code snippets.
    /// @param[in] codePrefix a string which contains all the #include directives,
    ///             global variables etc. which are required to verify the compilation
    ///             of the following code snippets
    /// @param[in] a list of all required include paths
    CompileTest(const std::string& codePrefix, const std::vector<std::string>& includePath) noexcept;

    /// @brief verifies a code snippet which is placed inside a function
    /// @param[in] codeSnippet a piece of code to verify. This piece of code is placed
    ///             inside a function! therefore there are no function declarations allowed.
    /// @return if the codeSnippet compiles true, otherwise false
    bool verify(const std::string& codeSnippet) const noexcept;

  private:
    std::string m_codePrefix;
    std::string m_compilerPath = "/usr/bin/g++";
    std::string m_includePaths;
    std::string m_compilerArguments = " -std=c++14 -xc++ - -o /tmp/funkyDoodle";

    std::string m_embeddedFunctionPre = "void UnitTestFunction() {";
    std::string m_embeddedFunctionPost = "}; int main() { UnitTestFunction(); }";
};

#endif
