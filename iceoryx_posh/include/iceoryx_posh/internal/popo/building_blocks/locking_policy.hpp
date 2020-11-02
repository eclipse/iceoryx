// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP

#include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"

namespace iox
{
namespace popo
{
class ThreadSafePolicy
{
  public:
    // needs to be public since we want to use std::lock_guard
    void lock() const noexcept;
    void unlock() const noexcept;
    bool tryLock() const noexcept;

  private:
    mutable posix::mutex m_mutex{posix::mutex::Recursive::ON, posix::mutex::Robust::ON}; // recursive lock
};

class SingleThreadedPolicy
{
  public:
    // needs to be public since we want to use std::lock_guard
    void lock() const noexcept;
    void unlock() const noexcept;
    bool tryLock() const noexcept;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP
