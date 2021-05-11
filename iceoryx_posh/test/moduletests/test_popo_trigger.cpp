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

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/trigger.hpp"

#include "test.hpp"
#include <thread>

using namespace iox;
using namespace iox::popo;
using namespace ::testing;
using namespace ::testing::internal;

class Trigger_test : public Test
{
  public:
    class TriggerClass
    {
      public:
        bool hasTriggered() const
        {
            return m_hasTriggered;
        }

        void resetCall(const uint64_t trigger)
        {
            m_resetCallTriggerArg = trigger;
        }

        static void callback(TriggerClass* const ptr)
        {
            m_lastCallbackArgument = ptr;
        }

        bool m_hasTriggered = false;
        uint64_t m_resetCallTriggerArg = 0U;
        ConditionVariableData* m_condVar = nullptr;
        static TriggerClass* m_lastCallbackArgument;

        const Trigger* m_moveCallTriggerArg = nullptr;
        void* m_moveCallNewOriginArg = nullptr;
    };

    void SetUp() override
    {
        CaptureStderr();
    }
    void TearDown() override
    {
        std::string output = GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    Trigger
    createValidStateBasedTrigger(const uint64_t eventId = 0U, const uint64_t type = 0U, const uint64_t typeHash = 0U)
    {
        static uint64_t uniqueId = 0U;
        Trigger trigger(StateBasedTrigger,
                        &m_triggerClass,
                        {m_triggerClass, &TriggerClass::hasTriggered},
                        {m_triggerClass, &TriggerClass::resetCall},
                        eventId,
                        createNotificationCallback(TriggerClass::callback),
                        uniqueId++,
                        type,
                        typeHash);
        EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::STATE_BASED));
        return trigger;
    }

    Trigger
    createValidEventBasedTrigger(const uint64_t eventId = 0U, const uint64_t type = 0U, const uint64_t typeHash = 0U)
    {
        static uint64_t uniqueId = 0U;
        Trigger trigger(EventBasedTrigger,
                        &m_triggerClass,
                        {m_triggerClass, &TriggerClass::resetCall},
                        eventId,
                        createNotificationCallback(TriggerClass::callback),
                        uniqueId++,
                        type,
                        typeHash);
        EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::EVENT_BASED));
        return trigger;
    }

    ConditionVariableData m_condVar{"Horscht"};
    TriggerClass m_triggerClass;
};

Trigger_test::TriggerClass* Trigger_test::TriggerClass::m_lastCallbackArgument = nullptr;

// state based trigger

TEST_F(Trigger_test, TriggerWithValidOriginIsValid)
{
    Trigger sut = createValidStateBasedTrigger();

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, MovedConstructedValidTriggerIsValid)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 90001U;
    constexpr uint64_t originTypeHash = 40001U;

    Trigger trigger = createValidStateBasedTrigger(id, originType, originTypeHash);
    Trigger sut{std::move(trigger)};

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::STATE_BASED));
    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, MovedAssignedValidTriggerIsValid)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 190001U;
    constexpr uint64_t originTypeHash = 140001U;
    constexpr uint64_t anotherOriginType = 290001U;
    constexpr uint64_t anotherOriginTypeHash = 240001U;

    Trigger sut = createValidStateBasedTrigger(id, originType, originTypeHash);
    Trigger trigger = createValidStateBasedTrigger(id, anotherOriginType, anotherOriginTypeHash);
    sut = std::move(trigger);

    EXPECT_FALSE(trigger.isValid());
    EXPECT_THAT(trigger.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::STATE_BASED));
    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, anotherOriginType, anotherOriginTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, anotherOriginType, anotherOriginTypeHash));
}

