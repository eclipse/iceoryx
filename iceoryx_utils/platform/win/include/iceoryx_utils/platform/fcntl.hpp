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
#ifndef IOX_UTILS_WIN_PLATFORM_FCNTL_HPP
#define IOX_UTILS_WIN_PLATFORM_FCNTL_HPP

#include "iceoryx_utils/platform/types.hpp"

#include <fcntl.h>

#define O_CREAT _O_CREAT
#define O_EXCL _O_EXCL
#define O_TRUN _O_TRUNC
#define O_APPEND _O_APPEND
#define O_SYNC _O_SYNC
#define O_RDONLY _O_RDONLY
#define O_RDWR _O_RDWR
#define O_WRONLY _O_WRONLY
#define O_NONBLOCK 0x0

int iox_open(const char* pathname, int flags, mode_t mode);

#endif // IOX_UTILS_WIN_PLATFORM_FCNTL_HPP
