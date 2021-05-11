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

#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"
#include "iceoryx_posh/internal/runtime/ipc_interface_base.hpp"

namespace iox
{
namespace roudi
{
IceOryxRouDiComponents::IceOryxRouDiComponents(const RouDiConfig_t& roudiConfig) noexcept
    : rouDiMemoryManager(roudiConfig)
    , portManager([&]() -> IceOryxRouDiMemoryManager* {
        // this temporary object will create a roudi IPC channel
        // and close it immediatelly
        // if there was an outdated roudi IPC channel, it will be cleaned up
        // if there is an outdated IPC channel, the start of the apps will be terminated
        runtime::IpcInterfaceBase::cleanupOutdatedIpcChannel(roudi::IPC_CHANNEL_ROUDI_NAME);

        rouDiMemoryManager.createAndAnnounceMemory().or_else([](RouDiMemoryManagerError error) {
            LogFatal() << "Could not create SharedMemory! Error: " << error;
            errorHandler(Error::kROUDI_COMPONENTS__SHARED_MEMORY_UNAVAILABLE, nullptr, iox::ErrorLevel::FATAL);
        });
        return &rouDiMemoryManager;
    }())
{
}

} // namespace roudi
} // namespace iox
