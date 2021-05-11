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
#ifndef IOX_POSH_ROUDI_INTROSPECTION_MEMPOOL_INTROSPECTION_HPP
#define IOX_POSH_ROUDI_INTROSPECTION_MEMPOOL_INTROSPECTION_HPP

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/roudi/port_manager.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/internal/concurrent/periodic_task.hpp"

#include <cstdint>

namespace iox
{
namespace roudi
{
/// @brief This class handles the mempool intropection for RouDi.
///        It is recommended to use the MemPoolIntrospectionType alias which sets
///        the intended template parameters required for the actual introspection.
///        The class sends snapshots of the mempool usage to the introspection
///        client if subscribed.
template <typename MemoryManager, typename SegmentManager, typename PublisherPort>
class MemPoolIntrospection
{
  public:
    /// @brief The constructor for the MemPoolIntrospection.
    ///        It starts a thread and set it into a wait condition.
    /// @param[in] rouDiInternalMemoryManager is the internal RouDi memory manager
    /// @param[in] segmentManager contains the shared memory segments and their memory pools which will be intropected
    /// @param[in] publisherPort is the publisher port for transmission of the introspection data
    MemPoolIntrospection(MemoryManager& rouDiInternalMemoryManager,
                         SegmentManager& segmentManager,
                         PublisherPort&& publisherPort);

    ~MemPoolIntrospection();

    // delete copy constructor and assignment operator
    MemPoolIntrospection(MemPoolIntrospection const&) = delete;
    MemPoolIntrospection& operator=(MemPoolIntrospection const&) = delete;
    // delete move constructor and assignment operator
    MemPoolIntrospection(MemPoolIntrospection&&) = delete;
    MemPoolIntrospection& operator=(MemPoolIntrospection&&) = delete;

    /// @brief This function starts the periodic transmission of snapshots of the mempool introspecton data.
    ///        The send interval can be set by @ref setSendInterval "setSendInterval(...)". By default it's 1 second.
    void run() noexcept;

    /// @brief This function stops the thread which sends the introspection data.
    ///        It is not possible to start the thread again.
    void stop() noexcept;

    /// @brief This function configures the interval for the transmission of the
    ///        mempool introspection data.
    /// @param[in] interval duration between two send invocations
    void setSendInterval(const units::Duration interval) noexcept;

  protected:
    MemoryManager* m_rouDiInternalMemoryManager{nullptr}; // mempool handler needs to outlive this class (!)
    SegmentManager* m_segmentManager{nullptr};
    PublisherPort m_publisherPort{nullptr};
    void send() noexcept;

  private:
    static void prepareIntrospectionSample(MemPoolIntrospectionInfo& sample,
                                           const posix::PosixGroup& readerGroup,
                                           const posix::PosixGroup& writerGroup,
                                           uint32_t id) noexcept;

    /// @brief copy data fro internal struct into interface struct
    void copyMemPoolInfo(const MemoryManager& memoryManager, MemPoolInfoContainer& dest) noexcept;

  private:
    units::Duration m_sendInterval{units::Duration::fromSeconds(1U)};
    concurrent::PeriodicTask<cxx::MethodCallback<void>> m_publishingTask{
        concurrent::PeriodicTaskManualStart, "MemPoolIntr", *this, &MemPoolIntrospection::send};
};

/// @brief typedef for the templated mempool introspection class that is used by RouDi for the
/// actual mempool introspection functionality.
using MemPoolIntrospectionType =
    MemPoolIntrospection<mepoo::MemoryManager, mepoo::SegmentManager<>, PublisherPortUserType>;

} // namespace roudi
} // namespace iox
#include "mempool_introspection.inl"

#endif // IOX_POSH_ROUDI_INTROSPECTION_MEMPOOL_INTROSPECTION_HPP
