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

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/log/logger.hpp"
#include "iceoryx_utils/log/logging.hpp"
#include "test.hpp"

#include <ctime>
#include <iostream>
#include <regex>
#include <sstream>

namespace
{
using namespace ::testing;

class LoggerSUT : public iox::log::Logger
{
  public:
    LoggerSUT()
        : iox::log::Logger("Test", "Context for testing!", iox::log::LogLevel::kVerbose)
    {
    }

    using Logger::Log;
};

class OutputBuffer
{
  public:
    OutputBuffer()
    {
        m_oldBuffer = std::clog.rdbuf();
        std::clog.rdbuf(m_capture.rdbuf());
    }
    ~OutputBuffer()
    {
        std::clog.rdbuf(m_oldBuffer);
    }

    std::string str()
    {
        return m_capture.str();
    }

    void clear()
    {
        m_capture.str(std::string());
    }

  private:
    std::stringstream m_capture;
    std::streambuf* m_oldBuffer{nullptr};
};

class IoxLogger_testBase
{
  public:
    std::string formatDateTime(std::chrono::milliseconds timeStamp)
    {
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(timeStamp);
        std::time_t time = sec.count();

        auto timeInfo = std::localtime(&time);

        char dateTimeBuffer[40];
        strftime(dateTimeBuffer, 40, "%Y-%m-%d %H:%M:%S", timeInfo);
        std::string dateTimeString = std::string(dateTimeBuffer) + ".000";

        return dateTimeString;
    }

    const std::regex colorCode{"\\x1B\\[([0-9]*;?)*m"};

    OutputBuffer outBuffer;

    LoggerSUT m_sut;
};

class IoxLogger_test : public Test, public IoxLogger_testBase
{
  public:
    void SetUp() override
    {
        outBuffer.clear();
    }

    void TearDown() override
    {
    }
};

TEST_F(IoxLogger_test, Output)
{
    iox::log::LogEntry entry;
    entry.level = iox::log::LogLevel::kError;
    entry.message = "42";

    m_sut.SetLogLevel(iox::log::LogLevel::kInfo);
    m_sut.Log(entry);

    const std::string expected = formatDateTime(entry.time) + " [ Error ]: 42\n";

    // at the moment we don't care how the colors are, therefore remove them
    std::string output = std::regex_replace(outBuffer.str(), colorCode, std::string(""));

    EXPECT_THAT(output, Eq(expected));
}


class IoxLoggerLogLevel_test : public TestWithParam<iox::log::LogLevel>, public IoxLogger_testBase
{
  public:
    void SetUp() override
    {
        outBuffer.clear();
    }

    void TearDown() override
    {
    }
};

/// we require INSTANTIATE_TEST_CASE_P since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
INSTANTIATE_TEST_CASE_P(AllLogLevel,
                        IoxLoggerLogLevel_test,
                        Values(iox::log::LogLevel::kOff,
                               iox::log::LogLevel::kFatal,
                               iox::log::LogLevel::kError,
                               iox::log::LogLevel::kWarn,
                               iox::log::LogLevel::kInfo,
                               iox::log::LogLevel::kDebug,
                               iox::log::LogLevel::kVerbose));
#pragma GCC diagnostic pop

TEST_P(IoxLoggerLogLevel_test, LogLevel)
{
    const auto loggerLogLevel = GetParam();
    m_sut.SetLogLevel(loggerLogLevel);

    struct LogLevel
    {
        LogLevel(iox::log::LogLevel logLevel, std::string str)
            : value(logLevel)
            , string(str)
        {
        }
        iox::log::LogLevel value;
        std::string string;
    };

    const std::initializer_list<LogLevel> logEntryLogLevels{{iox::log::LogLevel::kFatal, "Fatal"},
                                                            {iox::log::LogLevel::kError, "Error"},
                                                            {iox::log::LogLevel::kWarn, "Warning"},
                                                            {iox::log::LogLevel::kInfo, "Info"},
                                                            {iox::log::LogLevel::kDebug, "Debug"},
                                                            {iox::log::LogLevel::kVerbose, "Verbose"}};

    for (const auto& logEntryLogLevel : logEntryLogLevels)
    {
        iox::log::LogEntry entry;
        entry.level = logEntryLogLevel.value;

        outBuffer.clear();
        m_sut.Log(entry);

        if (logEntryLogLevel.value <= loggerLogLevel)
        {
            EXPECT_THAT(outBuffer.str().empty(), Eq(false));
            EXPECT_THAT(outBuffer.str().find(logEntryLogLevel.string), Ne(std::string::npos));
            EXPECT_THAT(m_sut.IsEnabled(entry.level), Eq(true));
        }
        else
        {
            EXPECT_THAT(outBuffer.str().empty(), Eq(true));
            EXPECT_THAT(m_sut.IsEnabled(entry.level), Eq(false));
        }
    }
}
} // namespace
