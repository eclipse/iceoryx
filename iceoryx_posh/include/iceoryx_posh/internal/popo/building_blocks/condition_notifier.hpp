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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_NOTIFIER_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_NOTIFIER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace popo
{
/// @brief ConditionNotifier can notifiy waiting threads and processes using a shared memory condition variable
class ConditionNotifier
{
  public:
    static constexpr uint64_t INVALID_NOTIFICATION_INDEX = std::numeric_limits<uint64_t>::max();

    ConditionNotifier(ConditionVariableData& condVarDataRef, const uint64_t index) noexcept;

    ConditionNotifier(const ConditionNotifier& rhs) = delete;
    ConditionNotifier(ConditionNotifier&& rhs) noexcept = delete;
    ConditionNotifier& operator=(const ConditionNotifier& rhs) = delete;
    ConditionNotifier& operator=(ConditionNotifier&& rhs) noexcept = delete;
    ~ConditionNotifier() noexcept = default;

    /// @brief If threads are waiting on the condition variable, this call unblocks one of the waiting threads
    void notify() noexcept;

  protected:
    const ConditionVariableData* getMembers() const noexcept;
    ConditionVariableData* getMembers() noexcept;

  private:
    ConditionVariableData* m_condVarDataPtr{nullptr};
    uint64_t m_notificationIndex = INVALID_NOTIFICATION_INDEX;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_VARIABLE_SIGNALER_HPP
