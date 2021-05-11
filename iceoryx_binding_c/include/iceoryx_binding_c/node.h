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

#ifndef IOX_BINDING_C_NODE_H
#define IOX_BINDING_C_NODE_H

#include "iceoryx_binding_c/internal/c2cpp_binding.h"

typedef CLASS NodeData* iox_node_t;

/// @brief creates a node in the shared memory
/// @param[in] nodeName name of the node
/// @return handle to the node
iox_node_t iox_node_create(const char* const nodeName);

/// @brief removes a node from the shared memory
/// @param[in] self handle to the node
void iox_node_destroy(iox_node_t const self);

/// @brief acquires the name of the node
/// @param[in] self handle to the node
/// @param[in] name pointer to a memory location where the name can be written to
/// @param[in] nameCapacity size of the memory location where the name is written to
/// @return the actual length of the node name, if the return value is greater
///         then nameCapacity the name is truncated.
///         If name is a nullptr, 0 will be returned.
uint64_t iox_node_get_name(iox_node_t const self, char* const name, const uint64_t nameCapacity);

/// @brief acquires the name of the application's runtime in which the node is stored
/// @param[in] self handle to the node
/// @param[in] name pointer to a memory location where the name can be written to
/// @param[in] nameCapacity size of the memory location where the name is written to
/// @return the actual length of the runtime name, if the return value is greater
///         than nameCapacity the name is truncated.
///         If name is a nullptr, 0 will be returned.
uint64_t iox_node_get_runtime_name(iox_node_t const self, char* const name, const uint64_t nameCapacity);

#endif
