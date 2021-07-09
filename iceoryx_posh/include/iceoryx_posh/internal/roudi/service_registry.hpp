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
#ifndef IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
#define IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"
#include "iceoryx_hoofs/internal/cxx/set.hpp"
#include "iceoryx_posh/capro/service_description.hpp"

#include <cstdint>
#include <map>
#include <utility>

namespace iox
{
namespace roudi
{
static const capro::IdString_t Wildcard{"*"};
class ServiceRegistry
{
  public:
    enum class Error
    {
        INVALID_STATE,
        SERVICE_DESCRIPTION_ALREADY_ADDED,
        SERVICE_REGISTRY_FULL,
        SERVICE_DESCRIPTION_INVALID,
    };
    /// @todo #415 should be connected with iox::MAX_NUMBER_OF_SERVICES
    static constexpr uint32_t MAX_SERVICE_DESCRIPTIONS = 100U;
    using ServiceDescriptionVector_t = cxx::vector<capro::ServiceDescription, MAX_SERVICE_DESCRIPTIONS>;

    /// @brief Adds given service description to registry
    /// @param[in] serviceDescription, service to be added
    /// @return ServiceRegistryError, error wrapped in cxx::expected
    cxx::expected<Error> add(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry
    /// @param[in] serviceDescription, service to be removed
    /// @return true, if service description was removed, false otherwise
    bool remove(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry
    /// @param[in] searchResult, reference to the vector which will be filled with the results
    /// @param[in] service, string or wildcard to search for
    /// @param[in] service, string or wildcard to search for
    void find(ServiceDescriptionVector_t& searchResult,
              const capro::IdString_t& service = Wildcard,
              const capro::IdString_t& instinstanceance = Wildcard) const noexcept;

    /// @brief Returns all service descriptions as copy
    /// @return ServiceDescriptionVector_t, copy of complete service registry
    const ServiceDescriptionVector_t getServices() const noexcept;

  private:
    /// @todo #859 replace std::multimap with prefix tree
    ::std::multimap<capro::IdString_t, uint64_t> m_serviceMap;
    ::std::multimap<capro::IdString_t, uint64_t> m_instanceMap;
    ServiceDescriptionVector_t m_serviceDescriptionVector;
};
} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
