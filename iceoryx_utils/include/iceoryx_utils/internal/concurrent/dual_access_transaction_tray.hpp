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

#ifndef IOX_UTILS_CONCURRENT_DUAL_ACCESS_TRANSACTION_TRAY_HPP
#define IOX_UTILS_CONCURRENT_DUAL_ACCESS_TRANSACTION_TRAY_HPP

#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

#include <atomic>

namespace iox
{
namespace concurrent
{
class DualAccessTransactionTray
{
  private:
    /// @note not public to prevent the user from passing AccessToken::NONE to a method
    enum class AccessToken
    {
        NONE,
        LEFT,
        RIGHT
    };

  public:
    static constexpr AccessToken LEFT{AccessToken::LEFT};
    static constexpr AccessToken RIGHT{AccessToken::RIGHT};

    class AccessGuard
    {
      public:
        AccessGuard(DualAccessTransactionTray& transactionTray, const AccessToken accessToken) noexcept;
        ~AccessGuard();

        AccessGuard(const AccessGuard&) = delete;
        AccessGuard(AccessGuard&&) = delete;

        AccessGuard& operator=(const AccessGuard&) = delete;
        AccessGuard& operator=(AccessGuard&&) = delete;

      private:
        DualAccessTransactionTray& m_transactionTray;
        AccessToken m_accessToken{AccessToken::NONE};
    };

    /// @brief Revokes the lock from an absent participant when LEFT or RIGHT terminated abnormally.
    /// @param[in] absentPaticipantToken is the DualAccessTransactionTray::LEFT or DualAccessTransactionTray::RIGHT
    /// token
    /// @attention this should only be called if the thread with `absentPaticipantToken` is not running anymore else the
    /// invariants are broken and you might observe pink elephants and dragons
    void revokeLockFromAbsentParticipant(const AccessToken absentPaticipantToken);

  private:
    void acquireExclusiveAccess(const AccessToken tokenToAcquireAccess);
    void releaseExclusiveAccess(const AccessToken tokenToBeReleased);

    std::atomic<AccessToken> m_accessToken{AccessToken::NONE};
    posix::Semaphore m_waitingLineLeft{posix::Semaphore::create(posix::CreateUnnamedSharedMemorySemaphore, 0U).value()};
    posix::Semaphore m_waitingLineRight{
        posix::Semaphore::create(posix::CreateUnnamedSharedMemorySemaphore, 0U).value()};
};

} // namespace concurrent
} // namespace iox

#endif // IOX_UTILS_CONCURRENT_DUAL_ACCESS_TRANSACTION_TRAY_HPP
