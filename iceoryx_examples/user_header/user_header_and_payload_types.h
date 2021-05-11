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

#ifndef IOX_EXAMPLES_USER_HEADER_AND_PAYLOAD_TYPES_H
#define IOX_EXAMPLES_USER_HEADER_AND_PAYLOAD_TYPES_H

#include <stdint.h>

//! [user-header]
typedef struct
{
    uint64_t publisherTimestamp;
} Header;
//! [user-header]

//! [user-payload]
typedef struct
{
    uint64_t fibonacci;
} Data;
//! [user-payload]

#endif // IOX_EXAMPLES_USER_HEADER_AND_PAYLOAD_TYPES_HPP
