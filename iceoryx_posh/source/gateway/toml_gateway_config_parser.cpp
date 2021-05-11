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

#include "iceoryx_posh/gateway/toml_gateway_config_parser.hpp"
#include "iceoryx_posh/internal/log/posh_config_logging.hpp"
#include "iceoryx_utils/internal/file_reader/file_reader.hpp"

#include <regex>

iox::cxx::expected<iox::config::GatewayConfig, iox::config::TomlGatewayConfigParseError>
iox::config::TomlGatewayConfigParser::parse()
{
    return iox::config::TomlGatewayConfigParser::parse(DEFAULT_CONFIG_FILE_PATH);
}

iox::cxx::expected<iox::config::GatewayConfig, iox::config::TomlGatewayConfigParseError>
iox::config::TomlGatewayConfigParser::parse(const roudi::ConfigFilePathString_t& path)
{
    iox::config::GatewayConfig config;

    // Set defaults if no path provided.
    if (path.size() == 0)
    {
        LogWarn() << "Invalid file path provided. Falling back to built-in config.";
        config.setDefaults();
        return iox::cxx::success<GatewayConfig>(config);
    }

    /// @todo Replace with C++17 std::filesystem::exists()
    iox::cxx::FileReader configFile(path, "", cxx::FileReader::ErrorMode::Ignore);
    if (!configFile.IsOpen())
    {
        LogWarn() << "Gateway config file not found at: '" << path << "'. Falling back to built-in config.";
        return iox::cxx::success<GatewayConfig>(config);
    }

    LogInfo() << "Using gateway config at: " << path;

    std::shared_ptr<cpptoml::table> parsedToml{nullptr};
    try
    {
        // Load the file
        parsedToml = cpptoml::parse_file(path.c_str());
    }
    catch (const std::exception& parserException)
    {
        auto parserError = iox::config::TomlGatewayConfigParseError::EXCEPTION_IN_PARSER;

        LogWarn() << iox::cxx::convertEnumToString(iox::config::TOML_GATEWAY_CONFIG_FILE_PARSE_ERROR_STRINGS, parserError)
                  << ": " << parserException.what();

        return iox::cxx::error<iox::config::TomlGatewayConfigParseError>(parserError);
    }

    auto result = validate(*parsedToml);
    if (result.has_error())
    {
        return iox::cxx::error<TomlGatewayConfigParseError>(result.get_error());
    }

    // Prepare config object
    auto serviceArray = parsedToml->get_table_array(GATEWAY_CONFIG_SERVICE_TABLE_NAME);
    for (const auto& service : *serviceArray)
    {
        GatewayConfig::ServiceEntry entry;
        auto serviceName = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_NAME);
        auto instance = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_INSTANCE_NAME);
        auto event = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_EVENT_NAME);
        entry.m_serviceDescription =
            iox::capro::ServiceDescription(iox::capro::IdString_t(iox::cxx::TruncateToCapacity, *serviceName),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, *instance),
                                           iox::capro::IdString_t(iox::cxx::TruncateToCapacity, *event));
        config.m_configuredServices.push_back(entry);
    }

    return iox::cxx::success<GatewayConfig>(config);
}

iox::cxx::expected<iox::config::TomlGatewayConfigParseError>
iox::config::TomlGatewayConfigParser::validate(const cpptoml::table& parsedToml) noexcept
{
    // Check for expected fields
    auto serviceArray = parsedToml.get_table_array(GATEWAY_CONFIG_SERVICE_TABLE_NAME);
    if (!serviceArray)
    {
        return iox::cxx::error<TomlGatewayConfigParseError>(TomlGatewayConfigParseError::INCOMPLETE_CONFIGURATION);
    }

    uint8_t count = 0;
    for (const auto& service : *serviceArray)
    {
        ++count;

        auto serviceName = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_NAME);
        auto instance = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_INSTANCE_NAME);
        auto event = service->get_as<std::string>(GATEWAY_CONFIG_SERVICE_EVENT_NAME);

        // check for incomplete service descriptions
        if (!serviceName || !instance || !event)
        {
            return iox::cxx::error<TomlGatewayConfigParseError>(
                TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION);
        }

        // check for invalid characters in strings
        if (hasInvalidCharacter(*serviceName) || hasInvalidCharacter(*instance) || hasInvalidCharacter(*event))
        {
            return iox::cxx::error<TomlGatewayConfigParseError>(
                TomlGatewayConfigParseError::INVALID_SERVICE_DESCRIPTION);
        }
    }

    return iox::cxx::success<>();
}

bool iox::config::TomlGatewayConfigParser::hasInvalidCharacter(const std::string& s) noexcept
{
    // See: https://design.ros2.org/articles/topic_and_service_names.html
    const std::regex regex(REGEX_VALID_CHARACTERS);
    auto isInvalid = !std::regex_match(s, regex);
    return isInvalid;
}
