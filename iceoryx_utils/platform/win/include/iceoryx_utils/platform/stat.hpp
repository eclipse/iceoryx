// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_WIN_PLATFORM_STAT_HPP
#define IOX_UTILS_WIN_PLATFORM_STAT_HPP

#include <io.h>
#include <sys/stat.h>

#define S_IRUSR 0
#define S_IWUSR 1
#define S_IRGRP 2
#define S_IWGRP 3
#define S_IROTH 4
#define S_IWOTH 5
#define S_IRWXU 6
#define S_IXUSR 7
#define S_IXGRP 8
#define S_IRWXO 9

#endif // IOX_UTILS_WIN_PLATFORM_STAT_HPP
