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

#ifndef IOX_POSH_RUNTIME_IPC_INTERFACE_USER_HPP
#define IOX_POSH_RUNTIME_IPC_INTERFACE_USER_HPP

#include "iceoryx_posh/internal/runtime/ipc_interface_base.hpp"

namespace iox
{
namespace runtime
{
/// @brief Class for using a IPC channel
class IpcInterfaceUser : public IpcInterfaceBase
{
  public:
    /// @brief Constructs a IpcInterfaceUser and opens a IPC channel.
    ///        Therefore, isInitialized should always be called
    ///        before using this class.
    /// @param[in] name Unique identifier of the IPC channel
    /// @param[in] maxMessages maximum number of queued messages
    /// @param[in] message size maximum message size
    IpcInterfaceUser(const RuntimeName_t& name,
                     const uint64_t maxMessages = APP_MAX_MESSAGES,
                     const uint64_t messageSize = APP_MESSAGE_SIZE) noexcept;

    /// @brief The copy constructor and assignment operator are deleted since
    ///         this class manages a resource (IPC channel) which cannot
    ///         be copied. Since move is not needed it is also deleted.
    IpcInterfaceUser(const IpcInterfaceUser&) = delete;
    IpcInterfaceUser& operator=(const IpcInterfaceUser&) = delete;

    /// @brief Not needed therefore deleted
    IpcInterfaceUser(IpcInterfaceUser&&) = delete;
    IpcInterfaceUser& operator=(IpcInterfaceUser&&) = delete;
};

} // namespace runtime
} // namespace iox

#endif // IOX_POSH_RUNTIME_IPC_INTERFACE_USER_HPP