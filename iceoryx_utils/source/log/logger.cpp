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

#include "iceoryx_utils/log/logger.hpp"

#include "iceoryx_utils/log/logging.hpp"
#include "iceoryx_utils/log/logstream.hpp"

#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"


#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace iox
{
namespace log
{
Logger::Logger(IOX_MAYBE_UNUSED std::string ctxId, IOX_MAYBE_UNUSED std::string ctxDescription, LogLevel appLogLevel)
    : m_logLevel(appLogLevel)
{
}

Logger::Logger(Logger&& other)
{
    m_logLevel.store(other.m_logLevel.load(std::memory_order_relaxed), std::memory_order_relaxed);
    m_logMode.store(other.m_logMode.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

Logger& Logger::operator=(Logger&& rhs)
{
    m_logLevel.store(rhs.m_logLevel.load(std::memory_order_relaxed), std::memory_order_relaxed);
    m_logMode.store(rhs.m_logMode.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
}

void Logger::SetLogLevel(const LogLevel logLevel) noexcept
{
    m_logLevel.store(logLevel, std::memory_order_relaxed);
}

void Logger::SetLogMode(const LogMode logMode) noexcept
{
    m_logMode.store(logMode, std::memory_order_relaxed);

    if ((logMode & LogMode::kRemote) == LogMode::kRemote)
    {
        LogError() << "Remote logging not yet supported!";
    }

    if ((logMode & LogMode::kFile) == LogMode::kFile)
    {
        LogError() << "Logging to file not yet supported!";
    }
}

LogStream Logger::LogFatal() noexcept
{
    return LogStream(*this, LogLevel::kFatal);
}

LogStream Logger::LogError() noexcept
{
    return LogStream(*this, LogLevel::kError);
}

LogStream Logger::LogWarn() noexcept
{
    return LogStream(*this, LogLevel::kWarn);
}

LogStream Logger::LogInfo() noexcept
{
    return LogStream(*this, LogLevel::kInfo);
}

LogStream Logger::LogDebug() noexcept
{
    return LogStream(*this, LogLevel::kDebug);
}

LogStream Logger::LogVerbose() noexcept
{
    return LogStream(*this, LogLevel::kVerbose);
}

void Logger::Print(const LogEntry entry) const
{
    // as long as there is only this synchronous logger, buffer the output before using clog to prevent interleaving
    // output because of threaded access
    std::stringstream buffer;

    auto sec = std::chrono::duration_cast<std::chrono::seconds>(entry.time);
    std::time_t time = sec.count();

    auto timeInfo = std::localtime(&time);

    buffer << "\033[0;90m" << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    buffer << "." << std::right << std::setfill('0') << std::setw(3) << entry.time.count() % 1000 << " ";
    buffer << LogLevelColor[cxx::enumTypeAsUnderlyingType(entry.level)]
           << LogLevelText[cxx::enumTypeAsUnderlyingType(entry.level)];
    buffer << "\033[m: " << entry.message << std::endl;
    std::clog << buffer.str();
}

bool Logger::IsEnabled(const LogLevel logLevel) const noexcept
{
    return (logLevel <= m_logLevel.load(std::memory_order_relaxed));
}

void Logger::Log(const LogEntry& entry) const
{
    /// @todo do we want a ringbuffer where we store the last e.g. 100 logs
    /// event if they are below the current log level and print them if case of kFatal?
    if (IsEnabled(entry.level))
    {
        Print(entry);
    }
}

} // namespace log
} // namespace iox
