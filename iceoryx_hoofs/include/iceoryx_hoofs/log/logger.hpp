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
#ifndef IOX_HOOFS_LOG_LOGGER_HPP
#define IOX_HOOFS_LOG_LOGGER_HPP

#include "iceoryx_hoofs/cxx/generic_raii.hpp"
#include "iceoryx_hoofs/log/logcommon.hpp"
#include "iceoryx_hoofs/log/logstream.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <string>

namespace iox
{
namespace log
{
/// @todo for asynchronous logging, make the logger an active object according to Herb Sutter
/// https://herbsutter.com/2010/07/12/effective-concurrency-prefer-using-active-objects-instead-of-naked-threads/

class Logger
{
    friend class LogManager;
    /// @todo LogStream needs to call Log(); do we want to make Log() public?
    friend class LogStream;

  public:
    Logger(Logger&& other);
    Logger& operator=(Logger&& rhs);

    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& rhs) = delete;

    /// @brief Getter method for the current LogLevel
    /// @return the current LogLevel
    LogLevel GetLogLevel() const noexcept;

    /// @brief Sets the LogLevel for the Logger
    /// @param[in] logLevel to be set
    void SetLogLevel(const LogLevel logLevel) noexcept;

    /// @brief Sets the LogLevel to the given level for the lifetime of the GenericRAII object and then sets it back to
    /// the previous one
    /// @param[in] logLevel to be set temporarily
    /// @return a scope guard which resets the LogLevel to the value at the time when this method was called
    cxx::GenericRAII SetLogLevelForScope(const LogLevel logLevel) noexcept;

    void SetLogMode(const LogMode logMode) noexcept;
    bool IsEnabled(const LogLevel logLevel) const noexcept;

    LogStream LogFatal() noexcept;
    LogStream LogError() noexcept;
    LogStream LogWarn() noexcept;
    LogStream LogInfo() noexcept;
    LogStream LogDebug() noexcept;
    LogStream LogVerbose() noexcept;

  protected:
    Logger(std::string ctxId, std::string ctxDescription, LogLevel appLogLevel);

    // virtual because of Logger_Mock
    virtual void Log(const LogEntry& entry) const;

  private:
    void Print(const LogEntry entry) const;

    std::atomic<LogLevel> m_logLevel{LogLevel::kVerbose};
    std::atomic<LogLevel> m_logLevelPredecessor{LogLevel::kVerbose};
    std::atomic<LogMode> m_logMode{LogMode::kConsole};
};

} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_LOGGER_HPP
