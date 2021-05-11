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
#ifndef IOX_POSH_ROUDI_PORT_MANAGER_HPP
#define IOX_POSH_ROUDI_PORT_MANAGER_HPP

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/popo/ports/application_port.hpp"
#include "iceoryx_posh/internal/popo/ports/interface_port.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_roudi.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_multi_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_single_producer.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/internal/roudi/introspection/port_introspection.hpp"
#include "iceoryx_posh/internal/roudi/service_registry.hpp"
#include "iceoryx_posh/internal/runtime/ipc_message.hpp"
#include "iceoryx_posh/internal/runtime/node_data.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/roudi/memory/roudi_memory_interface.hpp"
#include "iceoryx_posh/roudi/port_pool.hpp"
#include "iceoryx_posh/runtime/port_config_info.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/type_traits.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

#include <mutex>

namespace iox
{
namespace roudi
{
capro::Interfaces StringToCaProInterface(const capro::IdString_t& str) noexcept;

class PortManager
{
  public:
    using PortConfigInfo = iox::runtime::PortConfigInfo;
    PortManager(RouDiMemoryInterface* roudiMemoryInterface) noexcept;

    virtual ~PortManager() = default;

    /// @todo Remove this later
    void stopPortIntrospection() noexcept;

    void doDiscovery() noexcept;

    cxx::expected<PublisherPortRouDiType::MemberType_t*, PortPoolError>
    acquirePublisherPortData(const capro::ServiceDescription& service,
                             const popo::PublisherOptions& publisherOptions,
                             const RuntimeName_t& runtimeName,
                             mepoo::MemoryManager* const payloadDataSegmentMemoryManager,
                             const PortConfigInfo& portConfigInfo) noexcept;

    cxx::expected<SubscriberPortType::MemberType_t*, PortPoolError>
    acquireSubscriberPortData(const capro::ServiceDescription& service,
                              const popo::SubscriberOptions& subscriberOptions,
                              const RuntimeName_t& runtimeName,
                              const PortConfigInfo& portConfigInfo) noexcept;

    popo::InterfacePortData* acquireInterfacePortData(capro::Interfaces interface,
                                                      const RuntimeName_t& runtimeName,
                                                      const NodeName_t& nodeName = {""}) noexcept;

    popo::ApplicationPortData* acquireApplicationPortData(const RuntimeName_t& runtimeName) noexcept;

    cxx::expected<runtime::NodeData*, PortPoolError> acquireNodeData(const RuntimeName_t& runtimeName,
                                                                     const NodeName_t& nodeName) noexcept;

    cxx::expected<popo::ConditionVariableData*, PortPoolError>
    acquireConditionVariableData(const RuntimeName_t& runtimeName) noexcept;

    /// @brief Used to unblock potential locks in the shutdown phase of a process
    /// @param [in] name of the process runtime which is about to shut down
    void unblockProcessShutdown(const RuntimeName_t& runtimeName) noexcept;

    /// @brief Used to unblock potential locks in the shutdown phase of RouDi
    void unblockRouDiShutdown() noexcept;

    void deletePortsOfProcess(const RuntimeName_t& runtimeName) noexcept;

    const std::atomic<uint64_t>* serviceRegistryChangeCounter() noexcept;
    runtime::IpcMessage findService(const capro::ServiceDescription& service) noexcept;

  protected:
    void makeAllPublisherPortsToStopOffer() noexcept;

    void destroyPublisherPort(PublisherPortRouDiType::MemberType_t* const publisherPortData) noexcept;

    void destroySubscriberPort(SubscriberPortType::MemberType_t* const subscriberPortData) noexcept;

    void handlePublisherPorts() noexcept;

    void doDiscoveryForPublisherPort(PublisherPortRouDiType& publisherPort) noexcept;

    void handleSubscriberPorts() noexcept;

    void doDiscoveryForSubscriberPort(SubscriberPortType& subscriberPort) noexcept;

    void handleInterfaces() noexcept;

    void handleApplications() noexcept;

    void handleNodes() noexcept;

    void handleConditionVariables() noexcept;

    bool sendToAllMatchingPublisherPorts(const capro::CaproMessage& message,
                                         SubscriberPortType& subscriberSource) noexcept;

    void sendToAllMatchingSubscriberPorts(const capro::CaproMessage& message,
                                          PublisherPortRouDiType& publisherSource) noexcept;

    void sendToAllMatchingInterfacePorts(const capro::CaproMessage& message) noexcept;

    void addEntryToServiceRegistry(const capro::IdString_t& service, const capro::IdString_t& instance) noexcept;
    void removeEntryFromServiceRegistry(const capro::IdString_t& service, const capro::IdString_t& instance) noexcept;

    template <typename T, std::enable_if_t<std::is_same<T, iox::build::OneToManyPolicy>::value>* = nullptr>
    cxx::optional<RuntimeName_t>
    doesViolateCommunicationPolicy(const capro::ServiceDescription& service) const noexcept;

    template <typename T, std::enable_if_t<std::is_same<T, iox::build::ManyToManyPolicy>::value>* = nullptr>
    cxx::optional<RuntimeName_t>
    doesViolateCommunicationPolicy(const capro::ServiceDescription& service IOX_MAYBE_UNUSED) const noexcept;

  private:
    RouDiMemoryInterface* m_roudiMemoryInterface{nullptr};
    PortPool* m_portPool{nullptr};
    ServiceRegistry m_serviceRegistry;
    PortIntrospectionType m_portIntrospection;
};
} // namespace roudi
} // namespace iox

#include "iceoryx_posh/internal/roudi/port_manager.inl"

#endif // IOX_POSH_ROUDI_PORT_MANAGER_HPP
