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
#include "iceoryx_posh/roudi/cmd_line_args.hpp"
#include "iceoryx_posh/roudi/roudi_config_toml_file_provider.hpp"

#include "iceoryx/tests/posh/moduletests/test_input_path.hpp"
#include "test.hpp"

using namespace ::testing;

namespace
{
using ParseErrorInputFile_t = std::pair<iox::roudi::RouDiConfigFileParseError, iox::roudi::ConfigFilePathString_t>;
};

class RoudiConfigTomlFileProvider_test : public TestWithParam<ParseErrorInputFile_t>
{
  public:
    void SetUp()
    {
        // get file path via cmake
        m_cmdLineArgs.configFilePath = iox::testing::TEST_INPUT_PATH;
    }

    void TearDown()
    {
    }

    iox::config::CmdLineArgs_t m_cmdLineArgs;
};

TEST_F(RoudiConfigTomlFileProvider_test, ParseDefaultConfigIsSuccessful)
{
    iox::roudi::ConfigFilePathString_t emptyConfigFilePath;
    m_cmdLineArgs.configFilePath = emptyConfigFilePath;

    iox::config::TomlRouDiConfigFileProvider sut(m_cmdLineArgs);

    auto result = sut.parse();

    EXPECT_FALSE(result.has_error());
}

/// we require INSTANTIATE_TEST_CASE_P since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
INSTANTIATE_TEST_CASE_P(
    ParseAllMalformedInputConfigFiles,
    RoudiConfigTomlFileProvider_test,
    Values(ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::NO_GENERAL_SECTION,
                                 "roudi_config_error_no_general.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::INVALID_CONFIG_FILE_VERSION,
                                 "roudi_config_error_invalid_version.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::NO_SEGMENTS,
                                 "roudi_config_error_no_segments.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::MAX_NUMBER_OF_SEGMENTS_EXCEEDED,
                                 "roudi_config_error_max_segments_exceeded.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::SEGMENT_WITHOUT_MEMPOOL,
                                 "roudi_config_error_segment_without_mempool.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::MAX_NUMBER_OF_MEMPOOLS_PER_SEGMENT_EXCEEDED,
                                 "roudi_config_error_max_mempools_exceeded.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::MEMPOOL_WITHOUT_CHUNK_SIZE,
                                 "roudi_config_error_mempool_without_chunk_size.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::MEMPOOL_WITHOUT_CHUNK_COUNT,
                                 "roudi_config_error_mempool_without_chunk_count.toml"},
           ParseErrorInputFile_t{iox::roudi::RouDiConfigFileParseError::EXCEPTION_IN_PARSER,
                                 "toml_parser_exception.toml"}));
#pragma GCC diagnostic pop

TEST_P(RoudiConfigTomlFileProvider_test, ParseMalformedInputFileCausesError)
{
    const auto parseErrorInputFile = GetParam();

    m_cmdLineArgs.configFilePath.append(iox::cxx::TruncateToCapacity, parseErrorInputFile.second);

    iox::config::TomlRouDiConfigFileProvider sut(m_cmdLineArgs);

    auto result = sut.parse();

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(parseErrorInputFile.first, result.get_error());
}
