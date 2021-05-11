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

#include "iceoryx_posh/testing/roudi_environment/runtime_test_interface.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace roudi
{
using runtime::PoshRuntime;

thread_local PoshRuntime* RuntimeTestInterface::t_activeRuntime{nullptr};
thread_local std::atomic<uint64_t> RuntimeTestInterface::t_currentRouDiContext{0};
std::atomic<uint64_t> RuntimeTestInterface::s_currentRouDiContext{0};

std::mutex RuntimeTestInterface::s_runtimeAccessMutex;

std::map<RuntimeName_t, PoshRuntime*> RuntimeTestInterface::s_runtimes;

RuntimeTestInterface::RuntimeTestInterface()
{
    std::lock_guard<std::mutex> lock(RuntimeTestInterface::s_runtimeAccessMutex);

    PoshRuntime::setRuntimeFactory(RuntimeTestInterface::runtimeFactoryGetInstance);
}

RuntimeTestInterface::~RuntimeTestInterface()
{
    if (m_doCleanupOnDestruction)
    {
        // cleanup holds its own lock
        cleanupRuntimes();

        std::lock_guard<std::mutex> lock(RuntimeTestInterface::s_runtimeAccessMutex);
        PoshRuntime::setRuntimeFactory(PoshRuntime::defaultRuntimeFactory);
    }
}

RuntimeTestInterface::RuntimeTestInterface(RuntimeTestInterface&& rhs)
{
    rhs.m_doCleanupOnDestruction = false;
}
RuntimeTestInterface& RuntimeTestInterface::operator=(RuntimeTestInterface&& rhs)
{
    rhs.m_doCleanupOnDestruction = false;
    return *this;
}

void RuntimeTestInterface::cleanupRuntimes()
{
    std::lock_guard<std::mutex> lock(RuntimeTestInterface::s_runtimeAccessMutex);

    for (const auto& e : RuntimeTestInterface::s_runtimes)
    {
        delete e.second;
    }
    RuntimeTestInterface::s_runtimes.clear();
    RuntimeTestInterface::s_currentRouDiContext.operator++(std::memory_order_relaxed);
}

void RuntimeTestInterface::eraseRuntime(const RuntimeName_t& name)
{
    std::lock_guard<std::mutex> lock(RuntimeTestInterface::s_runtimeAccessMutex);
    auto iter = RuntimeTestInterface::s_runtimes.find(name);
    if (iter != RuntimeTestInterface::s_runtimes.end())
    {
        delete iter->second;
        RuntimeTestInterface::s_runtimes.erase(name);
    }
}

PoshRuntime& RuntimeTestInterface::runtimeFactoryGetInstance(cxx::optional<const RuntimeName_t*> name)
{
    std::lock_guard<std::mutex> lock(RuntimeTestInterface::s_runtimeAccessMutex);

    auto currentRouDiContext = RuntimeTestInterface::s_currentRouDiContext.load(std::memory_order_relaxed);
    if (RuntimeTestInterface::t_currentRouDiContext.load(std::memory_order_relaxed) != currentRouDiContext)
    {
        RuntimeTestInterface::t_currentRouDiContext.store(currentRouDiContext, std::memory_order_relaxed);
        RuntimeTestInterface::t_activeRuntime = nullptr;
    }

    bool nameIsNullopt{!name.has_value()};
    bool invalidGetRuntimeAccess{RuntimeTestInterface::t_activeRuntime == nullptr && nameIsNullopt};
    cxx::Expects(!invalidGetRuntimeAccess);

    if (RuntimeTestInterface::t_activeRuntime != nullptr && nameIsNullopt)
    {
        return *RuntimeTestInterface::t_activeRuntime;
    }

    auto iter = RuntimeTestInterface::s_runtimes.find(*name.value());
    if (iter != RuntimeTestInterface::s_runtimes.end())
    {
        RuntimeTestInterface::t_activeRuntime = iter->second;
    }
    else
    {
        auto runtimeImpl = new runtime::PoshRuntime(name, false);
        RuntimeTestInterface::s_runtimes.insert({*name.value(), runtimeImpl});

        RuntimeTestInterface::t_activeRuntime = runtimeImpl;
    }

    return *RuntimeTestInterface::t_activeRuntime;
}

} // namespace roudi
} // namespace iox
