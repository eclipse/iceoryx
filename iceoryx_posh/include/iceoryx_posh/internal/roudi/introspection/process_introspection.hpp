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
#ifndef IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_HPP
#define IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_utils/cxx/list.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/internal/concurrent/periodic_task.hpp"

#include <mutex>

namespace iox
{
namespace roudi
{
/// @brief This class handles the process intropection for RouDi.
///        It is recommended to use the ProcessIntrospectionType alias which sets
///        the intended template parameter.
///        The class tracks the adding and removal of processes and sends it to
///        the introspection client if subscribed.
template <typename PublisherPort>
class ProcessIntrospection
{
  public:
    ProcessIntrospection() noexcept;
    ~ProcessIntrospection() noexcept;

    // delete copy constructor and assignment operator
    ProcessIntrospection(ProcessIntrospection const&) = delete;
    ProcessIntrospection& operator=(ProcessIntrospection const&) = delete;
    // delete move constructor and assignment operator
    ProcessIntrospection(ProcessIntrospection&&) = delete;
    ProcessIntrospection& operator=(ProcessIntrospection&&) = delete;


    /// @brief This function is used to add a process to the process introspection
    /// @param[in] pid is the PID of the process to add
    /// @param[in] name is the name of the process
    void addProcess(const int pid, const RuntimeName_t& name) noexcept;

    /// @brief This function is used to remove the process from the process introspection
    /// @param[in] pid is the PID of the process to remove
    void removeProcess(const int pid) noexcept;

    /// @brief This function is used to add a node to the process introspection
    /// @param[in] runtimeName is the name of the proces
    /// @param[in] nodeName is the name of the node to add
    void addNode(const RuntimeName_t& runtimeName, const NodeName_t& node) noexcept;

    /// @brief This function is used to remove a node from the process introspection
    /// @param[in] runtimeName is the name of the proces
    /// @param[in] nodeName is the name of the node to remove
    void removeNode(const RuntimeName_t& runtimeName, const NodeName_t& node) noexcept;

    /// @brief This functions registers the POSH publisher port which is used
    ///        to send the data to the instrospcetion client
    /// @param publisherPort is the publisher port for transmission
    void registerPublisherPort(PublisherPort&& publisherPort) noexcept;

    /// @brief This function starts a thread which periodically sends
    ///        the introspection data to the client. The send interval
    ///        can be set by @ref setSendInterval "setSendInterval(...)".
    ///        Before this function is called, the publisher port hast to be
    ///        registered with @ref registerPublisherPort "registerPublisherPort()".
    void run() noexcept;

    /// @brief This function stops the thread previously started by @ref run "run()"
    void stop() noexcept;

    /// @brief This function configures the interval for the transmission of the
    ///        port introspection data.
    /// @param[in] interval duration between two send invocations.
    void setSendInterval(const units::Duration interval) noexcept;

  protected:
    cxx::optional<PublisherPort> m_publisherPort;
    void send() noexcept;

  private:
    using ProcessList_t = cxx::list<ProcessIntrospectionData, MAX_PROCESS_NUMBER>;
    ProcessList_t m_processList;
    bool m_processListNewData{true}; // true because we want to have a valid field, even with an empty list

    std::mutex m_mutex;

    units::Duration m_sendInterval{units::Duration::fromSeconds(1U)};
    concurrent::PeriodicTask<cxx::MethodCallback<void>> m_publishingTask{
        concurrent::PeriodicTaskManualStart, "ProcessIntr", *this, &ProcessIntrospection::send};
};

/// @brief typedef for the templated process introspection class that is used by RouDi for the
/// actual process introspection functionality.
using ProcessIntrospectionType = ProcessIntrospection<PublisherPortUserType>;

} // namespace roudi
} // namespace iox

#include "iceoryx_posh/internal/roudi/introspection/process_introspection.inl"

#endif // IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_HPP
