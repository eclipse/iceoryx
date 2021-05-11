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
#ifndef IOX_UTILS_CONCURRENT_ACTIVE_OBJECT_HPP
#define IOX_UTILS_CONCURRENT_ACTIVE_OBJECT_HPP

#include <functional>
#include <thread>

#include "iceoryx_utils/internal/concurrent/fifo.hpp"
#include "iceoryx_utils/internal/concurrent/trigger_queue.hpp"

namespace iox
{
namespace concurrent
{
class ActiveObject
{
  protected:
    ActiveObject() noexcept;
    virtual ~ActiveObject() noexcept;
    void addTask(const std::function<void()> f) noexcept;
    void mainLoop() noexcept;
    void stopRunning() noexcept;

    friend class cxx::optional<ActiveObject>;

  private:
    static constexpr uint32_t taskQueueSize = 128;
    using taskQueue_t = concurrent::TriggerQueue<std::function<void()>, taskQueueSize, concurrent::FiFo>;

    taskQueue_t m_tasks;

    bool m_keepRunning{true};
    std::thread m_mainLoopThread;
};
} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_ACTIVE_OBJECT_HPP