TEST_F(Trigger_test, TriggerWithNullptrOriginIsValid)
{
    constexpr uint64_t eventId = 0U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;

    Trigger sut(StateBasedTrigger,
                static_cast<TriggerClass*>(nullptr),
                {m_triggerClass, &TriggerClass::hasTriggered},
                {m_triggerClass, &TriggerClass::resetCall},
                eventId,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerWithInvalidHasTriggeredCallbackCallsErrorHandlerAndIsInvalid)
{
    constexpr uint64_t eventId = 0U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;

    bool hasTerminated = false;
    iox::Error errorType = iox::Error::kNO_ERROR;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            hasTerminated = true;
            errorType = error;
        });

    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                cxx::ConstMethodCallback<bool>(),
                {m_triggerClass, &TriggerClass::resetCall},
                eventId,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    EXPECT_TRUE(hasTerminated);
    EXPECT_THAT(errorType, Eq(iox::Error::kPOPO__TRIGGER_INVALID_HAS_TRIGGERED_CALLBACK));
    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerWithEmptyResetCallCallsErrorHandlerAndIsInvalid)
{
    constexpr uint64_t eventId = 0U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;

    bool hasTerminated = false;
    iox::Error errorType = iox::Error::kNO_ERROR;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            hasTerminated = true;
            errorType = error;
        });

    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                {m_triggerClass, &TriggerClass::hasTriggered},
                cxx::MethodCallback<void, uint64_t>(),
                eventId,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    EXPECT_TRUE(hasTerminated);
    EXPECT_THAT(errorType, Eq(iox::Error::kPOPO__TRIGGER_INVALID_RESET_CALLBACK));
    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, ResetInvalidatesTrigger)
{
    Trigger sut = createValidStateBasedTrigger();
    sut.reset();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(Trigger_test, InvalidateInvalidatesTrigger)
{
    Trigger sut = createValidStateBasedTrigger();
    sut.invalidate();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
}

TEST_F(Trigger_test, ResetCallsResetcallbackWithCorrectTriggerOrigin)
{
    Trigger sut = createValidStateBasedTrigger();
    auto uniqueId = sut.getUniqueId();
    sut.reset();

    EXPECT_EQ(m_triggerClass.m_resetCallTriggerArg, uniqueId);
}

TEST_F(Trigger_test, ResetSetsTriggerIdToInvalid)
{
    Trigger sut = createValidStateBasedTrigger();
    sut.reset();

    EXPECT_EQ(sut.getUniqueId(), Trigger::INVALID_TRIGGER_ID);
}

TEST_F(Trigger_test, TriggerWithEmptyResetInvalidatesTriggerWhenBeingResetted)
{
    constexpr uint64_t eventId = 0U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;

    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error, const std::function<void()>, const iox::ErrorLevel) {});

    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                {m_triggerClass, &TriggerClass::hasTriggered},
                cxx::MethodCallback<void, uint64_t>(),
                eventId,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    sut.reset();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerCallsHasTriggeredCallback)
{
    Trigger sut = createValidStateBasedTrigger();

    m_triggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.isStateConditionSatisfied());
    m_triggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.isStateConditionSatisfied());
}

TEST_F(Trigger_test, HasTriggeredCallbackReturnsAlwaysFalseWhenInvalid)
{
    Trigger sut = createValidStateBasedTrigger();
    m_triggerClass.m_hasTriggered = true;
    sut.reset();

    EXPECT_FALSE(sut.isStateConditionSatisfied());
}

TEST_F(Trigger_test, UpdateOriginLeadsToDifferentHasTriggeredCallback)
{
    TriggerClass secondTriggerClass;
    Trigger sut = createValidStateBasedTrigger();

    sut.updateOrigin(secondTriggerClass);

    secondTriggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.isStateConditionSatisfied());
    secondTriggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.isStateConditionSatisfied());
}

TEST_F(Trigger_test, TriggerUpdateOriginToSameOriginChangesNothing)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 123U;
    constexpr uint64_t originTypeHash = 123123U;
    Trigger sut = createValidStateBasedTrigger(id, originType, originTypeHash);
    sut.updateOrigin(m_triggerClass);

    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, UpdateOriginDoesNotUpdateHasTriggeredIfItsNotOriginatingFromOrigin)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 891U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;
    TriggerClass secondTriggerClass, thirdTriggerClass;
    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                {thirdTriggerClass, &TriggerClass::hasTriggered},
                {m_triggerClass, &TriggerClass::resetCall},
                USER_DEFINED_EVENT_ID,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    sut.updateOrigin(secondTriggerClass);

    thirdTriggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.isStateConditionSatisfied());
    thirdTriggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.isStateConditionSatisfied());
}

