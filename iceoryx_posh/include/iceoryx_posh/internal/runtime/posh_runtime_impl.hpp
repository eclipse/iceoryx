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
#ifndef IOX_POSH_RUNTIME_POSH_RUNTIME_IMPL_HPP
#define IOX_POSH_RUNTIME_POSH_RUNTIME_IMPL_HPP

#include "iceoryx_hoofs/cxx/method_callback.hpp"
#include "iceoryx_hoofs/internal/concurrent/periodic_task.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_posh/internal/runtime/shared_memory_user.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace iox
{
namespace runtime
{
enum class RuntimeLocation
{
    SEPARATE_PROCESS_FROM_ROUDI,
    SAME_PROCESS_LIKE_ROUDI,
};

/// @brief The runtime that is needed for each application to communicate with the RouDi daemon
class PoshRuntimeImpl : public PoshRuntime
{
  public:
    PoshRuntimeImpl(const PoshRuntimeImpl&) = delete;
    PoshRuntimeImpl& operator=(const PoshRuntimeImpl&) = delete;
    PoshRuntimeImpl(PoshRuntimeImpl&&) = delete;
    PoshRuntimeImpl& operator=(PoshRuntimeImpl&&) = delete;
    virtual ~PoshRuntimeImpl() noexcept;

    /// @copydoc PoshRuntime::findService
    cxx::expected<ServiceContainer, FindServiceError>
    findService(const cxx::variant<Wildcard_t, capro::IdString_t> service,
                const cxx::variant<Wildcard_t, capro::IdString_t> instance) noexcept override;

    /// @copydoc PoshRuntime::offerService
    bool offerService(const capro::ServiceDescription& serviceDescription) noexcept override;

    /// @copydoc PoshRuntime::stopOfferService
    bool stopOfferService(const capro::ServiceDescription& serviceDescription) noexcept override;

    /// @copydoc PoshRuntime::getMiddlewarePublisher
    PublisherPortUserType::MemberType_t*
    getMiddlewarePublisher(const capro::ServiceDescription& service,
                           const popo::PublisherOptions& publisherOptions = popo::PublisherOptions(),
                           const PortConfigInfo& portConfigInfo = PortConfigInfo()) noexcept override;

    /// @copydoc PoshRuntime::getMiddlewareSubscriber
    SubscriberPortUserType::MemberType_t*
    getMiddlewareSubscriber(const capro::ServiceDescription& service,
                            const popo::SubscriberOptions& subscriberOptions = popo::SubscriberOptions(),
                            const PortConfigInfo& portConfigInfo = PortConfigInfo()) noexcept override;

    /// @copydoc PoshRuntime::getMiddlewareInterface
    popo::InterfacePortData* getMiddlewareInterface(const capro::Interfaces interface,
                                                    const NodeName_t& nodeName = {""}) noexcept override;

    /// @copydoc PoshRuntime::getMiddlewareApplication
    popo::ApplicationPortData* getMiddlewareApplication() noexcept override;

    /// @copydoc PoshRuntime::getMiddlewareConditionVariable
    popo::ConditionVariableData* getMiddlewareConditionVariable() noexcept override;

    /// @copydoc PoshRuntime::createNode
    NodeData* createNode(const NodeProperty& nodeProperty) noexcept override;

    /// @copydoc PoshRuntime::getServiceRegistryChangeCounter
    const std::atomic<uint64_t>* getServiceRegistryChangeCounter() noexcept override;

    /// @copydoc PoshRuntime::sendRequestToRouDi
    bool sendRequestToRouDi(const IpcMessage& msg, IpcMessage& answer) noexcept override;

  protected:
    friend class PoshRuntime;
    friend class roudi::RuntimeTestInterface;

    // Protected constructor for IPC setup
    PoshRuntimeImpl(cxx::optional<const RuntimeName_t*> name,
                    const RuntimeLocation location = RuntimeLocation::SEPARATE_PROCESS_FROM_ROUDI) noexcept;

  private:
    cxx::expected<PublisherPortUserType::MemberType_t*, IpcMessageErrorType>
    requestPublisherFromRoudi(const IpcMessage& sendBuffer) noexcept;

    cxx::expected<SubscriberPortUserType::MemberType_t*, IpcMessageErrorType>
    requestSubscriberFromRoudi(const IpcMessage& sendBuffer) noexcept;

    cxx::expected<popo::ConditionVariableData*, IpcMessageErrorType>
    requestConditionVariableFromRoudi(const IpcMessage& sendBuffer) noexcept;

    mutable posix::mutex m_appIpcRequestMutex{false};

    IpcRuntimeInterface m_ipcChannelInterface;
    cxx::optional<SharedMemoryUser> m_ShmInterface;
    popo::ApplicationPort m_applicationPort;

    void sendKeepAliveAndHandleShutdownPreparation() noexcept;
    static_assert(PROCESS_KEEP_ALIVE_INTERVAL > roudi::DISCOVERY_INTERVAL, "Keep alive interval too small");

    // the m_keepAliveTask should always be the last member, so that it will be the first member to be destroyed
    concurrent::PeriodicTask<cxx::MethodCallback<void>> m_keepAliveTask{
        concurrent::PeriodicTaskAutoStart,
        PROCESS_KEEP_ALIVE_INTERVAL,
        "KeepAlive",
        *this,
        &PoshRuntimeImpl::sendKeepAliveAndHandleShutdownPreparation};
};

} // namespace runtime
} // namespace iox

#endif // IOX_POSH_RUNTIME_POSH_RUNTIME_IMPL_HPP
