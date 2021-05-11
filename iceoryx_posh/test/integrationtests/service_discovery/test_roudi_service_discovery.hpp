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

#ifndef IOX_POSH_SERVICE_DISCOVERY_TEST_ROUDI_SERVICE_DISCOVERY_HPP
#define IOX_POSH_SERVICE_DISCOVERY_TEST_ROUDI_SERVICE_DISCOVERY_HPP

#include "iceoryx_posh/testing/roudi_gtest.hpp"

using iox::capro::IdString_t;
using iox::runtime::InstanceContainer;

template <class T, uint64_t Capacity>
class vector_init_list : public iox::cxx::vector<T, Capacity>
{
  public:
    vector_init_list(std::initializer_list<T> l)
    {
        for (auto& i : l)
        {
            iox::cxx::vector<T, Capacity>::push_back(i);
        }
    }
};

class RouDiServiceDiscoveryTest : public RouDi_GTest
{
  protected:
    void InitContainer(InstanceContainer& dest, std::vector<std::string> src)
    {
        dest.clear();
        for (size_t i = 0; i < src.size(); i++)
        {
            dest.push_back(IdString_t(iox::cxx::TruncateToCapacity, src[i]));
        }
    }
};

#endif // IOX_POSH_SERVICE_DISCOVERY_TEST_ROUDI_SERVICE_DISCOVERY_HPP
