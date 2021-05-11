// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

#if !defined(_WIN32)
#include "test.hpp"

#include "iceoryx_posh/roudi/roudi_cmd_line_parser.hpp"

using namespace ::testing;
using ::testing::Return;

using namespace iox::roudi;
using namespace iox::config;
using namespace iox::log;
using namespace iox::units;
using namespace iox::version;

namespace iox
{
namespace config
{
bool operator==(const CmdLineArgs_t& lhs, const CmdLineArgs_t& rhs)
{
    return (lhs.monitoringMode == rhs.monitoringMode) && (lhs.logLevel == rhs.logLevel)
           && (lhs.compatibilityCheckLevel == rhs.compatibilityCheckLevel)
           && (lhs.processKillDelay == rhs.processKillDelay) && (lhs.uniqueRouDiId == rhs.uniqueRouDiId)
           && (lhs.run == rhs.run) && (lhs.configFilePath == rhs.configFilePath);
}
} // namespace config
} // namespace iox

namespace iox
{
namespace test
{
class CmdLineParser_test : public Test
{
  public:
    void SetUp(){};
    void TearDown()
    {
        // Reset optind to be able to parse again
        optind = 0;
    };

    void testLogLevel(uint8_t numberOfArgs, char* args[], LogLevel level)
    {
        CmdLineParser sut;
        auto result = sut.parse(numberOfArgs, args);

        ASSERT_FALSE(result.has_error());
        EXPECT_EQ(result.value().logLevel, level);
        EXPECT_TRUE(result.value().run);

        // Reset optind to be able to parse again
        optind = 0;
    }

    void testMonitoringMode(uint8_t numberOfArgs, char* args[], MonitoringMode mode)
    {
        CmdLineParser sut;
        auto result = sut.parse(numberOfArgs, args);

        ASSERT_FALSE(result.has_error());
        EXPECT_EQ(result.value().monitoringMode, mode);
        EXPECT_TRUE(result.value().run);

        // Reset optind to be able to parse again
        optind = 0;
    }

