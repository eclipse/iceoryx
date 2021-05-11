// Copyright (c) 2020, 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"

namespace iox
{
namespace popo
{
TriggerHandle::TriggerHandle(ConditionVariableData& conditionVariableData,
                             const cxx::MethodCallback<void, uint64_t> resetCallback,
                             const uint64_t uniqueTriggerId) noexcept
    : m_conditionVariableDataPtr(&conditionVariableData)
    , m_resetCallback(resetCallback)
    , m_uniqueTriggerId(uniqueTriggerId)
{
    if (!m_resetCallback)
    {
        errorHandler(Error::kPOPO__TRIGGER_HANDLE_INVALID_RESET_CALLBACK, nullptr, ErrorLevel::FATAL);
    }
}

TriggerHandle::TriggerHandle(TriggerHandle&& rhs) noexcept
{
    *this = std::move(rhs);
}

TriggerHandle& TriggerHandle::operator=(TriggerHandle&& rhs) noexcept
{
    if (this != &rhs)
    {
        std::lock(m_mutex, rhs.m_mutex);
        std::lock_guard<std::recursive_mutex> lock(m_mutex, std::adopt_lock);
        std::lock_guard<std::recursive_mutex> lockRhs(rhs.m_mutex, std::adopt_lock);

        reset();

        m_conditionVariableDataPtr = rhs.m_conditionVariableDataPtr;
        m_resetCallback = std::move(rhs.m_resetCallback);
        m_uniqueTriggerId = rhs.m_uniqueTriggerId;

        rhs.invalidate();
    }

    return *this;
}

TriggerHandle::~TriggerHandle()
{
    reset();
}

TriggerHandle::operator bool() const noexcept
{
    return isValid();
}

bool TriggerHandle::isValid() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return m_conditionVariableDataPtr != nullptr;
}

void TriggerHandle::trigger() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (isValid())
    {
        ConditionNotifier(*m_conditionVariableDataPtr, m_uniqueTriggerId).notify();
    }
}

bool TriggerHandle::wasTriggered() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_conditionVariableDataPtr != nullptr)
    {
        return m_conditionVariableDataPtr->m_activeNotifications[m_uniqueTriggerId].load(std::memory_order_relaxed);
    }
    return false;
}

void TriggerHandle::reset() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!isValid())
    {
        return;
    }

    // constructor ensured that resetCallback is valid
    IOX_DISCARD_RESULT(m_resetCallback(m_uniqueTriggerId));

    invalidate();
}

void TriggerHandle::invalidate() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_conditionVariableDataPtr = nullptr;
    m_resetCallback = cxx::MethodCallback<void, uint64_t>();
    m_uniqueTriggerId = Trigger::INVALID_TRIGGER_ID;
}

ConditionVariableData* TriggerHandle::getConditionVariableData() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return m_conditionVariableDataPtr;
}

uint64_t TriggerHandle::getUniqueId() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return m_uniqueTriggerId;
}
} // namespace popo
} // namespace iox
