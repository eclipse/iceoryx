// Copyright (c) 2019 - 2020 by Robert Bosch GmbH All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_POSH_ROUDI_ICEORYX_ROUDI_COMPONENTS_HPP
#define IOX_POSH_ROUDI_ICEORYX_ROUDI_COMPONENTS_HPP

#include "iceoryx_posh/internal/roudi/port_manager.hpp"
#include "iceoryx_posh/roudi/memory/iceoryx_roudi_memory_manager.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/generic_raii.hpp"

namespace iox
{
namespace roudi
{
struct IceOryxRouDiComponents
{
  public:
    IceOryxRouDiComponents(const RouDiConfig_t& roudiConfig) noexcept;

    virtual ~IceOryxRouDiComponents() = default;

    /// @brief Handles MemoryProvider and MemoryBlocks
    IceOryxRouDiMemoryManager rouDiMemoryManager;

    /// @brief Handles the ports in shared memory
    PortManager portManager;
};
} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_ICEORYX_ROUDI_COMPONENTS_HPP
