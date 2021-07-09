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
#ifndef IOX_POSH_RUNTIME_POSH_RUNTIME_HPP
#define IOX_POSH_RUNTIME_POSH_RUNTIME_HPP

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/internal/popo/ports/application_port.hpp"
#include "iceoryx_posh/internal/popo/ports/interface_port.hpp"
#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_user.hpp"
#include "iceoryx_posh/internal/runtime/ipc_runtime_interface.hpp"
#include "iceoryx_posh/internal/runtime/node_property.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_posh/runtime/port_config_info.hpp"

#include <atomic>

namespace iox
{
namespace roudi
{
class RuntimeTestInterface;
} // namespace roudi

namespace runtime
{
class Node;
class NodeData;

enum class FindServiceError
{
    INVALID_STATE,
    UNABLE_TO_WRITE_TO_ROUDI_CHANNEL, /// @todo #415 remove as IPC channel won't be used
    INSTANCE_CONTAINER_OVERFLOW /// @todo #415 set container to iox::MAX_NUMBER_OF_SERVICES and remove error
};

/// @brief Used to search for any string
struct Wildcard_t
{
};

/// @brief The runtime that is needed for each application to communicate with the RouDi daemon
class PoshRuntime
{
  public:
    PoshRuntime(const PoshRuntime&) = delete;
    PoshRuntime& operator=(const PoshRuntime&) = delete;
    PoshRuntime(PoshRuntime&&) = delete;
    PoshRuntime& operator=(PoshRuntime&&) = delete;
    virtual ~PoshRuntime() noexcept = default;

    /// @brief returns active runtime
    ///
    /// @return active runtime
    static PoshRuntime& getInstance() noexcept;

    /// @brief creates the runtime with given name
    ///
    /// @param[in] name used for registering the process with the RouDi daemon
    ///
    /// @return active runtime
    static PoshRuntime& initRuntime(const RuntimeName_t& name) noexcept;

    /// @brief get the name that was used to register with RouDi
    /// @return name of the registered application
    RuntimeName_t getInstanceName() const noexcept;

    /// @brief initiates the shutdown of the runtime to unblock all potentially blocking publisher
    /// with the SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER option set
    void shutdown() noexcept;

    /// @brief find all services that match the provided service description
    /// @param[in] service service string to search for (wildcards allowed)
    /// @param[in] instance instance string to search for (wildcards allowed)
    /// @return cxx::expected<ServiceContainer, FindServiceError>
    /// ServiceContainer: on success, container that is filled with all matching instances
    /// FindServiceError: if any, encountered during the operation
    virtual cxx::expected<ServiceContainer, FindServiceError>
    findService(const cxx::variant<Wildcard_t, capro::IdString_t> service,
                const cxx::variant<Wildcard_t, capro::IdString_t> instance) noexcept = 0;

    /// @brief offer the provided service, sends the offer from application to RouDi daemon
    /// @param[in] service valid ServiceDescription to offer
    /// @return bool, if service is offered returns true else false
    virtual bool offerService(const capro::ServiceDescription& serviceDescription) noexcept = 0;

    /// @brief stop offering the provided service
    /// @param[in] service valid ServiceDescription that shall be no more offered
    /// @return bool, if service is not offered anymore returns true else false
    virtual bool stopOfferService(const capro::ServiceDescription& serviceDescription) noexcept = 0;

    /// @brief request the RouDi daemon to create a publisher port
    /// @param[in] serviceDescription service description for the new publisher port
    /// @param[in] publisherOptions like the history capacity of a publisher
    /// @param[in] portConfigInfo configuration information for the port
    /// (i.e. what type of port is requested, device where its payload memory is located on etc.)
    /// @return pointer to a created publisher port user
    virtual PublisherPortUserType::MemberType_t*
    getMiddlewarePublisher(const capro::ServiceDescription& service,
                           const popo::PublisherOptions& publisherOptions = {},
                           const PortConfigInfo& portConfigInfo = {}) noexcept = 0;

