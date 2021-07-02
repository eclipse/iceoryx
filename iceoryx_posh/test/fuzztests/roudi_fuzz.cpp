// Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "roudi_fuzz.hpp"
#include "string_to_ipc_message.hpp"

RouDiFuzz::RouDiFuzz(iox::roudi::RouDiMemoryInterface& roudiMemoryInterface,
                     iox::roudi::PortManager& portManager,
                     iox::roudi::RouDi::RoudiStartupParameters aStartupParameter) noexcept
    : iox::roudi::RouDi(roudiMemoryInterface, portManager, aStartupParameter)
{
}

void RouDiFuzz::processMessageFuzz(std::string aMessage) noexcept
{
    iox::runtime::IpcMessage ipcMessage;
    StringToIPCMessage::setMessageFromString(aMessage.c_str(), ipcMessage);
    iox::runtime::IpcMessageType cmd = iox::runtime::stringToIpcMessageType(ipcMessage.getElementAtIndex(0).c_str());
    std::string processName = ipcMessage.getElementAtIndex(1);
    iox::roudi::RouDi::processMessage(ipcMessage, cmd, iox::RuntimeName_t(iox::cxx::TruncateToCapacity, processName));
}
