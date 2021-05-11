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

#include "iceoryx_posh/roudi/roudi_config_toml_file_provider.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/roudi/roudi_cmd_line_parser.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/file_reader/file_reader.hpp"
#include "iceoryx_utils/platform/getopt.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

#include <cpptoml.h>
#include <string>

namespace iox
{
namespace config
{
TomlRouDiConfigFileProvider::TomlRouDiConfigFileProvider(config::CmdLineArgs_t& cmdLineArgs)
{
    /// don't print additional output if not running
    if (cmdLineArgs.run)
    {
        if (cmdLineArgs.configFilePath.empty())
        {
            /// @todo Replace with C++17 std::filesystem::exists()
            cxx::FileReader configFile(defaultConfigFilePath, "", cxx::FileReader::ErrorMode::Ignore);

            if (configFile.IsOpen())
            {
                LogInfo() << "No config file provided. Using '" << defaultConfigFilePath << "'";
                m_customConfigFilePath = defaultConfigFilePath;
            }
            else
            {
                LogInfo() << "No config file provided and also not found at '" << defaultConfigFilePath
                          << "'. Falling back to built-in config.";
            }
        }
        m_customConfigFilePath = cmdLineArgs.configFilePath;
    }
}

iox::cxx::expected<iox::RouDiConfig_t, iox::roudi::RouDiConfigFileParseError> TomlRouDiConfigFileProvider::parse()
{
    // Early exit in case no config file path was provided
    if (m_customConfigFilePath.empty())
    {
        iox::RouDiConfig_t defaultConfig;
        defaultConfig.setDefaults();
        return iox::cxx::success<iox::RouDiConfig_t>(defaultConfig);
    }
    auto groupOfCurrentProcess = iox::posix::PosixGroup::getGroupOfCurrentProcess().getName();

    std::shared_ptr<cpptoml::table> parsedFile{nullptr};
    try
    {
        parsedFile = cpptoml::parse_file(m_customConfigFilePath.c_str());
    }
    catch (const std::exception& parserException)
    {
        auto parserError = iox::roudi::RouDiConfigFileParseError::EXCEPTION_IN_PARSER;

        LogWarn() << iox::cxx::convertEnumToString(iox::roudi::ROUDI_CONFIG_FILE_PARSE_ERROR_STRINGS, parserError)
                  << ": " << parserException.what();

        return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(parserError);
    }

    auto general = parsedFile->get_table("general");
    if (!general)
    {
        return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
            iox::roudi::RouDiConfigFileParseError::NO_GENERAL_SECTION);
    }
    auto configFileVersion = general->get_as<uint32_t>("version");
    if (!configFileVersion || *configFileVersion != 1)
    {
        return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
            iox::roudi::RouDiConfigFileParseError::INVALID_CONFIG_FILE_VERSION);
    }

    auto segments = parsedFile->get_table_array("segment");
    if (!segments)
    {
        return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
            iox::roudi::RouDiConfigFileParseError::NO_SEGMENTS);
    }

    if (segments->get().size() > iox::MAX_SHM_SEGMENTS)
    {
        return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
            iox::roudi::RouDiConfigFileParseError::MAX_NUMBER_OF_SEGMENTS_EXCEEDED);
    }

    iox::RouDiConfig_t parsedConfig;
    for (auto segment : *segments)
    {
        auto writer = segment->get_as<std::string>("writer").value_or(groupOfCurrentProcess);
        auto reader = segment->get_as<std::string>("reader").value_or(groupOfCurrentProcess);
        iox::mepoo::MePooConfig mempoolConfig;
        auto mempools = segment->get_table_array("mempool");
        if (!mempools)
        {
            return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
                iox::roudi::RouDiConfigFileParseError::SEGMENT_WITHOUT_MEMPOOL);
        }

        if (mempools->get().size() > iox::MAX_NUMBER_OF_MEMPOOLS)
        {
            return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
                iox::roudi::RouDiConfigFileParseError::MAX_NUMBER_OF_MEMPOOLS_PER_SEGMENT_EXCEEDED);
        }

        for (auto mempool : *mempools)
        {
            auto chunkSize = mempool->get_as<uint32_t>("size");
            auto chunkCount = mempool->get_as<uint32_t>("count");
            if (!chunkSize)
            {
                return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
                    iox::roudi::RouDiConfigFileParseError::MEMPOOL_WITHOUT_CHUNK_SIZE);
            }
            if (!chunkCount)
            {
                return iox::cxx::error<iox::roudi::RouDiConfigFileParseError>(
                    iox::roudi::RouDiConfigFileParseError::MEMPOOL_WITHOUT_CHUNK_COUNT);
            }
            mempoolConfig.addMemPool({*chunkSize, *chunkCount});
        }
        parsedConfig.m_sharedMemorySegments.push_back(
            {iox::posix::PosixGroup::string_t(iox::cxx::TruncateToCapacity, reader),
             iox::posix::PosixGroup::string_t(iox::cxx::TruncateToCapacity, writer),
             mempoolConfig});
    }

    return iox::cxx::success<iox::RouDiConfig_t>(parsedConfig);
}
} // namespace config
} // namespace iox