    /// @brief request the RouDi daemon to create a subscriber port
    /// @param[in] serviceDescription service description for the new subscriber port
    /// @param[in] subscriberOptions like the queue capacity and history requested by a subscriber
    /// @param[in] portConfigInfo configuration information for the port
    /// (what type of port is requested, device where its payload memory is located on etc.)
    /// @return pointer to a created subscriber port data
    virtual SubscriberPortUserType::MemberType_t*
    getMiddlewareSubscriber(const capro::ServiceDescription& service,
                            const popo::SubscriberOptions& subscriberOptions = {},
                            const PortConfigInfo& portConfigInfo = {}) noexcept = 0;

    /// @brief request the RouDi daemon to create an interface port
    /// @param[in] interface interface to create
    /// @param[in] nodeName name of the node where the interface should belong to
    /// @return pointer to a created interface port data
    virtual popo::InterfacePortData* getMiddlewareInterface(const capro::Interfaces interface,
                                                            const NodeName_t& nodeName = {}) noexcept = 0;

    /// @brief request the RouDi daemon to create an application port
    /// @return pointer to a created application port data
    virtual popo::ApplicationPortData* getMiddlewareApplication() noexcept = 0;

    /// @brief request the RouDi daemon to create a condition variable
    /// @return pointer to a created condition variable data
    virtual popo::ConditionVariableData* getMiddlewareConditionVariable() noexcept = 0;

    /// @brief request the RouDi daemon to create a node
    /// @param[in] nodeProperty class which contains all properties which the node should have
    /// @return pointer to the data of the node
    virtual NodeData* createNode(const NodeProperty& nodeProperty) noexcept = 0;

    /// @brief requests the serviceRegistryChangeCounter from the shared memory
    /// @return pointer to the serviceRegistryChangeCounter
    virtual const std::atomic<uint64_t>* getServiceRegistryChangeCounter() noexcept = 0;

    /// @brief send a request to the RouDi daemon and get the response
    ///        currently each request is followed by a response
    /// @param[in] msg request message to send
    /// @param[out] response from the RouDi daemon
    /// @return true if sucessful request/response, false on error
    virtual bool sendRequestToRouDi(const IpcMessage& msg, IpcMessage& answer) noexcept = 0;

  protected:
    friend class roudi::RuntimeTestInterface;
    using factory_t = PoshRuntime& (*)(cxx::optional<const RuntimeName_t*>);

    // Protected constructor for derived classes
    PoshRuntime(cxx::optional<const RuntimeName_t*> name) noexcept;

    static PoshRuntime& defaultRuntimeFactory(cxx::optional<const RuntimeName_t*> name) noexcept;

    /// @brief gets current runtime factory. If the runtime factory is not yet initialized it is set to
    /// defaultRuntimeFactory.
    ///
    /// @return current runtime factory
    static factory_t& getRuntimeFactory() noexcept;

    /// @brief sets runtime factory, terminates if given factory is empty
    ///
    /// @param[in] factory std::function to which the runtime factory should be set
    static void setRuntimeFactory(const factory_t& factory) noexcept;

    /// @brief creates the runtime or returns the already existing one -> Singleton
    ///
    /// @param[in] name optional containing the name used for registering with the RouDi daemon
    ///
    /// @return active runtime
    static PoshRuntime& getInstance(cxx::optional<const RuntimeName_t*> name) noexcept;

    /// @brief checks the given application name for certain constraints like length or if is empty
    const RuntimeName_t& verifyInstanceName(cxx::optional<const RuntimeName_t*> name) noexcept;

    const RuntimeName_t m_appName;
    std::atomic<bool> m_shutdownRequested{false};
};

} // namespace runtime
} // namespace iox

#endif // IOX_POSH_RUNTIME_POSH_RUNTIME_HPP
