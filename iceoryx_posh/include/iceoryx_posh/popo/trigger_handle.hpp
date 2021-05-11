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

#ifndef IOX_POSH_POPO_TRIGGER_HANDLE_HPP
#define IOX_POSH_POPO_TRIGGER_HANDLE_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/trigger.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"

#include <limits>
#include <mutex>

namespace iox
{
namespace popo
{
/// @brief TriggerHandle is threadsafe without restrictions in a single process.
///        Not qualified for inter process usage. The TriggerHandle is generated
///        by a Notifyable like the WaitSet and handed out to the user when they
///        acquire a trigger. The TriggerHandle corresponds with an internal Trigger
///        and is used to signal an event via the trigger method. When it goes
///        out of scope it cleans up the corresponding trigger in the Notifyable.
class TriggerHandle
{
  public:
    TriggerHandle() = default;

    /// @brief Creates a TriggerHandle
    /// @param[in] conditionVariableDataRef reference to a condition variable data struct
    /// @param[in] resetCallback callback which will be called it goes out of scope or reset is called
    /// @param[in] uniqueTriggerId the unique trigger id of the Trigger which corresponds to the TriggerHandle. Usually
    /// stored in a Notifyable. It is required for the resetCallback
    TriggerHandle(ConditionVariableData& conditionVariableData,
                  const cxx::MethodCallback<void, uint64_t> resetCallback,
                  const uint64_t uniqueTriggerId) noexcept;
    TriggerHandle(const TriggerHandle&) = delete;
    TriggerHandle& operator=(const TriggerHandle&) = delete;

    TriggerHandle(TriggerHandle&& rhs) noexcept;
    TriggerHandle& operator=(TriggerHandle&& rhs) noexcept;
    ~TriggerHandle();

    /// @brief returns true if the TriggerHandle is valid otherwise false. A TriggerHandle is valid if
    /// m_conditionVariableDataPtr != nullptr.
    explicit operator bool() const noexcept;

    /// @brief returns true if the TriggerHandle is valid otherwise false. A TriggerHandle is valid if
    /// m_conditionVariableDataPtr != nullptr.
    bool isValid() const noexcept;

    /// @brief Returns true when the TriggerHandle was triggered.
    /// @note The TriggerHandle wasTriggered state is set to false again after the underlying ConditionListener gathered
    /// all events.
    bool wasTriggered() const noexcept;

    /// @brief triggers the Trigger and informs the Notifyable which verifies that the Trigger was triggered by calling
    /// the hasTriggeredCallback
    void trigger() noexcept;

    /// @brief calls the resetCallback and invalidates the TriggerHandle
    void reset() noexcept;

    /// @brief invalidates the TriggerHandle without calling the reset callback
    void invalidate() noexcept;

    /// @brief returns the uniqueTriggerId
    uint64_t getUniqueId() const noexcept;

    /// @brief returns the pointer to the ConditionVariableData
    ConditionVariableData* getConditionVariableData() noexcept;

  private:
    ConditionVariableData* m_conditionVariableDataPtr = nullptr;
    cxx::MethodCallback<void, uint64_t> m_resetCallback;
    uint64_t m_uniqueTriggerId = Trigger::INVALID_TRIGGER_ID;
    mutable std::recursive_mutex m_mutex;
};
} // namespace popo
} // namespace iox

#endif
