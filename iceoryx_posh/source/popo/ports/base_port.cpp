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

#include "iceoryx_posh/internal/popo/ports/base_port.hpp"

namespace iox
{
namespace popo
{
BasePort::BasePort(MemberType_t* const basePortDataPtr) noexcept
    : m_basePortDataPtr(basePortDataPtr)
{
}

BasePort::BasePort(BasePort&& rhs) noexcept
{
    *this = std::move(rhs);
}

BasePort& BasePort::operator=(BasePort&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_basePortDataPtr = rhs.m_basePortDataPtr;
        rhs.m_basePortDataPtr = nullptr;
    }
    return *this;
}

capro::ServiceDescription BasePort::getCaProServiceDescription() const noexcept
{
    return getMembers()->m_serviceDescription;
}

RuntimeName_t BasePort::getRuntimeName() const noexcept
{
    return getMembers()->m_runtimeName;
}

UniquePortId BasePort::getUniqueID() const noexcept
{
    return getMembers()->m_uniqueId;
}

NodeName_t BasePort::getNodeName() const noexcept
{
    return getMembers()->m_nodeName;
}

BasePort::operator bool() const noexcept
{
    return m_basePortDataPtr != nullptr;
}

void BasePort::destroy() noexcept
{
    getMembers()->m_toBeDestroyed.store(true, std::memory_order_relaxed);
}

bool BasePort::toBeDestroyed() const noexcept
{
    return getMembers()->m_toBeDestroyed.load(std::memory_order_relaxed);
}

} // namespace popo
} // namespace iox
