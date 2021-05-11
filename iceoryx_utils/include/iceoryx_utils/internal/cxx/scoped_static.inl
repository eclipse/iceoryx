// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_CXX_SCOPED_STATIC_INL
#define IOX_UTILS_CXX_SCOPED_STATIC_INL

namespace iox
{
namespace cxx
{
template <typename T, typename... CTorArgs>
inline GenericRAII makeScopedStatic(T& memory, CTorArgs&&... ctorArgs)
{
    memory.emplace(std::forward<CTorArgs>(ctorArgs)...);
    return GenericRAII([] {}, [&memory] { memory.reset(); });
}
} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_SCOPED_STATIC_INL
