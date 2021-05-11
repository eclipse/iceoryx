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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_LISTENER_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_LISTENER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace popo
{
/// @brief ConditionListener allows one to wait using a shared memory condition variable
class ConditionListener
{
  public:
    using NotificationVector_t = cxx::vector<cxx::BestFittingType_t<MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE>,
                                             MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE>;

    explicit ConditionListener(ConditionVariableData& condVarData) noexcept;
    ~ConditionListener() noexcept = default;
    ConditionListener(const ConditionListener& rhs) = delete;
    ConditionListener(ConditionListener&& rhs) noexcept = delete;
    ConditionListener& operator=(const ConditionListener& rhs) = delete;
    ConditionListener& operator=(ConditionListener&& rhs) noexcept = delete;

    /// @brief Was the ConditionListener notified by a ConditionNotifier?
    /// @return true if it was notified otherwise false
    bool wasNotified() const noexcept;

    /// @brief Used in classes to signal a thread which waits in wait() to return
    ///         and stop working. Destroy will send an empty notification to wait() and
    ///         after this call wait() turns into a non blocking call which always
    ///         returns an empty vector.
    void destroy() noexcept;

    /// @brief returns a sorted vector of indices of active notifications; blocking if ConditionVariableData was
    /// not notified unless destroy() was called before. The indices of active notifications are
    /// never empty unless destroy() was called, then it's always empty.
    ///
    /// @return a sorted vector of active notifications
    NotificationVector_t wait() noexcept;

    /// @brief returns a sorted vector of indices of active notifications; blocking for the specified time if
    /// ConditionVariableData was not notified unless destroy() was called before.
    /// The indices of active notifications can be empty (spurious wakeups). When destroy() was called then it
    /// is always empty.
    /// @param[in] timeToWait duration how long at most this method should wait
    ///
    /// @return a sorted vector of active notifications
    NotificationVector_t timedWait(const units::Duration& timeToWait) noexcept;

  protected:
    const ConditionVariableData* getMembers() const noexcept;
    ConditionVariableData* getMembers() noexcept;

  private:
    void reset(const uint64_t index) noexcept;
    void resetSemaphore() noexcept;

    NotificationVector_t waitImpl(const cxx::function_ref<bool()>& waitCall) noexcept;

  private:
    ConditionVariableData* m_condVarDataPtr{nullptr};
    std::atomic_bool m_toBeDestroyed{false};
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CONDITION_VARIABLE_WAITER_HPP
