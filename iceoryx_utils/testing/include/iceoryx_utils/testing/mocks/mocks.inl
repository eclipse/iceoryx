// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_MOCKS_MOCKS_INL
#define IOX_UTILS_MOCKS_MOCKS_INL

namespace mocks
{
template <typename T>
void loadSymbol(T& destination, const std::string& functionName)
{
    destination = (T)(dlsym(RTLD_NEXT, functionName.c_str()));
    assert(destination != nullptr && "could not load function");
}

template <typename T>
T assignSymbol(const std::string& functionName)
{
    T destination = (T)(dlsym(RTLD_NEXT, functionName.c_str()));
    assert(destination != nullptr && "could not load function");
    return destination;
}
} // namespace mocks

#endif // IOX_UTILS_MOCKS_MOCKS_INL