TEST_F(Trigger_test, UpdateOriginLeadsToDifferentResetCallback)
{
    Trigger sut = createValidStateBasedTrigger();
    TriggerClass secondTriggerClass;

    sut.updateOrigin(secondTriggerClass);
    auto uniqueId = sut.getUniqueId();
    sut.reset();

    EXPECT_EQ(secondTriggerClass.m_resetCallTriggerArg, uniqueId);
}

TEST_F(Trigger_test, UpdateOriginDoesNotUpdateResetIfItsNotOriginatingFromOrigin)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 892U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;
    TriggerClass secondTriggerClass, thirdTriggerClass;
    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                {m_triggerClass, &TriggerClass::hasTriggered},
                {thirdTriggerClass, &TriggerClass::resetCall},
                USER_DEFINED_EVENT_ID,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    sut.updateOrigin(secondTriggerClass);
    auto uniqueId = sut.getUniqueId();
    sut.reset();

    EXPECT_EQ(thirdTriggerClass.m_resetCallTriggerArg, uniqueId);
}

TEST_F(Trigger_test, UpdateOriginUpdatesOriginOfNotificationInfo)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 893U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t type = 0U;
    constexpr uint64_t typeHash = 0U;
    TriggerClass secondTriggerClass;
    Trigger sut(StateBasedTrigger,
                &m_triggerClass,
                {m_triggerClass, &TriggerClass::hasTriggered},
                {m_triggerClass, &TriggerClass::resetCall},
                USER_DEFINED_EVENT_ID,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                type,
                typeHash);

    sut.updateOrigin(secondTriggerClass);
    EXPECT_TRUE(sut.getNotificationInfo().doesOriginateFrom(&secondTriggerClass));
}

TEST_F(Trigger_test, TriggerIsLogicalEqualToItself)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 894U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t originType = 4123U;
    constexpr uint64_t originTypeHash = 1423123U;
    Trigger sut1(StateBasedTrigger,
                 &m_triggerClass,
                 {m_triggerClass, &TriggerClass::hasTriggered},
                 {m_triggerClass, &TriggerClass::resetCall},
                 USER_DEFINED_EVENT_ID,
                 createNotificationCallback(TriggerClass::callback),
                 uniqueTriggerId,
                 originType,
                 originTypeHash);

    EXPECT_TRUE(sut1.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, TriggerIsNotLogicalEqualIfOriginTypeDiffers)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 4896U;
    constexpr uint64_t uniqueTriggerId1 = 0U;
    constexpr uint64_t originType = 84123U;
    constexpr uint64_t differentOriginType = 23U;
    constexpr uint64_t originTypeHash = 11423123U;
    Trigger sut1(StateBasedTrigger,
                 &m_triggerClass,
                 {m_triggerClass, &TriggerClass::hasTriggered},
                 {m_triggerClass, &TriggerClass::resetCall},
                 USER_DEFINED_EVENT_ID,
                 createNotificationCallback(TriggerClass::callback),
                 uniqueTriggerId1,
                 originType,
                 originTypeHash);

    EXPECT_FALSE(sut1.isLogicalEqualTo(&m_triggerClass, differentOriginType, originTypeHash));
}

TEST_F(Trigger_test, TriggerIsNotLogicalEqualIfOriginAndOriginTypeHashDiffers)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 4896U;
    constexpr uint64_t uniqueTriggerId1 = 0U;
    constexpr uint64_t originType = 84U;
    constexpr uint64_t differentOriginTypeHash = 823U;
    constexpr uint64_t originTypeHash = 13U;
    TriggerClass secondTriggerClass;
    Trigger sut1(StateBasedTrigger,
                 &m_triggerClass,
                 {m_triggerClass, &TriggerClass::hasTriggered},
                 {m_triggerClass, &TriggerClass::resetCall},
                 USER_DEFINED_EVENT_ID,
                 createNotificationCallback(TriggerClass::callback),
                 uniqueTriggerId1,
                 originType,
                 originTypeHash);

    EXPECT_FALSE(sut1.isLogicalEqualTo(&secondTriggerClass, originType, differentOriginTypeHash));
}

