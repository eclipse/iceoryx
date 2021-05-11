// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/popo/building_blocks/condition_notifier.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"

namespace iox
{
namespace popo
{
ConditionNotifier::ConditionNotifier(ConditionVariableData& condVarDataRef, const uint64_t index) noexcept
    : m_condVarDataPtr(&condVarDataRef)
    , m_notificationIndex(index)
{
    if (index >= MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE)
    {
        LogFatal() << "The provided index " << index << " is too large. The index has to be in the range of [0, "
                   << MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE << "[.";
        errorHandler(Error::kPOPO__CONDITION_NOTIFIER_INDEX_TOO_LARGE, nullptr, ErrorLevel::FATAL);
    }
}

void ConditionNotifier::notify() noexcept
{
    if (m_notificationIndex < MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE)
    {
        getMembers()->m_activeNotifications[m_notificationIndex].store(true, std::memory_order_release);
    }
    getMembers()->m_semaphore.post().or_else([](auto) {
        errorHandler(Error::kPOPO__CONDITION_NOTIFIER_SEMAPHORE_CORRUPT_IN_NOTIFY, nullptr, ErrorLevel::FATAL);
    });
}

const ConditionVariableData* ConditionNotifier::getMembers() const noexcept
{
    return m_condVarDataPtr;
}

ConditionVariableData* ConditionNotifier::getMembers() noexcept
{
    return m_condVarDataPtr;
}

} // namespace popo
} // namespace iox
