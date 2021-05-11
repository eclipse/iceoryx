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

#include "iceoryx_posh/mepoo/chunk_header.hpp"


extern "C" {
#include "iceoryx_binding_c/chunk.h"
}

using namespace iox::mepoo;

void* iox_chunk_header_to_user_payload(iox_chunk_header_t* const chunkHeader)
{
    return reinterpret_cast<ChunkHeader*>(chunkHeader)->userPayload();
}

const void* iox_chunk_header_to_user_payload_const(const iox_chunk_header_t* const chunkHeader)
{
    return reinterpret_cast<const ChunkHeader*>(chunkHeader)->userPayload();
}

void* iox_chunk_header_to_user_header(iox_chunk_header_t* const chunkHeader)
{
    return reinterpret_cast<ChunkHeader*>(chunkHeader)->userHeader();
}

const void* iox_chunk_header_to_user_header_const(const iox_chunk_header_t* const chunkHeader)
{
    return reinterpret_cast<const ChunkHeader*>(chunkHeader)->userHeader();
}

iox_chunk_header_t* iox_chunk_header_from_user_payload(void* const userPayload)
{
    return reinterpret_cast<iox_chunk_header_t*>(ChunkHeader::fromUserPayload(userPayload));
}

const iox_chunk_header_t* iox_chunk_header_from_user_payload_const(const void* const userPayload)
{
    return reinterpret_cast<const iox_chunk_header_t*>(ChunkHeader::fromUserPayload(userPayload));
}
