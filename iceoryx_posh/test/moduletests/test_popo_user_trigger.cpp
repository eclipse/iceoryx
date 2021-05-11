// Copyright (c) 2020, 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"

#include "test.hpp"

using namespace ::testing;
using namespace iox;
using namespace iox::popo;

namespace
{
class WaitSetTest : public iox::popo::WaitSet<>
{
  public:
    WaitSetTest(iox::popo::ConditionVariableData& condVarData) noexcept
        : WaitSet(condVarData)
    {
    }
};

class UserTrigger_test : public Test
{
  public:
    UserTrigger m_sut;
    ConditionVariableData m_condVar{"Horscht"};
    ConditionVariableData m_condVar2{"Schnuppi"};
    WaitSetTest m_waitSet{m_condVar};
    WaitSetTest m_waitSet2{m_condVar2};

    void SetUp()
    {
        m_callbackOrigin = nullptr;
    }

    static UserTrigger* m_callbackOrigin;
    static void callback(UserTrigger* origin)
    {
        m_callbackOrigin = origin;
    }
};
} // namespace

UserTrigger* UserTrigger_test::m_callbackOrigin = nullptr;

TEST_F(UserTrigger_test, IsNotTriggeredWhenCreated)
{
    EXPECT_FALSE(m_sut.hasTriggered());
}

TEST_F(UserTrigger_test, CannotBeTriggeredWhenNotAttached)
{
    m_sut.trigger();
    EXPECT_FALSE(m_sut.hasTriggered());
}

TEST_F(UserTrigger_test, CannotBeTriggeredMultipleTimesWhenNotAttached)
{
    m_sut.trigger();
    m_sut.trigger();
    m_sut.trigger();

    EXPECT_FALSE(m_sut.hasTriggered());
}

TEST_F(UserTrigger_test, CanBeTriggeredWhenAttached)
{
    ASSERT_FALSE(m_waitSet.attachEvent(m_sut).has_error());
    m_sut.trigger();
    EXPECT_TRUE(m_sut.hasTriggered());
}

TEST_F(UserTrigger_test, CanBeTriggeredMultipleTimesWhenAttached)
{
    ASSERT_FALSE(m_waitSet.attachEvent(m_sut).has_error());
    m_sut.trigger();
    m_sut.trigger();
    m_sut.trigger();

    EXPECT_TRUE(m_sut.hasTriggered());
}

TEST_F(UserTrigger_test, UserTriggerGoesOutOfScopeCleansupAtWaitSet)
{
    {
        UserTrigger sut;
        ASSERT_FALSE(m_waitSet.attachEvent(sut).has_error());
    }

    EXPECT_EQ(m_waitSet.size(), 0U);
}

TEST_F(UserTrigger_test, ReattachedUserTriggerCleansUpWhenOutOfScope)
{
    {
        UserTrigger sut;

        ASSERT_FALSE(m_waitSet.attachEvent(sut).has_error());
        ASSERT_FALSE(m_waitSet2.attachEvent(sut).has_error());
    }

    EXPECT_EQ(m_waitSet.size(), 0U);
    EXPECT_EQ(m_waitSet2.size(), 0U);
}

TEST_F(UserTrigger_test, AttachingToAnotherWaitSetCleansupFirstWaitset)
{
    UserTrigger sut;

    ASSERT_FALSE(m_waitSet.attachEvent(m_sut).has_error());
    ASSERT_FALSE(m_waitSet2.attachEvent(m_sut).has_error());

    EXPECT_EQ(m_waitSet.size(), 0U);
    EXPECT_EQ(m_waitSet2.size(), 1U);
}

TEST_F(UserTrigger_test, AttachingToSameWaitsetTwiceLeadsToOneAttachment)
{
    UserTrigger sut;

    ASSERT_FALSE(m_waitSet.attachEvent(m_sut).has_error());
    ASSERT_TRUE(m_waitSet.attachEvent(m_sut).has_error());

    EXPECT_EQ(m_waitSet.size(), 1U);
}

TEST_F(UserTrigger_test, TriggersWaitSet)
{
    using namespace iox::units::duration_literals;
    UserTrigger sut;

    ASSERT_FALSE(m_waitSet.attachEvent(sut, 4412U).has_error());
    sut.trigger();

    auto result = m_waitSet.timedWait(1_s);
    ASSERT_THAT(result.size(), Eq(1U));
    EXPECT_THAT(result[0U]->getNotificationId(), 4412U);
}

TEST_F(UserTrigger_test, DetachingFromAttachedWaitsetCleansUp)
{
    UserTrigger sut;
    ASSERT_FALSE(m_waitSet.attachEvent(sut).has_error());

    m_waitSet.detachEvent(sut);

    EXPECT_EQ(m_waitSet.size(), 0U);
}

TEST_F(UserTrigger_test, UserTriggerCallbackCanBeCalled)
{
    UserTrigger sut;
    ASSERT_FALSE(m_waitSet.attachEvent(sut, 123U, createNotificationCallback(UserTrigger_test::callback)).has_error());
    sut.trigger();

    auto triggerInfoVector = m_waitSet.wait();

    ASSERT_THAT(triggerInfoVector.size(), Eq(1));
    (*triggerInfoVector[0U])();
    EXPECT_THAT(m_callbackOrigin, &sut);
}

TEST_F(UserTrigger_test, UserTriggerCallbackCanBeCalledOverloadWithoutId)
{
    UserTrigger sut;
    ASSERT_FALSE(m_waitSet.attachEvent(sut, 0U, createNotificationCallback(UserTrigger_test::callback)).has_error());
    sut.trigger();

    auto triggerInfoVector = m_waitSet.wait();

    ASSERT_THAT(triggerInfoVector.size(), Eq(1U));
    (*triggerInfoVector[0U])();
    EXPECT_THAT(m_callbackOrigin, &sut);
}
