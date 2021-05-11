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

#ifndef IOX_POSH_POPO_BASE_PUBLISHER_HPP
#define IOX_POSH_POPO_BASE_PUBLISHER_HPP

#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"
#include "iceoryx_posh/internal/popo/sample_deleter.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

namespace iox
{
namespace popo
{
using uid_t = UniquePortId;


///
/// @brief The BasePublisher class contains the common implementation for the different publisher specializations.
///
template <typename port_t = iox::PublisherPortUserType>
class BasePublisher
{
  public:
    using PortType = port_t;

    BasePublisher(const BasePublisher& other) = delete;
    BasePublisher& operator=(const BasePublisher&) = delete;
    BasePublisher(BasePublisher&& rhs) = delete;
    BasePublisher& operator=(BasePublisher&& rhs) = delete;
    virtual ~BasePublisher();

    ///
    /// @brief uid Get the UID of the publisher.
    /// @return The publisher's UID.
    ///
    uid_t getUid() const noexcept;

    ///
    /// @brief getServiceDescription Get the service description of the publisher.
    /// @return The service description.
    ///
    capro::ServiceDescription getServiceDescription() const noexcept;

    ///
    /// @brief offer Offer the service to be subscribed to.
    ///
    void offer() noexcept;

    ///
    /// @brief stopOffer Stop offering the service.
    ///
    void stopOffer() noexcept;

    ///
    /// @brief isOffered
    /// @return True if service is currently being offered.
    ///
    bool isOffered() const noexcept;

    ///
    /// @brief hasSubscribers
    /// @return True if currently has subscribers to the service.
    ///
    bool hasSubscribers() const noexcept;

  protected:
    BasePublisher() = default; // Required for testing.
    BasePublisher(const capro::ServiceDescription& service, const PublisherOptions& publisherOptions);

    ///
    /// @brief port
    /// @return const accessor of the underlying port
    ///
    const port_t& port() const noexcept;

    ///
    /// @brief port
    /// @return accessor of the underlying port
    ///
    port_t& port() noexcept;

    port_t m_port{nullptr};
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/base_publisher.inl"

#endif // IOX_POSH_POPO_BASE_PUBLISHER_HPP
