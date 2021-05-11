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

#include "iceoryx_posh/runtime/node.hpp"

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/runtime/node_data.hpp"
#include "iceoryx_posh/internal/runtime/node_property.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace iox
{
namespace runtime
{
Node::Node(NodeData* const data) noexcept
    : m_data(data)
{
}

Node::Node(const NodeName_t& nodeName) noexcept
    : Node(PoshRuntime::getInstance().createNode(NodeProperty(nodeName, 0U)))
{
}

Node::~Node() noexcept
{
    if (m_data)
    {
        m_data->m_toBeDestroyed.store(true, std::memory_order_relaxed);
    }
}

Node::Node(Node&& rhs) noexcept
{
    *this = std::move(rhs);
}

Node& Node::operator=(Node&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_data = rhs.m_data;
        rhs.m_data = nullptr;
    }

    return *this;
}

NodeName_t Node::getNodeName() const noexcept
{
    return m_data->m_nodeName;
}

RuntimeName_t Node::getRuntimeName() const noexcept
{
    return m_data->m_runtimeName;
}

} // namespace runtime
} // namespace iox
