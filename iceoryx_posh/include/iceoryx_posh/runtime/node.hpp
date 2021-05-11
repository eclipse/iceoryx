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
#ifndef IOX_POSH_RUNTIME_NODE_HPP
#define IOX_POSH_RUNTIME_NODE_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"

namespace iox
{
namespace runtime
{
class NodeData;

/// @brief class which represents a node
class Node
{
  public:
    /// @brief constructor which requires the name of the node
    /// @param[in] nodeName name of the node
    Node(const NodeName_t& nodeName) noexcept;

    /// @brief destructor
    ~Node() noexcept;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    /// @brief move constructor
    /// @param[in] rhs source object
    Node(Node&& rhs) noexcept;

    /// @brief move assignment operator
    /// @param[in] rhs source object, where to move from
    Node& operator=(Node&& rhs) noexcept;

    /// @brief returns the name of the node
    /// @return string which contains the node name
    NodeName_t getNodeName() const noexcept;

    /// @brief returns the name of the application's runtime
    /// @return string which contains the runtime name
    RuntimeName_t getRuntimeName() const noexcept;

  protected:
    Node(NodeData* const data) noexcept;

    NodeData* m_data = nullptr;
};
} // namespace runtime
} // namespace iox

#endif // IOX_POSH_RUNTIME_NODE_HPP
