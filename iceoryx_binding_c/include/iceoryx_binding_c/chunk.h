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

#ifndef IOX_BINDING_C_CHUNK_H
#define IOX_BINDING_C_CHUNK_H

#include "iceoryx_binding_c/types.h"

/// @brief gets the user-payload from the chunk-header
/// @param[in] chunkHeader pointer to the chunk-header
/// @return pointer to the user-payload
void* iox_chunk_header_to_user_payload(iox_chunk_header_t* const chunkHeader);

/// @brief gets the const user-payload from the const chunk-header
/// @param[in] chunkHeader const pointer to the chunk-header
/// @return const pointer to the user-payload
const void* iox_chunk_header_to_user_payload_const(const iox_chunk_header_t* const chunkHeader);

/// @brief gets the user-header from the chunk-header
/// @param[in] chunkHeader pointer to the chunk-header
/// @return pointer to the user-header
void* iox_chunk_header_to_user_header(iox_chunk_header_t* const chunkHeader);

/// @brief gets the const user-payload from the const chunk-header
/// @param[in] chunkHeader const pointer to the chunk-header
/// @return const pointer to the user-header
const void* iox_chunk_header_to_user_header_const(const iox_chunk_header_t* const chunkHeader);

/// @brief gets the chunk-header from the user-payload
/// @param[in] userPayload pointer to the user-payload
/// @return pointer to the chunk-header
iox_chunk_header_t* iox_chunk_header_from_user_payload(void* const userPayload);

/// @brief gets the const chunk-header from the const user-payload
/// @param[in] userPayload const pointer to the user-payload
/// @return const pointer to the chunk-header
const iox_chunk_header_t* iox_chunk_header_from_user_payload_const(const void* const userPayload);

#endif
