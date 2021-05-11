// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_BINDING_C_USER_TRIGGER_H
#define IOX_BINDING_C_USER_TRIGGER_H

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/c2cpp_binding.h"
#include "iceoryx_binding_c/types.h"

/// @brief user trigger handle
typedef CLASS UserTrigger* iox_user_trigger_t;

/// @brief initialize user trigger handle
/// @param[in] self pointer to preallocated memory of size = sizeof(iox_user_trigger_storage_t)
/// @return handle to user trigger
iox_user_trigger_t iox_user_trigger_init(iox_user_trigger_storage_t* self);

/// @brief deinitialize user trigger handle
/// @param[in] self handle to user trigger
void iox_user_trigger_deinit(iox_user_trigger_t const self);

/// @brief trigger a user trigger
/// @note a user trigger cannot be triggered when it is not attached
/// @param[in] self handle to user trigger
void iox_user_trigger_trigger(iox_user_trigger_t const self);

/// @brief was the user trigger triggered
/// @param[in] self handle to user trigger
/// @note The hasTrigger state will be reset after it was handled by a WaitSet/Listener
/// @return returns true if the user trigger was triggered, otherwise false
bool iox_user_trigger_has_triggered(iox_user_trigger_t const self);

#endif
