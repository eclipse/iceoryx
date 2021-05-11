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
#ifndef IOX_UTILS_MOCKS_LOGGER_MOCK_HPP
#define IOX_UTILS_MOCKS_LOGGER_MOCK_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/log/logger.hpp"
#include "iceoryx_utils/log/logstream.hpp"

#include <vector>

class Logger_Mock : public iox::log::Logger
{
  public:
    Logger_Mock()
        : iox::log::Logger("Mock", "Context for logger mock!", iox::log::LogLevel::kVerbose)
    {
    }

    void Log(const iox::log::LogEntry& entry) const override
    {
        m_logs.push_back(entry);
    }

    mutable std::vector<iox::log::LogEntry> m_logs;
};

#endif // IOX_UTILS_MOCKS_LOGGER_MOCK_HPP
