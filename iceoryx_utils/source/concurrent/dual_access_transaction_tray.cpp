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

#include "iceoryx_utils/internal/concurrent/dual_access_transaction_tray.hpp"

namespace iox
{
namespace concurrent
{
DualAccessTransactionTray::AccessGuard::AccessGuard(DualAccessTransactionTray& transactionTray,
                                                    const AccessToken accessToken) noexcept
    : m_transactionTray(transactionTray)
    , m_accessToken(accessToken)
{
    transactionTray.acquireExclusiveAccess(m_accessToken);
}

DualAccessTransactionTray::AccessGuard::~AccessGuard()
{
    m_transactionTray.releaseExclusiveAccess(m_accessToken);
}

void DualAccessTransactionTray::revokeLockFromAbsentParticipant(const AccessToken absentPaticipantToken)
{
    releaseExclusiveAccess(absentPaticipantToken);
}

void DualAccessTransactionTray::acquireExclusiveAccess(const AccessToken tokenToAcquireAccess)
{
    auto existingToken = m_accessToken.exchange(tokenToAcquireAccess, std::memory_order_relaxed);
    if (existingToken == tokenToAcquireAccess)
    {
        // TODO return expected DOUBLE_ACQUIRE_BROKEN_INVARIANT
        std::terminate();
    }
    else if (existingToken != AccessToken::NONE)
    {
        if (tokenToAcquireAccess == AccessToken::LEFT)
        {
            if (m_waitingLineLeft.wait().has_error())
            {
                // TODO return expected ERROR_WHILE_WAITING_FOR_SEMAPHORE
            }
        }
        else
        {
            if (m_waitingLineRight.wait().has_error())
            {
                // TODO return expected ERROR_WHILE_WAITING_FOR_SEMAPHORE
            }
        }
    }
}

void DualAccessTransactionTray::releaseExclusiveAccess(const AccessToken tokenToBeReleased)
{
    AccessToken expected = tokenToBeReleased;
    auto casSuccessful = m_accessToken.compare_exchange_strong(expected, AccessToken::NONE, std::memory_order_relaxed);
    if (!casSuccessful)
    {
        if (expected == AccessToken::NONE)
        {
            // TODO return expected RELEASE_ON_NONE_BROKEN_INVARIANT
            std::terminate();
        }
        else if (tokenToBeReleased == AccessToken::LEFT)
        {
            // post can result in either EINVAL or EOVERFLOW; neither of those can be triggered by this code
            IOX_DISCARD_RESULT(m_waitingLineRight.post());
        }
        else
        {
            // post can result in either EINVAL or EOVERFLOW; neither of those can be triggered by this code
            IOX_DISCARD_RESULT(m_waitingLineLeft.post());
        }
    }
}

} // namespace concurrent
} // namespace iox
