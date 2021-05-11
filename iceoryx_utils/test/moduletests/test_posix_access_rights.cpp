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
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"
#include "test.hpp"

#include <cstdlib>
#include <fstream>
#include <string>

namespace
{
using namespace ::testing;

const std::string TestFileName = "/tmp/PosixAccessRights_test.tmp";

class PosixAccessRights_test : public Test
{
  public:
    PosixAccessRights_test()
    {
    }

    void SetUp()
    {
        fileStream.open(TestFileName, std::fstream::out | std::fstream::trunc);
        fileStream.close();

        auto sysC = iox::cxx::makeSmartC(system,
                                         iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE,
                                         {-1},
                                         {},
                                         std::string("groups > " + TestFileName).c_str());

        if (sysC.hasErrors())
        {
            std::cerr << "system call failed with error: " << sysC.getErrorString();
            exit(EXIT_FAILURE);
        }

        internal::CaptureStderr();
    }

    void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
        if (std::remove(TestFileName.c_str()) != 0)
        {
            std::cerr << "Failed to remove temporary file '" << TestFileName
                      << "'. You'll have to remove it by yourself.";
        }
    }

    ~PosixAccessRights_test()
    {
    }

    std::fstream fileStream;
};


TEST_F(PosixAccessRights_test, DISABLED_testGroups)
{
    std::string bla;
    std::getline(fileStream, bla, ' ');
    EXPECT_TRUE(false);
}
} // namespace
