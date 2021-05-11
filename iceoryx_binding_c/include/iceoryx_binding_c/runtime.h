// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_BINDING_C_POSH_RUNTIME_H
#define IOX_BINDING_C_POSH_RUNTIME_H

#include "internal/c2cpp_binding.h"

/// @brief initializes a new posh runtime with a given name
/// @param[in] name name of the posh runtime
void iox_runtime_init(const char* const name);

/// @brief retrieves the instance-name of the current posh runtime instance
/// @param[in] name char pointer to preallocated memory
/// @param[in] nameLength size of the preallocated memory.
/// @return The length of the instance-name. If the instance-name is longer then nameLength a
///         number greater nameLength is returned and the instance-name, truncated
///         to nameLength, is written into the memory location of name.
///         If name is a nullptr, 0 will be returned.
uint64_t iox_runtime_get_instance_name(char* const name, const uint64_t nameLength);

/// @brief initiates the shutdown of the runtime to unblock all potentially blocking publisher
/// with the iox_SubscriberTooSlowPolicy::SubscriberTooSlowPolicy_WAIT_FOR_SUBSCRIBER option set
void iox_runtime_shutdown();

#endif