    void testCompatibilityLevel(uint8_t numberOfArgs, char* args[], CompatibilityCheckLevel level)
    {
        CmdLineParser sut;
        auto result = sut.parse(numberOfArgs, args);

        ASSERT_FALSE(result.has_error());
        EXPECT_EQ(result.value().compatibilityCheckLevel, level);
        EXPECT_TRUE(result.value().run);

        // Reset optind to be able to parse again
        optind = 0;
    }
};

TEST_F(CmdLineParser_test, NoOptionLeadsToDefaultValues)
{
    constexpr uint8_t NUMBER_OF_ARGS{1U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    args[0] = &appName[0];
    const CmdLineArgs_t defaultValues;

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(result.value(), defaultValues);
}

TEST_F(CmdLineParser_test, WrongOptionLeadsUnkownOptionResult)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--ICanHazLulz";
    args[0] = &appName[0];
    args[1] = &option[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), CmdLineParserResult::UNKNOWN_OPTION_USED);
}

TEST_F(CmdLineParser_test, HelpLongOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--help";
    args[0] = &appName[0];
    args[1] = &option[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, HelpShortOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-h";
    args[0] = &appName[0];
    args[1] = &option[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, VersionShortOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-v";
    args[0] = &appName[0];
    args[1] = &option[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, VersionLongOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{2U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--version";
    args[0] = &appName[0];
    args[1] = &option[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, MonitoringModeOptionsLeadToCorrectMode)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    MonitoringMode modeArray[] = {MonitoringMode::ON, MonitoringMode::OFF};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char optionArray[][20] = {"-m", "--monitoring-mode"};
    char valueArray[][10] = {"on", "off"};
    args[0] = &appName[0];

    for (auto optionValue : optionArray)
    {
        args[1] = optionValue;
        uint8_t i{0U};
        for (auto expectedValue : modeArray)
        {
            args[2] = valueArray[i];
            testMonitoringMode(NUMBER_OF_ARGS, args, expectedValue);
            i++;
        }
    }
}

TEST_F(CmdLineParser_test, WrongMonitoringModeOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-m";
    char wrongValue[] = "DontBlink";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &wrongValue[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, LogLevelOptionsLeadToCorrectLogLevel)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    LogLevel loglevelArray[] = {LogLevel::kOff,
                                LogLevel::kFatal,
                                LogLevel::kError,
                                LogLevel::kWarn,
                                LogLevel::kInfo,
                                LogLevel::kDebug,
                                LogLevel::kVerbose};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char optionArray[][20] = {"-l", "--log-level"};
    char valueArray[][10] = {"off", "fatal", "error", "warning", "info", "debug", "verbose"};
    args[0] = &appName[0];

    for (auto optionValue : optionArray)
    {
        args[1] = optionValue;
        uint8_t i{0U};
        for (auto expectedValue : loglevelArray)
        {
            args[2] = valueArray[i];
            testLogLevel(NUMBER_OF_ARGS, args, expectedValue);
            i++;
        }
    }
}

TEST_F(CmdLineParser_test, WrongLogLevelOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-l";
    char wrongValue[] = "TimeyWimey";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &wrongValue[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, KillDelayLongOptionLeadsToCorrectDelay)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--kill-delay";
    char value[] = "73";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(result.value().processKillDelay, Duration::fromSeconds(73));
    EXPECT_TRUE(result.value().run);
}

TEST_F(CmdLineParser_test, KillDelayShortOptionLeadsToCorrectDelay)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-k";
    char value[] = "42";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_EQ(result.value().processKillDelay, Duration::fromSeconds(42));
    EXPECT_TRUE(result.value().run);
}

TEST_F(CmdLineParser_test, KillDelayOptionOutOfBoundsLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--kill-delay";
    char value[] = "4294967296"; // MAX_PROCESS_KILL_DELAY + 1
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, CompatibilityLevelOptionsLeadToCorrectCompatibilityLevel)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    CompatibilityCheckLevel loglevelArray[] = {CompatibilityCheckLevel::OFF,
                                               CompatibilityCheckLevel::MAJOR,
                                               CompatibilityCheckLevel::MINOR,
                                               CompatibilityCheckLevel::PATCH,
                                               CompatibilityCheckLevel::COMMIT_ID,
                                               CompatibilityCheckLevel::BUILD_DATE};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char optionArray[][20] = {"-x", "--compatibility"};
    char valueArray[][10] = {"off", "major", "minor", "patch", "commitId", "buildDate"};
    args[0] = &appName[0];

    for (auto optionValue : optionArray)
    {
        args[1] = optionValue;
        uint8_t i{0U};
        for (auto expectedValue : loglevelArray)
        {
            args[2] = valueArray[i];
            testCompatibilityLevel(NUMBER_OF_ARGS, args, expectedValue);
            i++;
        }
    }
}

TEST_F(CmdLineParser_test, WrongCompatibilityLevelOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-x";
    char wrongValue[] = "AmyPond";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &wrongValue[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, UniqueIdLongOptionLeadsToCorrectUniqueId)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "--unique-roudi-id";
    char value[] = "4242";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    ASSERT_TRUE(result.value().uniqueRouDiId.has_value());
    EXPECT_EQ(result.value().uniqueRouDiId.value(), 4242);
    EXPECT_TRUE(result.value().run);
}

TEST_F(CmdLineParser_test, UniqueIdShortOptionLeadsToCorrectUniqueId)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-u";
    char value[] = "4242";
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &value[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    ASSERT_TRUE(result.value().uniqueRouDiId.has_value());
    EXPECT_EQ(result.value().uniqueRouDiId.value(), 4242);
    EXPECT_TRUE(result.value().run);
}

TEST_F(CmdLineParser_test, OutOfBoundsUniqueIdOptionLeadsToProgrammNotRunning)
{
    constexpr uint8_t NUMBER_OF_ARGS{3U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char option[] = "-u";
    char wrongValue[] = "65536"; // MAX_ROUDI_ID + 1
    args[0] = &appName[0];
    args[1] = &option[0];
    args[2] = &wrongValue[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(result.has_error());
    EXPECT_FALSE(result.value().run);
}

TEST_F(CmdLineParser_test, CmdLineParsingModeEqualToOneHandlesOnlyTheFirstOption)
{
    constexpr uint8_t NUMBER_OF_ARGS{5U};
    char* args[NUMBER_OF_ARGS];
    char appName[] = "./foo";
    char uniqueIdOption[] = "-u";
    char idValue[] = "4242";
    char killOption[] = "-k";
    char killValue[] = "42";
    args[0] = &appName[0];
    args[1] = &uniqueIdOption[0];
    args[2] = &idValue[0];
    args[3] = &killOption[0];
    args[4] = &killValue[0];

    CmdLineParser sut;
    auto result = sut.parse(NUMBER_OF_ARGS, args, CmdLineParser::CmdLineArgumentParsingMode::ONE);

    ASSERT_FALSE(result.has_error());
    ASSERT_TRUE(result.value().uniqueRouDiId.has_value());
    EXPECT_EQ(result.value().uniqueRouDiId.value(), 4242);
    EXPECT_EQ(result.value().processKillDelay, iox::roudi::PROCESS_DEFAULT_KILL_DELAY); // default value for kill delay
    EXPECT_TRUE(result.value().run);

    optind = 0;

    auto res = sut.parse(NUMBER_OF_ARGS, args);

    ASSERT_FALSE(res.has_error());
    ASSERT_TRUE(res.value().uniqueRouDiId.has_value());
    EXPECT_EQ(res.value().uniqueRouDiId.value(), 4242);
    EXPECT_EQ(res.value().processKillDelay, Duration::fromSeconds(42));
    EXPECT_TRUE(result.value().run);
}

} // namespace test
} // namespace iox
#endif
