// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_app.hpp"
#include "iceoryx_posh/roudi/roudi_cmd_line_parser_config_file_option.hpp"
#include "iceoryx_posh/roudi/roudi_config_toml_file_provider.hpp"
#include "iceoryx_utils/platform/getopt.hpp"

#include "test.hpp"

#include <regex>

using namespace ::testing;
using ::testing::Return;

using iox::roudi::IceOryxRouDiApp;
using namespace iox::config;

namespace iox
{
namespace test
{
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

class IceoryxRoudiApp_Child : public IceOryxRouDiApp
{
  public:
    IceoryxRoudiApp_Child(const config::CmdLineArgs_t& cmdLineArgs, const RouDiConfig_t& roudiConfig)
        : IceOryxRouDiApp(cmdLineArgs, roudiConfig)
    {
    }

    bool getVariableRun()
    {
        return m_run;
    }

    iox::log::LogLevel getLogLevel()
    {
        return m_logLevel;
    }

    roudi::MonitoringMode getMonitoringMode()
    {
        return m_monitoringMode;
    }

    void setVariableRun(bool condition)
    {
        m_run = condition;
    }

    uint8_t callRun() noexcept
    {
        return run();
    }
};

/// @brief Test goal: This file tests class roudi app
class IceoryxRoudiApp_test : public Test
{
  public:
    CmdLineParserConfigFileOption cmdLineParser;

    void SetUp() override
    {
        outBuffer.clear();
    }

    void TearDown() override
    {
        // Reset optind to be able to parse again
        optind = 0;
    };

    OutputBuffer outBuffer;
    const std::regex colorCode{"\\x1B\\[([0-9]*;?)*m"};
};

TEST_F(IceoryxRoudiApp_test, VerifyConstructorIsSuccessful)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    EXPECT_TRUE(roudi.getVariableRun());
    EXPECT_EQ(roudi.getLogLevel(), iox::log::LogLevel::kWarn);
    EXPECT_EQ(roudi.getMonitoringMode(), roudi::MonitoringMode::ON);
}

TEST_F(IceoryxRoudiApp_test, CreateTwoRoudiAppIsSuccessful)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    IceoryxRoudiApp_Child roudiTest(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    EXPECT_TRUE(roudiTest.getVariableRun());
}

TEST_F(IceoryxRoudiApp_test, VerifyRunMethodWithFalseConditionReturnExitSuccess)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    roudi.setVariableRun(false);

    uint8_t result = roudi.callRun();

    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(IceoryxRoudiApp_test, ConstructorCalledWithArgUniqueIdTwoTimesReturnError)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--unique-roudi-id";
    char value[] = "4242";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&detectedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_EQ(errorLevel, iox::ErrorLevel::MODERATE);
        });

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    IceoryxRoudiApp_Child roudiTest(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    ASSERT_TRUE(detectedError.has_value());
    EXPECT_EQ(detectedError.value(), iox::Error::kPOPO__TYPED_UNIQUE_ID_ROUDI_HAS_ALREADY_DEFINED_UNIQUE_ID);
}

TEST_F(IceoryxRoudiApp_test, ConstructorCalledWithArgVersionSetRunVariableToFalse)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-v";
    args[0] = &appName[0];
    args[1] = &option[0];

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), iox::RouDiConfig_t().setDefaults());

    EXPECT_FALSE(roudi.getVariableRun());
}

TEST_F(IceoryxRoudiApp_test, VerifyConstructorWithEmptyConfigSetRunVariableToFalse)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];
    const std::string expected = "A RouDiConfig without segments was specified! Please provide a valid config!";

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    iox::RouDiConfig_t roudiConfig;

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), roudiConfig);

    std::string output = std::regex_replace(outBuffer.str(), colorCode, std::string(""));

    EXPECT_FALSE(roudi.getVariableRun());
    EXPECT_NE(output.find(expected), std::string::npos);
}

TEST_F(IceoryxRoudiApp_test, VerifyConstructorUsingConfigWithSegmentWithoutMemPoolSetRunVariableToFalse)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];
    const std::string expected =
        "A RouDiConfig with segments without mempools was specified! Please provide a valid config!";

    auto cmdLineArgs = cmdLineParser.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(cmdLineArgs.has_error());

    iox::mepoo::MePooConfig mempoolConfig;
    auto currentGroup = iox::posix::PosixGroup::getGroupOfCurrentProcess();

    iox::RouDiConfig_t roudiConfig;

    roudiConfig.m_sharedMemorySegments.push_back({currentGroup.getName(), currentGroup.getName(), mempoolConfig});

    IceoryxRoudiApp_Child roudi(cmdLineArgs.value(), roudiConfig);

    std::string output = std::regex_replace(outBuffer.str(), colorCode, std::string(""));

    EXPECT_FALSE(roudi.getVariableRun());
    EXPECT_NE(output.find(expected), std::string::npos);
}

} // namespace test
} // namespace iox
