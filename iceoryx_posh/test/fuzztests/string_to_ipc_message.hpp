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

#ifndef IOX_POSH_FUZZTESTS_STRINGTOIPCMESSAGE_HPP
#define IOX_POSH_FUZZTESTS_STRINGTOIPCMESSAGE_HPP

#include "iceoryx_posh/internal/runtime/ipc_interface_base.hpp"

/// @brief The StringToIPCMessage is a class which inherits from iox::runtime::IpcInterfaceBase to make the protected
/// method iox::runtime::IpcInterfaceBase::setMessageFromString public and accessible for the fuzz test.
class StringToIPCMessage : public iox::runtime::IpcInterfaceBase
{
  public:
    StringToIPCMessage(const iox::RuntimeName_t& name, const int64_t maxMessages, const int64_t messageSize) noexcept;

    /// @brief Set the content of answer from buffer.
    /// @param[in] buffer Raw message as char pointer
    /// @param[out] answer Raw message is setting this IpcMessage
    /// @return answer.isValid()
    using iox::runtime::IpcInterfaceBase::setMessageFromString;
};

#endif /*IOX_POSH_FUZZTESTS_STRINGTOIPCMESSAGE*/
