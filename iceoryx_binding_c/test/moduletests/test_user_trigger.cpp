// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/popo/user_trigger.hpp"
#include "mocks/wait_set_mock.hpp"

using namespace iox;
using namespace iox::popo;

extern "C" {
#include "iceoryx_binding_c/user_trigger.h"
#include "iceoryx_binding_c/wait_set.h"
}

#include "test.hpp"

using namespace ::testing;

class iox_user_trigger_test : public Test
{
  public:
    void SetUp()
    {
        m_sut = iox_user_trigger_init(&m_sutStorage);
        wasTriggerCallbackCalled = false;
    }

    void TearDown()
    {
        iox_user_trigger_deinit(m_sut);
    }

    static void triggerCallback(iox_user_trigger_t)
    {
        wasTriggerCallbackCalled = true;
    }

    iox_user_trigger_storage_t m_sutStorage;
    iox_user_trigger_t m_sut;

    ConditionVariableData m_condVar{"Horscht"};
    WaitSetMock m_waitSet{m_condVar};
    static bool wasTriggerCallbackCalled;
};

bool iox_user_trigger_test::wasTriggerCallbackCalled = false;

TEST_F(iox_user_trigger_test, initUserTriggerWithNullptrForStorageReturnsNullptr)
{
    EXPECT_EQ(iox_user_trigger_init(nullptr), nullptr);
}

TEST_F(iox_user_trigger_test, isNotTriggeredWhenCreated)
{
    EXPECT_FALSE(iox_user_trigger_has_triggered(m_sut));
}

TEST_F(iox_user_trigger_test, cannotBeTriggeredWhenNotAttached)
{
    iox_user_trigger_trigger(m_sut);
    EXPECT_FALSE(iox_user_trigger_has_triggered(m_sut));
}

TEST_F(iox_user_trigger_test, canBeTriggeredWhenAttached)
{
    iox_ws_attach_user_trigger_event(&m_waitSet, m_sut, 0U, NULL);
    iox_user_trigger_trigger(m_sut);
    EXPECT_TRUE(iox_user_trigger_has_triggered(m_sut));
}

TEST_F(iox_user_trigger_test, triggeringWaitSetResultsInCorrectNotificationId)
{
    iox_ws_attach_user_trigger_event(&m_waitSet, m_sut, 88191U, NULL);
    iox_user_trigger_trigger(m_sut);

    auto eventVector = m_waitSet.wait();

    ASSERT_THAT(eventVector.size(), Eq(1U));
    EXPECT_EQ(eventVector[0U]->getNotificationId(), 88191U);
}

TEST_F(iox_user_trigger_test, triggeringWaitSetResultsInCorrectCallback)
{
    iox_ws_attach_user_trigger_event(&m_waitSet, m_sut, 0U, iox_user_trigger_test::triggerCallback);
    iox_user_trigger_trigger(m_sut);

    auto eventVector = m_waitSet.wait();

    ASSERT_THAT(eventVector.size(), Eq(1U));
    (*eventVector[0U])();

    EXPECT_TRUE(wasTriggerCallbackCalled);
}

TEST_F(iox_user_trigger_test, attachingToAnotherWaitSetCleansupFirstWaitset)
{
    WaitSetMock m_waitSet2{m_condVar};
    iox_ws_attach_user_trigger_event(&m_waitSet, m_sut, 0U, NULL);

    iox_ws_attach_user_trigger_event(&m_waitSet2, m_sut, 0U, NULL);

    EXPECT_EQ(m_waitSet.size(), 0U);
    EXPECT_EQ(m_waitSet2.size(), 1U);
}

TEST_F(iox_user_trigger_test, disable_trigger_eventingItFromWaitsetCleansup)
{
    WaitSetMock m_waitSet2{m_condVar};
    iox_ws_attach_user_trigger_event(&m_waitSet, m_sut, 0U, NULL);

    iox_ws_detach_user_trigger_event(&m_waitSet, m_sut);

    EXPECT_EQ(m_waitSet.size(), 0U);
}