TEST_F(Trigger_test, TriggerIsNotLogicalEqualIfOriginTypeAndOriginTypeHashDiffers)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 4896U;
    constexpr uint64_t uniqueTriggerId1 = 0U;
    constexpr uint64_t originType = 584U;
    constexpr uint64_t differentOriginType = 65823U;
    constexpr uint64_t differentOriginTypeHash = 5823U;
    constexpr uint64_t originTypeHash = 513U;

    TriggerClass secondTriggerClass;
    Trigger sut1(StateBasedTrigger,
                 &m_triggerClass,
                 {m_triggerClass, &TriggerClass::hasTriggered},
                 {m_triggerClass, &TriggerClass::resetCall},
                 USER_DEFINED_EVENT_ID,
                 createNotificationCallback(TriggerClass::callback),
                 uniqueTriggerId1,
                 originType,
                 originTypeHash);

    EXPECT_FALSE(sut1.isLogicalEqualTo(&secondTriggerClass, differentOriginType, differentOriginTypeHash));
}

TEST_F(Trigger_test, TriggerIsNotLogicalEqualWhenInvalid)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 4896U;
    constexpr uint64_t uniqueTriggerId1 = 0U;
    constexpr uint64_t originType = 584U;
    constexpr uint64_t originTypeHash = 513U;

    Trigger sut1(StateBasedTrigger,
                 &m_triggerClass,
                 {m_triggerClass, &TriggerClass::hasTriggered},
                 {m_triggerClass, &TriggerClass::resetCall},
                 USER_DEFINED_EVENT_ID,
                 createNotificationCallback(TriggerClass::callback),
                 uniqueTriggerId1,
                 originType,
                 originTypeHash);
    sut1.invalidate();

    EXPECT_FALSE(sut1.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

// event based trigger

TEST_F(Trigger_test, ValidEventBasedTriggerIsValidAndAlwaysTriggered)
{
    Trigger sut = createValidEventBasedTrigger();
    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(sut.isStateConditionSatisfied());
    EXPECT_THAT(sut.getUniqueId(), Ne(Trigger::INVALID_TRIGGER_ID));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::EVENT_BASED));
}

TEST_F(Trigger_test, InvalidatedEventBasedTriggerIsNotValidAndNotTriggered)
{
    Trigger sut = createValidEventBasedTrigger();
    sut.invalidate();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(sut.isStateConditionSatisfied());
    EXPECT_THAT(sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::INVALID));
}

TEST_F(Trigger_test, ResetEventBasedTriggerIsNotValidAndNotTriggered)
{
    Trigger sut = createValidEventBasedTrigger();
    sut.reset();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(sut.isStateConditionSatisfied());
    EXPECT_THAT(sut.getUniqueId(), Eq(Trigger::INVALID_TRIGGER_ID));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::INVALID));
}

