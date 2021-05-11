// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/notification_info.hpp"

#include "test.hpp"
#include <thread>

using namespace iox;
using namespace iox::popo;
using namespace ::testing;

class NotificationInfo_test : public Test
{
  public:
    class NotificationOriginTest
    {
      public:
        static void callback(NotificationOriginTest* const origin)
        {
            origin->m_callbackOrigin = origin;
        }

        NotificationOriginTest* m_callbackOrigin = nullptr;
    };

    NotificationInfo_test()
    {
    }

    ~NotificationInfo_test()
    {
    }

    NotificationOriginTest m_origin;
    NotificationOriginTest m_falseOrigin;
    NotificationInfo m_sut{&m_origin, 1478U, createNotificationCallback(NotificationOriginTest::callback)};
};

TEST_F(NotificationInfo_test, defaultCTorConstructsEmptyNotificationInfo)
{
    int bla;
    NotificationInfo sut;

    EXPECT_EQ(sut.getNotificationId(), NotificationInfo::INVALID_ID);
    EXPECT_EQ(sut.doesOriginateFrom(&bla), false);
    EXPECT_EQ(sut(), false);
}

TEST_F(NotificationInfo_test, getNotificationIdReturnsValidNotificationId)
{
    EXPECT_EQ(m_sut.getNotificationId(), 1478U);
}

TEST_F(NotificationInfo_test, doesOriginateFromStatesOriginCorrectly)
{
    EXPECT_EQ(m_sut.doesOriginateFrom(&m_origin), true);
    EXPECT_EQ(m_sut.doesOriginateFrom(&m_falseOrigin), false);
}

TEST_F(NotificationInfo_test, getOriginReturnsCorrectOriginWhenHavingCorrectType)
{
    EXPECT_EQ(m_sut.getOrigin<NotificationOriginTest>(), &m_origin);
}

TEST_F(NotificationInfo_test, constGetOriginReturnsCorrectOriginWhenHavingCorrectType)
{
    EXPECT_EQ(const_cast<const NotificationInfo&>(m_sut).getOrigin<NotificationOriginTest>(), &m_origin);
}

TEST_F(NotificationInfo_test, getOriginReturnsNullptrWithWrongType)
{
    auto errorHandlerCalled{false};
    iox::Error errorHandlerType;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });

    m_sut.getOrigin<int>();

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, iox::Error::kPOPO__NOTIFICATION_INFO_TYPE_INCONSISTENCY_IN_GET_ORIGIN);
}

TEST_F(NotificationInfo_test, constGetOriginReturnsNullptrWithWrongType)
{
    auto errorHandlerCalled{false};
    iox::Error errorHandlerType;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerType = error;
            errorHandlerCalled = true;
        });

    const_cast<NotificationInfo&>(m_sut).getOrigin<int>();

    EXPECT_TRUE(errorHandlerCalled);
    EXPECT_EQ(errorHandlerType, iox::Error::kPOPO__NOTIFICATION_INFO_TYPE_INCONSISTENCY_IN_GET_ORIGIN);
}

TEST_F(NotificationInfo_test, triggerCallbackReturnsTrueAndCallsCallbackWithSettedCallback)
{
    EXPECT_TRUE(m_sut());
    EXPECT_EQ(m_origin.m_callbackOrigin, &m_origin);
}

TEST_F(NotificationInfo_test, triggerCallbackReturnsFalseWithUnsetCallback)
{
    m_sut = NotificationInfo{&m_origin, 9U, NotificationCallback<NotificationOriginTest, int>{}};
    EXPECT_FALSE(m_sut());
}
