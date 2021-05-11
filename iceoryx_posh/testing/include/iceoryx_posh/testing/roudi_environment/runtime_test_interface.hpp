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
#ifndef IOX_POSH_ROUDI_ENVIRONMENT_RUNTIME_TEST_INTERFACE_HPP
#define IOX_POSH_ROUDI_ENVIRONMENT_RUNTIME_TEST_INTERFACE_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"

#include <atomic>
#include <map>
#include <mutex>

namespace iox
{
namespace runtime
{
class PoshRuntime;
}
namespace roudi
{
class RuntimeTestInterface
{
    friend class RouDiEnvironment;

  private:
    bool m_doCleanupOnDestruction{true};

    thread_local static runtime::PoshRuntime* t_activeRuntime;
    thread_local static std::atomic<uint64_t> t_currentRouDiContext;
    static std::atomic<uint64_t> s_currentRouDiContext;

    static std::mutex s_runtimeAccessMutex;

    static std::map<RuntimeName_t, runtime::PoshRuntime*> s_runtimes;

    /// This is a replacement for the PoshRuntime::getInstance factory method
    /// @param [in] name is an optional containing the name of the runtime
    /// @return a reference to a PoshRuntime
    /// @note The runtime is stored in a vector and a thread local storage.
    ///
    ///       In a multithreaded environment each thread has its own runtime. This means that for each thread
    ///       iox::runtime::PoshRuntime::initRuntime(...) must be called. Threads that call initRuntime(...)
    ///       with the same name, share the same runtime.
    ///
    ///       It is also possible to use multiple runtimes in a singlethreaded environment. There are some points to
    ///       take care of, though.  There are some classes that call PoshRuntime::getInstance(). In this
    ///       case the already created runtime is used. In the context of the roudi environment this
    ///       means that the active runtime is used. The active runtime is the one from the latest
    ///       iox::runtime::PoshRuntime::initRuntime(...) call.
    ///       Places where a getInstance() call happens are:
    ///         - constructors of Publisher, Subscriber and GatewayGeneric
    ///         - FindService, OfferService and StopOfferService
    ///       This means that iox::runtime::PoshRuntime::initRuntime(...) must be called before the above classes
    ///       are created or functions are called, to make the correct runtime active.
    static runtime::PoshRuntime& runtimeFactoryGetInstance(cxx::optional<const RuntimeName_t*> name);

  public:
    RuntimeTestInterface(RuntimeTestInterface&& rhs);
    RuntimeTestInterface& operator=(RuntimeTestInterface&& rhs);

    RuntimeTestInterface(const RuntimeTestInterface&) = delete;
    RuntimeTestInterface& operator=(const RuntimeTestInterface&) = delete;

  private:
    RuntimeTestInterface();
    ~RuntimeTestInterface();

    void cleanupRuntimes();

    void eraseRuntime(const RuntimeName_t& name);
};

} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_ENVIRONMENT_RUNTIME_TEST_INTERFACE_HPP