TEST_F(Trigger_test, ValidEventBasedTriggerIsLogicalEqualToSameEventOriginAndEmptyHasTriggeredCallback)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 4584U;
    constexpr uint64_t originTypeHash = 4513U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);

    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, ValidEventBasedTriggerIsNotLogicalEqualToDifferentEventOrigin)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 458U;
    constexpr uint64_t originTypeHash = 413U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    TriggerClass anotherTriggerClass;

    EXPECT_FALSE(sut.isLogicalEqualTo(&anotherTriggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, InvalidEventBasedTriggerIsLogicalEqualToSameEventOrigin)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 4598U;
    constexpr uint64_t originTypeHash = 4883U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    sut.invalidate();

    EXPECT_FALSE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, InvalidEventBasedTriggerIsNotLogicalEqualToDifferentEventOrigin)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 48U;
    constexpr uint64_t originTypeHash = 83U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    sut.invalidate();
    TriggerClass anotherTriggerClass;

    EXPECT_FALSE(sut.isLogicalEqualTo(&anotherTriggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, ValidEventBasedTriggerUpdateOriginWorks)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 24598U;
    constexpr uint64_t originTypeHash = 24883U;
    TriggerClass anotherTriggerClass;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    sut.updateOrigin(anotherTriggerClass);

    EXPECT_TRUE(sut.isLogicalEqualTo(&anotherTriggerClass, originType, originTypeHash));
    EXPECT_FALSE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, ValidEventBasedTriggerUpdateOriginWorksToSameOriginChangesNothing)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 324598U;
    constexpr uint64_t originTypeHash = 324883U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    sut.updateOrigin(m_triggerClass);

    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, InvalidEventBasedTriggerUpdateOriginDoesNotWork)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 424598U;
    constexpr uint64_t originTypeHash = 424883U;
    Trigger sut = createValidEventBasedTrigger(id, originType, originTypeHash);
    sut.invalidate();
    TriggerClass anotherTriggerClass;
    sut.updateOrigin(anotherTriggerClass);

    EXPECT_FALSE(sut.isLogicalEqualTo(&anotherTriggerClass, originType, originTypeHash));
    EXPECT_FALSE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, EventBasedTriggerWithEmptyResetCallInvokesErrorHandlerAndIsInvalid)
{
    constexpr uint64_t eventId = 0U;
    constexpr uint64_t uniqueTriggerId = 0U;
    constexpr uint64_t originType = 0U;
    constexpr uint64_t originTypeHash = 0U;

    bool hasTerminated = false;
    iox::Error errorType = iox::Error::kNO_ERROR;
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            hasTerminated = true;
            errorType = error;
        });

    Trigger sut(EventBasedTrigger,
                &m_triggerClass,
                cxx::MethodCallback<void, uint64_t>(),
                eventId,
                createNotificationCallback(TriggerClass::callback),
                uniqueTriggerId,
                originType,
                originTypeHash);

    EXPECT_TRUE(hasTerminated);
    EXPECT_THAT(errorType, Eq(iox::Error::kPOPO__TRIGGER_INVALID_RESET_CALLBACK));
    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, EventBasedMovedConstructedWithValidTriggerWorks)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 7424598U;
    constexpr uint64_t originTypeHash = 6424883U;
    Trigger trigger = createValidEventBasedTrigger(id, originType, originTypeHash);
    Trigger sut{std::move(trigger)};

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::EVENT_BASED));
    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, EventBasedMovedAssignedWithValidTriggerWorks)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 74598U;
    constexpr uint64_t originTypeHash = 243U;
    constexpr uint64_t anotherOriginType = 11174598U;
    constexpr uint64_t anotherOriginTypeHash = 111243U;

    Trigger sut = createValidStateBasedTrigger(id, originType, originTypeHash);
    Trigger trigger = createValidEventBasedTrigger(id, anotherOriginType, anotherOriginTypeHash);
    sut = std::move(trigger);

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::EVENT_BASED));
    EXPECT_TRUE(sut.isLogicalEqualTo(&m_triggerClass, anotherOriginType, anotherOriginTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, anotherOriginType, anotherOriginTypeHash));
}

TEST_F(Trigger_test, EventBasedMovedConstructedWithInvalidTrigger)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 997458U;
    constexpr uint64_t originTypeHash = 99243U;
    Trigger trigger = createValidEventBasedTrigger(id, originType, originTypeHash);
    Trigger trigger1 = std::move(trigger);
    Trigger sut{std::move(trigger)};

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));
}

TEST_F(Trigger_test, EventBasedMovedAssignedWithInvalidTrigger)
{
    constexpr uint64_t id = 0U;
    constexpr uint64_t originType = 740598U;
    constexpr uint64_t originTypeHash = 20043U;
    constexpr uint64_t anotherOriginType = 111074598U;
    constexpr uint64_t anotherOriginTypeHash = 10011243U;
    Trigger sut = createValidStateBasedTrigger(id, originType, originTypeHash);
    Trigger trigger = createValidEventBasedTrigger(id, anotherOriginType, anotherOriginTypeHash);
    Trigger trigger1 = std::move(trigger);
    sut = std::move(trigger);

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
    EXPECT_THAT(sut.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(sut.isLogicalEqualTo(&m_triggerClass, originType, originTypeHash));

    EXPECT_FALSE(trigger.isValid());
    EXPECT_FALSE(static_cast<bool>(trigger));
    EXPECT_THAT(trigger.getTriggerType(), Eq(TriggerType::INVALID));
    EXPECT_FALSE(trigger.isLogicalEqualTo(&m_triggerClass, anotherOriginType, anotherOriginTypeHash));
}
