// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_INL
#define IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_INL

#include "process_introspection.hpp"

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_utils/posix_wrapper/thread.hpp"

#include <chrono>

namespace iox
{
namespace roudi
{
template <typename PublisherPort>
inline ProcessIntrospection<PublisherPort>::ProcessIntrospection() noexcept
{
}

template <typename PublisherPort>
inline ProcessIntrospection<PublisherPort>::~ProcessIntrospection() noexcept
{
    stop();
    if (m_publisherPort.has_value())
    {
        m_publisherPort->stopOffer();
    }
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::addProcess(const int pid, const RuntimeName_t& name) noexcept
{
    ProcessIntrospectionData procIntrData;
    procIntrData.m_pid = pid;
    procIntrData.m_name = name;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_processList.push_back(procIntrData);
        m_processListNewData = true;
    }
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::removeProcess(const int pid) noexcept
{
    std::lock_guard<std::mutex> guard(m_mutex);

    for (auto it = m_processList.begin(); it != m_processList.end(); ++it)
    {
        if (it->m_pid == pid)
        {
            m_processList.erase(it);
            break;
        }
    }
    m_processListNewData = true;
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::addNode(const RuntimeName_t& runtimeName,
                                                         const NodeName_t& nodeName) noexcept
{
    std::lock_guard<std::mutex> guard(m_mutex);

    bool processFound = false;
    for (auto it_process = m_processList.begin(); it_process != m_processList.end(); ++it_process)
    {
        if (it_process->m_name == runtimeName)
        {
            processFound = true;
            bool alreadyInList = false;
            for (auto it_node = it_process->m_nodes.begin(); it_node != it_process->m_nodes.end(); ++it_node)
            {
                if (*it_node == nodeName)
                {
                    LogWarn() << "Node " << nodeName.c_str() << " already registered";
                    alreadyInList = true;
                }
            }
            if (!alreadyInList)
            {
                it_process->m_nodes.emplace_back(nodeName);
            }
        }
    }
    if (!processFound)
    {
        LogWarn() << "Trying to register node " << nodeName.c_str() << " but the related process is not registered";
    }
    m_processListNewData = true;
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::removeNode(const RuntimeName_t& runtimeName,
                                                            const NodeName_t& nodeName) noexcept
{
    std::lock_guard<std::mutex> guard(m_mutex);

    bool processFound = false;
    for (auto it_process = m_processList.begin(); it_process != m_processList.end(); ++it_process)
    {
        if (it_process->m_name == runtimeName)
        {
            processFound = true;
            bool removedFromList = false;
            for (auto it_node = it_process->m_nodes.begin(); it_node != it_process->m_nodes.end(); ++it_node)
            {
                if (*it_node == nodeName)
                {
                    it_process->m_nodes.erase(it_node);
                    removedFromList = true;
                    break;
                }
            }
            if (!removedFromList)
            {
                LogWarn() << "Trying to remove node " << nodeName.c_str() << " but it was not registered";
            }
        }
    }
    if (!processFound)
    {
        LogWarn() << "Trying to remove node " << nodeName.c_str() << " but the related process is not registered";
    }
    m_processListNewData = true;
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::registerPublisherPort(PublisherPort&& publisherPort) noexcept
{
    // we do not want to call this twice
    if (!m_publisherPort.has_value())
    {
        m_publisherPort.emplace(std::move(publisherPort));
    }
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::run() noexcept
{
    // TODO: error handling for non debug builds
    cxx::Expects(m_publisherPort.has_value());

    // this is a field, there needs to be a sample before activate is called
    send();
    m_publisherPort->offer();

    m_publishingTask.start(m_sendInterval);
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::send() noexcept
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_processListNewData)
    {
        auto maybeChunkHeader = m_publisherPort->tryAllocateChunk(sizeof(ProcessIntrospectionFieldTopic),
                                                                  alignof(ProcessIntrospectionFieldTopic),
                                                                  CHUNK_NO_USER_HEADER_SIZE,
                                                                  CHUNK_NO_USER_HEADER_ALIGNMENT);
        if (!maybeChunkHeader.has_error())
        {
            auto sample = static_cast<ProcessIntrospectionFieldTopic*>(maybeChunkHeader.value()->userPayload());
            new (sample) ProcessIntrospectionFieldTopic;

            for (auto& intrData : m_processList)
            {
                sample->m_processList.emplace_back(intrData);
            }
            m_processListNewData = false;

            m_publisherPort->sendChunk(maybeChunkHeader.value());
        }
    }
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::stop() noexcept
{
    m_publishingTask.stop();
}

template <typename PublisherPort>
inline void ProcessIntrospection<PublisherPort>::setSendInterval(const units::Duration interval) noexcept
{
    m_sendInterval = interval;
    if (m_publishingTask.isActive())
    {
        m_publishingTask.stop();
        m_publishingTask.start(m_sendInterval);
    }
}


} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_INTROSPECTION_PROCESS_INTROSPECTION_INL
