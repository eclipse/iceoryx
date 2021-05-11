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
#ifndef IOX_POSH_ROUDI_ENVIRONMENT_ROUDI_ENVIRONMENT_HPP
#define IOX_POSH_ROUDI_ENVIRONMENT_ROUDI_ENVIRONMENT_HPP

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"
#include "iceoryx_posh/roudi/memory/iceoryx_roudi_memory_manager.hpp"
#include "iceoryx_posh/testing/roudi_environment/runtime_test_interface.hpp"

#include <atomic>
#include <map>
#include <mutex>

namespace iox
{
namespace roudi
{
class RouDi;

class RouDiEnvironment
{
  public:
    RouDiEnvironment(const RouDiConfig_t& roudiConfig = RouDiConfig_t().setDefaults(),
                     roudi::MonitoringMode monitoringMode = roudi::MonitoringMode::OFF,
                     const uint16_t uniqueRouDiId = 0u);
    virtual ~RouDiEnvironment();

    RouDiEnvironment(RouDiEnvironment&& rhs) = default;
    RouDiEnvironment& operator=(RouDiEnvironment&& rhs) = default;

    RouDiEnvironment(const RouDiEnvironment&) = delete;
    RouDiEnvironment& operator=(const RouDiEnvironment&) = delete;

    void SetInterOpWaitingTime(const std::chrono::milliseconds& v);
    void InterOpWait();

    void CleanupAppResources(const RuntimeName_t& name);

  protected:
    /// @note this is due to ambiguity of the cTor with the default parameter
    enum class BaseCTor
    {
        BASE,
    };
    /// @brief for implementations on top of RouDiEnvironment
    RouDiEnvironment(BaseCTor, const uint16_t uniqueRouDiId = 0u);

    void CleanupRuntimes();

  private:
    RuntimeTestInterface m_runtimes;
#if defined(__APPLE__)
    std::chrono::milliseconds m_interOpWaitingTime = std::chrono::milliseconds(1000);
#else
    std::chrono::milliseconds m_interOpWaitingTime = std::chrono::milliseconds(200);
#endif
    std::unique_ptr<IceOryxRouDiComponents> m_roudiComponents;
    std::unique_ptr<RouDi> m_roudiApp;
};

} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_ENVIRONMENT_ROUDI_ENVIRONMENT_HPP
