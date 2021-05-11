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

#include "iceoryx_posh/runtime/posh_runtime_single_process.hpp"
namespace iox
{
namespace runtime
{
constexpr bool DO_NOT_MAP_SHARED_MEMORY_INTO_THREAD{false};

PoshRuntime*& getSingleProcessRuntime()
{
    static PoshRuntime* singleProcessRuntime = nullptr;
    return singleProcessRuntime;
}

PoshRuntime& singleProcessRuntimeFactory(cxx::optional<const RuntimeName_t*>)
{
    return *getSingleProcessRuntime();
}

PoshRuntimeSingleProcess::PoshRuntimeSingleProcess(const RuntimeName_t& name) noexcept
    : PoshRuntime(cxx::make_optional<const RuntimeName_t*>(&name), DO_NOT_MAP_SHARED_MEMORY_INTO_THREAD)
{
    auto currentFactory = PoshRuntime::getRuntimeFactory();
    if (currentFactory != nullptr && *currentFactory == PoshRuntime::defaultRuntimeFactory)
    {
        getSingleProcessRuntime() = this;
        PoshRuntime::setRuntimeFactory(singleProcessRuntimeFactory);
    }
    else
    {
        std::cerr << "PoshRuntimeSingleProcess can only created once per process and only if the default PoshRuntime "
                     "factory method is set!\n";
        errorHandler(Error::kPOSH__RUNTIME_IS_CREATED_MULTIPLE_TIMES);
    }
}

PoshRuntimeSingleProcess::~PoshRuntimeSingleProcess()
{
    PoshRuntime::setRuntimeFactory(PoshRuntime::defaultRuntimeFactory);
}

} // namespace runtime
} // namespace iox
