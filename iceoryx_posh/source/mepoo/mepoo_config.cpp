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

#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"

namespace iox
{
namespace mepoo
{
const MePooConfig::MePooConfigContainerType* MePooConfig::getMemPoolConfig() const noexcept
{
    return &m_mempoolConfig;
}

void MePooConfig::addMemPool(MePooConfig::Entry f_entry) noexcept
{
    if (m_mempoolConfig.size() < m_mempoolConfig.capacity())
    {
        m_mempoolConfig.push_back(f_entry);
    }
    else
    {
        LogFatal() << "Maxmimum number of mempools reached, no more mempools available";
        errorHandler(Error::kMEPOO__MAXIMUM_NUMBER_OF_MEMPOOLS_REACHED, nullptr, ErrorLevel::FATAL);
    }
}

/// this is the default memory pool configuration if no one is provided by the user
MePooConfig& MePooConfig::setDefaults() noexcept
{
    m_mempoolConfig.push_back({128, 10000});
    m_mempoolConfig.push_back({1024, 5000});
    m_mempoolConfig.push_back({1024 * 16, 1000});
    m_mempoolConfig.push_back({1024 * 128, 200});
    m_mempoolConfig.push_back({1024 * 512, 50});
    m_mempoolConfig.push_back({1024 * 1024, 30});
    m_mempoolConfig.push_back({1024 * 1024 * 4, 10});

    return *this;
}

MePooConfig& MePooConfig::optimize() noexcept
{
    auto config = m_mempoolConfig;
    m_mempoolConfig.clear();

    std::sort(config.begin(), config.end(), [](const Entry& lhs, const Entry& rhs) { return lhs.m_size < rhs.m_size; });

    MePooConfig::Entry newEntry{0u, 0u};

    for (const auto& entry : config)
    {
        if (entry.m_size != newEntry.m_size)
        {
            if (newEntry.m_size != 0u)
            {
                m_mempoolConfig.push_back(newEntry);
            }
            newEntry.m_size = entry.m_size;
            newEntry.m_chunkCount = entry.m_chunkCount;
        }
        else
        {
            newEntry.m_chunkCount += entry.m_chunkCount;
        }
    }

    if (newEntry.m_size != 0u)
    {
        m_mempoolConfig.push_back(newEntry);
    }

    return *this;
}

} // namespace mepoo
} // namespace iox
