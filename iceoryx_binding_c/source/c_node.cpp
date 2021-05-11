// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/runtime/node_data.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_posh/runtime/node.hpp"

using namespace iox;
using namespace iox::runtime;

extern "C" {
#include "iceoryx_binding_c/node.h"
}

class NodeBindingExtension : public iox::runtime::Node
{
  public:
    NodeBindingExtension(NodeData* const data)
        : Node(data)
    {
    }

    ~NodeBindingExtension()
    {
        m_data = nullptr;
    }

    void destroy()
    {
        m_data->m_toBeDestroyed.store(true, std::memory_order_relaxed);
    }
};

iox_node_t iox_node_create(const char* const nodeName)
{
    return PoshRuntime::getInstance().createNode(NodeProperty(NodeName_t(iox::cxx::TruncateToCapacity, nodeName), 0u));
}

void iox_node_destroy(iox_node_t const self)
{
    NodeBindingExtension(self).destroy();
}

uint64_t iox_node_get_name(iox_node_t const self, char* const name, const uint64_t nameCapacity)
{
    if (name == nullptr)
    {
        return 0U;
    }

    auto nameAsString = NodeBindingExtension(self).getNodeName();
    strncpy(name, nameAsString.c_str(), nameCapacity);
    name[nameCapacity - 1U] = '\0'; // strncpy doesn't add a null-termination if destination is smaller than source

    return nameAsString.size();
}

uint64_t iox_node_get_runtime_name(iox_node_t const self, char* const name, const uint64_t nameCapacity)
{
    if (name == nullptr)
    {
        return 0U;
    }

    auto nameAsString = NodeBindingExtension(self).getRuntimeName();
    strncpy(name, nameAsString.c_str(), nameCapacity);
    name[nameCapacity - 1U] = '\0'; // strncpy doesn't add a null-termination if destination is smaller than source

    return nameAsString.size();
}
