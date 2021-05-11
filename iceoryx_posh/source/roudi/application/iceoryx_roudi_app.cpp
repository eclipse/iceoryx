// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2019 - 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/roudi/iceoryx_roudi_app.hpp"

#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/scoped_static.hpp"

namespace iox
{
namespace roudi
{
IceOryxRouDiApp::IceOryxRouDiApp(const config::CmdLineArgs_t& cmdLineArgs, const RouDiConfig_t& roudiConfig) noexcept
    : RouDiApp(cmdLineArgs, roudiConfig)
{
}

uint8_t IceOryxRouDiApp::run() noexcept
{
    if (m_run)
    {
        static cxx::optional<IceOryxRouDiComponents> m_rouDiComponents;
        auto componentsScopeGuard = cxx::makeScopedStatic(m_rouDiComponents, m_config);

        static cxx::optional<RouDi> roudi;
        auto roudiScopeGuard =
            cxx::makeScopedStatic(roudi,
                                  m_rouDiComponents.value().rouDiMemoryManager,
                                  m_rouDiComponents.value().portManager,
                                  RouDi::RoudiStartupParameters{m_monitoringMode,
                                                                true,
                                                                RouDi::RuntimeMessagesThreadStart::IMMEDIATE,
                                                                m_compatibilityCheckLevel,
                                                                m_processKillDelay});
        waitForSignal();
    }
    return EXIT_SUCCESS;
}
} // namespace roudi
} // namespace iox
