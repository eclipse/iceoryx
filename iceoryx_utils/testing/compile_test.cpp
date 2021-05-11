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

#include "iceoryx_utils/testing/compile_test.hpp"

CompileTest::CompileTest(const std::string& codePrefix, const std::vector<std::string>& includePath) noexcept
    : m_codePrefix(codePrefix)
{
    for (auto& p : includePath)
    {
        m_includePaths += "-I" + p + " ";
    }
}

bool CompileTest::verify(const std::string& codeSnippet) const noexcept
{
    std::string code = m_codePrefix + m_embeddedFunctionPre + codeSnippet + m_embeddedFunctionPost;

    std::string compilerCommand = m_compilerPath + " " + m_includePaths + " " + m_compilerArguments;
    std::string command = "echo \"" + code + "\" | " + compilerCommand;
    int sysCall = system(command.c_str());
    int exitCode = WEXITSTATUS(sysCall);

    return exitCode == 0;
}
