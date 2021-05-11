// Copyright (c) 2021 Apex.AI Inc. All rights reserved.
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

#include "iceoryx_utils/log/logmanager.hpp"

extern "C" {
#include "iceoryx_binding_c/log.h"
}

#include "test.hpp"

using namespace ::testing;
using namespace iox::log;

TEST(iox_log_test, LogLevelIsSetCorrectly)
{
    auto& logManager = iox::log::LogManager::GetLogManager();

    iox_set_loglevel(Iceoryx_LogLevel_Off);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kOff);

    iox_set_loglevel(Iceoryx_LogLevel_Fatal);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kFatal);

    iox_set_loglevel(Iceoryx_LogLevel_Debug);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kDebug);

    iox_set_loglevel(Iceoryx_LogLevel_Warn);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kWarn);

    iox_set_loglevel(Iceoryx_LogLevel_Info);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kInfo);

    iox_set_loglevel(Iceoryx_LogLevel_Debug);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kDebug);

    iox_set_loglevel(Iceoryx_LogLevel_Verbose);
    EXPECT_EQ(logManager.DefaultLogLevel(), LogLevel::kVerbose);
}
