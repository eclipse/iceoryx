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

#ifndef IOX_POSH_RUNTIME_IPC_INTERFACE_CREATOR_HPP
#define IOX_POSH_RUNTIME_IPC_INTERFACE_CREATOR_HPP

#include "iceoryx_posh/internal/runtime/ipc_interface_base.hpp"
#include "iceoryx_utils/posix_wrapper/file_lock.hpp"

namespace iox
{
namespace runtime
{
/// @brief Class for creating and handling a IPC channel
/// @note This class makes sures the IPC channel is created uniquely
class IpcInterfaceCreator : public IpcInterfaceBase
{
  public:
    /// @brief Constructs a IpcInterfaceCreator and opens a new IPC channel.
    ///        If it fails isInitialized will return false. Therefore, isInitialized
    ///        should always be called before using this class.
    /// @param[in] name Unique identifier of the IPC channel
    /// @param[in] maxMessages maximum number of queued messages
    /// @param[in] message size maximum message size
    IpcInterfaceCreator(const RuntimeName_t& name,
                        const uint64_t maxMessages = ROUDI_MAX_MESSAGES,
                        const uint64_t messageSize = ROUDI_MESSAGE_SIZE) noexcept;

    /// @brief The copy constructor and assignment operator is deleted since
    ///         this class manages a resource (IPC channel) which cannot
    ///         be copied. Move is also not needed, it is also deleted.
    IpcInterfaceCreator(const IpcInterfaceCreator&) = delete;
    IpcInterfaceCreator& operator=(const IpcInterfaceCreator&) = delete;

    /// @brief Not needed therefore deleted
    IpcInterfaceCreator(IpcInterfaceCreator&&) = delete;
    IpcInterfaceCreator& operator=(IpcInterfaceCreator&&) = delete;

  private:
    friend class IpcRuntimeInterface;
    void cleanupResource();
    posix::FileLock m_fileLock;
};

} // namespace runtime
} // namespace iox

#endif // IOX_POSH_RUNTIME_IPC_INTERFACE_CREATOR_HPP