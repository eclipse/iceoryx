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

#include "iceoryx_posh/internal/popo/building_blocks/condition_listener.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"

#include "test.hpp"
#include <thread>

using namespace iox;
using namespace iox::popo;
using namespace ::testing;

class TriggerHandle_test : public Test
{
  public:
    void resetCallback(const uint64_t id)
    {
        m_resetCallbackId = id;
    }

    void SetUp()
    {
        m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    }

    uint64_t m_resetCallbackId = 0U;
    ConditionVariableData m_condVar{"Horscht"};
    TriggerHandle_test* m_self = this;

    Watchdog m_watchdog{units::Duration::fromSeconds(2U)};
    TriggerHandle m_sut{m_condVar, {*this, &TriggerHandle_test::resetCallback}, 12U};
};


TEST_F(TriggerHandle_test, IsValidWhenConditionVariableIsNotNull)
{
    EXPECT_TRUE(m_sut.isValid());
    EXPECT_TRUE(m_sut);
}

TEST_F(TriggerHandle_test, DefaultCTorConstructsInvalidHandle)
{
    TriggerHandle sut2;

    EXPECT_FALSE(sut2.isValid());
    EXPECT_THAT(sut2.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
    EXPECT_FALSE(sut2);
}

TEST_F(TriggerHandle_test, InvalidateCreatesInvalidTriggerHandle)
{
    m_sut.invalidate();

    EXPECT_FALSE(m_sut.isValid());
    EXPECT_FALSE(m_sut);
    EXPECT_THAT(m_sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(TriggerHandle_test, ResetCreatesInvalidTriggerHandle)
{
    m_sut.reset();

    EXPECT_FALSE(m_sut.isValid());
    EXPECT_FALSE(m_sut);
    EXPECT_THAT(m_sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(TriggerHandle_test, ResetCallsResetCallbackWhenHandleIsValid)
{
    m_sut.reset();
    EXPECT_EQ(m_resetCallbackId, 12U);
    EXPECT_THAT(m_sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(TriggerHandle_test, ResetDoesNotCallResetCallbackWhenHandleIsInvalid)
{
    m_sut.invalidate();
    m_sut.reset();
    EXPECT_EQ(m_resetCallbackId, 0U);
    EXPECT_THAT(m_sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(TriggerHandle_test, getConditionVariableDataReturnsCorrectVar)
{
    EXPECT_EQ(m_sut.getConditionVariableData(), &m_condVar);
}

TEST_F(TriggerHandle_test, getUniqueIdReturnsCorrectId)
{
    TriggerHandle sut2{m_condVar, {*m_self, &TriggerHandle_test::resetCallback}, 8912U};
    EXPECT_EQ(sut2.getUniqueId(), 8912U);
}

TEST_F(TriggerHandle_test, triggerNotifiesConditionVariable)
{
    std::atomic_int stage{0};

    std::thread t([&] {
        ConditionListener(m_condVar).wait();
        stage.store(1);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(stage.load(), 0);
    m_sut.trigger();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(stage.load(), 1);

    t.join();
}

TEST_F(TriggerHandle_test, wasTriggeredReturnsFalseAfterCreation)
{
    EXPECT_FALSE(m_sut.wasTriggered());
}

TEST_F(TriggerHandle_test, wasTriggeredReturnsFalseWhenHandleIsInvalid)
{
    m_sut.reset();
    EXPECT_FALSE(m_sut.wasTriggered());
}

TEST_F(TriggerHandle_test, wasTriggeredReturnsTrueAfterItWasTriggered)
{
    m_sut.trigger();
    EXPECT_TRUE(m_sut.wasTriggered());
}

TEST_F(TriggerHandle_test, wasTriggeredReturnsFalseAfterItWasTriggeredAndTheListenerResetIt)
{
    m_sut.trigger();
    ConditionListener(m_condVar).timedWait(units::Duration::fromSeconds(0U));
    EXPECT_FALSE(m_sut.wasTriggered());
}

