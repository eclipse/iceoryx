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

#include "iceoryx_utils/log/logmanager.hpp"

#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/log/logger.hpp"
#include "logging_internal.hpp"

#include <mutex>

namespace iox
{
namespace log
{
LogManager& LogManager::GetLogManager()
{
    static LogManager manager;
    return manager;
}

Logger& LogManager::CreateLogContext(IOX_MAYBE_UNUSED std::string ctxId,
                                     IOX_MAYBE_UNUSED std::string ctxDescription,
                                     LogLevel appDefLogLevel) noexcept
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);

    auto& logManager = GetLogManager();
    auto logger = logManager.m_loggers.find(ctxId);
    if (logger != logManager.m_loggers.end())
    {
        return logger->second;
    }
    else
    {
        auto newLogger = logManager.m_loggers.emplace(ctxId, Logger(ctxId, ctxDescription, appDefLogLevel));
        return newLogger.first->second;
    }
}

LogLevel LogManager::DefaultLogLevel() const noexcept
{
    return m_defaultLogLevel.load(std::memory_order_relaxed);
}

void LogManager::SetDefaultLogLevel(const LogLevel logLevel, const LogLevelOutput logLevelOutput) noexcept
{
    m_defaultLogLevel.store(logLevel, std::memory_order_relaxed);

    for (auto& logger : m_loggers)
    {
        logger.second.SetLogLevel(logLevel);
    }

    if (logLevelOutput == LogLevelOutput::kDisplayLogLevel)
    {
        std::clog << "Log level set to: " << LogLevelColor[cxx::enumTypeAsUnderlyingType(logLevel)]
                  << LogLevelText[cxx::enumTypeAsUnderlyingType(logLevel)] << "\033[m" << std::endl;
    }
}

LogMode LogManager::DefaultLogMode() const noexcept
{
    return m_defaultLogMode.load(std::memory_order_relaxed);
}

void LogManager::SetDefaultLogMode(const LogMode logMode) noexcept
{
    m_defaultLogMode.store(logMode, std::memory_order_relaxed);

    for (auto& logger : m_loggers)
    {
        logger.second.SetLogMode(logMode);
    }

    if ((logMode & LogMode::kRemote) == LogMode::kRemote)
    {
        LogError() << "Remote logging not yet supported!";
    }

    if ((logMode & LogMode::kFile) == LogMode::kFile)
    {
        LogError() << "Logging to file not yet supported!";
    }
}

} // namespace log
} // namespace iox
