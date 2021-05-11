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

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"
#include "test.hpp"

#include <chrono>
#include <memory>
#include <thread>

using namespace ::testing;
using ::testing::Return;
using namespace iox::popo;
using namespace iox::cxx;
using namespace iox::units::duration_literals;

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

enum class SimpleEvent1 : iox::popo::EventEnumIdentifier
{
    EVENT1 = 0,
    EVENT2 = 1,
    INVALID = 2
};

enum class SimpleEvent2 : iox::popo::EventEnumIdentifier
{
    EVENT1 = 0,
    EVENT2 = 1,
    INVALID = 2
};

enum class SimpleState1 : iox::popo::StateEnumIdentifier
{
    STATE1 = 0,
    STATE2 = 1,
    INVALID = 2
};

enum class SimpleState2 : iox::popo::StateEnumIdentifier
{
    STATE1 = 0,
    STATE2 = 1,
    INVALID = 2
};

class WaitSet_test : public Test
{
  public:
    class SimpleEventClass
    {
      public:
        SimpleEventClass() = default;
        SimpleEventClass(const SimpleEventClass&) = delete;
        SimpleEventClass(SimpleEventClass&&) = delete;

        SimpleEventClass& operator=(const SimpleEventClass&) = delete;
        SimpleEventClass& operator=(SimpleEventClass&&) = delete;
        ~SimpleEventClass() = default;

        bool hasEventSet() const noexcept
        {
            return static_cast<bool>(m_eventHandle);
        }

        bool hasStateSet() const noexcept
        {
            return static_cast<bool>(m_stateHandle);
        }

        void enableEvent(iox::popo::TriggerHandle&& handle) noexcept
        {
            m_eventHandle = std::move(handle);
        }

        void enableEvent(iox::popo::TriggerHandle&& handle, const SimpleEvent1 event) noexcept
        {
            m_eventHandle = std::move(handle);
            m_simpleEvent1 = event;
        }

        void enableEvent(iox::popo::TriggerHandle&& handle, const SimpleEvent2 event) noexcept
        {
            m_eventHandle = std::move(handle);
            m_simpleEvent2 = event;
        }

        void enableState(iox::popo::TriggerHandle&& handle) noexcept
        {
            m_stateHandle = std::move(handle);
        }

        void enableState(iox::popo::TriggerHandle&& handle, const SimpleState1 state) noexcept
        {
            m_stateHandle = std::move(handle);
            m_simpleState1 = state;
        }

        void enableState(iox::popo::TriggerHandle&& handle, const SimpleState2 state) noexcept
        {
            m_stateHandle = std::move(handle);
            m_simpleState2 = state;
        }

        void invalidateTrigger(const uint64_t id)
        {
            m_invalidateTriggerId.emplace_back(id);
            m_stateHandle.invalidate();
            m_eventHandle.invalidate();
        }

        iox::popo::WaitSetIsConditionSatisfiedCallback getCallbackForIsStateConditionSatisfied() const noexcept
        {
            return (m_isEventBased) ? iox::cxx::ConstMethodCallback<bool>()
                                    : iox::cxx::ConstMethodCallback<bool>{*this, &SimpleEventClass::hasTriggered};
        }

        iox::popo::WaitSetIsConditionSatisfiedCallback
        getCallbackForIsStateConditionSatisfied(SimpleState1 state) const noexcept
        {
            m_simpleState1TriggerCallback = state;
            return (m_isEventBased) ? iox::cxx::ConstMethodCallback<bool>()
                                    : iox::cxx::ConstMethodCallback<bool>{*this, &SimpleEventClass::hasTriggered};
        }

        iox::popo::WaitSetIsConditionSatisfiedCallback
        getCallbackForIsStateConditionSatisfied(SimpleState2 state) const noexcept
        {
            m_simpleState2TriggerCallback = state;
            return (m_isEventBased) ? iox::cxx::ConstMethodCallback<bool>()
                                    : iox::cxx::ConstMethodCallback<bool>{*this, &SimpleEventClass::hasTriggered};
        }

        bool hasTriggered() const
        {
            if (m_autoResetTrigger)
            {
                return m_hasTriggered.exchange(false);
            }
            return m_hasTriggered.load();
        }

        void disableEvent()
        {
            m_eventHandle.reset();
        }

        void disableState()
        {
            m_stateHandle.reset();
        }

        uint64_t getUniqueStateId() const
        {
            return m_stateHandle.getUniqueId();
        }

        uint64_t getUniqueNotificationId() const
        {
            return m_eventHandle.getUniqueId();
        }

        void trigger()
        {
            m_hasTriggered.store(true);
            m_stateHandle.trigger();
            m_eventHandle.trigger();
        }

        void resetTrigger()
        {
            m_hasTriggered.store(false);
        }

        iox::popo::TriggerHandle m_eventHandle;
        iox::popo::TriggerHandle m_stateHandle;
        mutable std::atomic_bool m_hasTriggered{false};
        static std::vector<uint64_t> m_invalidateTriggerId;

        static SimpleEvent1 m_simpleEvent1;
        static SimpleEvent2 m_simpleEvent2;
        static SimpleState1 m_simpleState1;
        static SimpleState2 m_simpleState2;
        static SimpleState1 m_simpleState1TriggerCallback;
        static SimpleState2 m_simpleState2TriggerCallback;

        SimpleEventClass* m_triggerCallbackArgument1 = nullptr;
        SimpleEventClass* m_triggerCallbackArgument2 = nullptr;
        uint64_t* m_contextData1 = nullptr;
        uint64_t* m_contextData2 = nullptr;
        bool m_autoResetTrigger = true;
        bool m_isEventBased = false;
    };

    ConditionVariableData m_condVarData{"Horscht"};
    optional<WaitSetTest> m_sut;

    static void triggerCallback1(WaitSet_test::SimpleEventClass* const waitset)
    {
        waitset->m_triggerCallbackArgument1 = waitset;
    }

    static void triggerCallback2(WaitSet_test::SimpleEventClass* const waitset)
    {
        waitset->m_triggerCallbackArgument2 = waitset;
    }

    static void triggerCallback1WithContextData(WaitSet_test::SimpleEventClass* const waitset,
                                                uint64_t* const contextData)
    {
        waitset->m_triggerCallbackArgument1 = waitset;
        waitset->m_contextData1 = contextData;
    }

    static void triggerCallback2WithContextData(WaitSet_test::SimpleEventClass* const waitset,
                                                uint64_t* const contextData)
    {
        waitset->m_triggerCallbackArgument2 = waitset;
        waitset->m_contextData2 = contextData;
    }

    void SetUp() override
    {
        SimpleEventClass::m_simpleEvent1 = SimpleEvent1::INVALID;
        SimpleEventClass::m_simpleEvent2 = SimpleEvent2::INVALID;
        SimpleEventClass::m_simpleState1 = SimpleState1::INVALID;
        SimpleEventClass::m_simpleState2 = SimpleState2::INVALID;
        SimpleEventClass::m_simpleState1TriggerCallback = SimpleState1::INVALID;
        SimpleEventClass::m_simpleState2TriggerCallback = SimpleState2::INVALID;
        m_sut.emplace(m_condVarData);
        WaitSet_test::SimpleEventClass::m_invalidateTriggerId.clear();
        m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    }

    void TearDown() override
    {
    }

    template <uint64_t NotificationInfoVectorCapacity, typename EventOrigin>
    static bool doesNotificationInfoVectorContain(
        const iox::cxx::vector<const NotificationInfo*, NotificationInfoVectorCapacity>& eventInfoVector,
        const uint64_t eventId,
        const EventOrigin& origin)
    {
        for (auto& e : eventInfoVector)
        {
            if (e->getNotificationId() == eventId && e->doesOriginateFrom(&origin)
                && e->template getOrigin<EventOrigin>() == &origin)
            {
                return true;
            }
        }
        return false;
    }

    bool attachAllEvents()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[i], i));
            EXPECT_TRUE(m_simpleEvents[i].hasEventSet());
            EXPECT_FALSE(m_simpleEvents[i].hasStateSet());
            EXPECT_THAT(m_sut->size(), Eq(i + 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == m_sut->capacity();
    }

    bool attachAllStates()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            EXPECT_TRUE(m_sut->attachState(m_simpleEvents[i], i));
            EXPECT_FALSE(m_simpleEvents[i].hasEventSet());
            EXPECT_TRUE(m_simpleEvents[i].hasStateSet());
            EXPECT_THAT(m_sut->size(), Eq(i + 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == m_sut->capacity();
    }

    bool attachAllWithEventStateMix()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            if (i % 2 == 0U)
            {
                EXPECT_TRUE(m_sut->attachState(m_simpleEvents[i], i));
                EXPECT_FALSE(m_simpleEvents[i].hasEventSet());
                EXPECT_TRUE(m_simpleEvents[i].hasStateSet());
            }
            else
            {
                EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[i], i));
                EXPECT_TRUE(m_simpleEvents[i].hasEventSet());
                EXPECT_FALSE(m_simpleEvents[i].hasStateSet());
            }
            EXPECT_THAT(m_sut->size(), Eq(i + 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == m_sut->capacity();
    }

    bool detachAllEvents()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            m_sut->detachEvent(m_simpleEvents[i]);
            EXPECT_FALSE(m_simpleEvents[i].hasEventSet());
            EXPECT_FALSE(m_simpleEvents[i].hasStateSet());
            EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - i - 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == 0U;
    }

    bool detachAllStates()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            m_sut->detachState(m_simpleEvents[i]);
            EXPECT_FALSE(m_simpleEvents[i].hasEventSet());
            EXPECT_FALSE(m_simpleEvents[i].hasStateSet());
            EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - i - 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == 0U;
    }

    bool detachAllWithEventStateMix()
    {
        for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
        {
            if (i % 2 == 0U)
            {
                m_sut->detachState(m_simpleEvents[i]);
            }
            else
            {
                m_sut->detachEvent(m_simpleEvents[i]);
            }
            EXPECT_FALSE(m_simpleEvents[i].hasEventSet());
            EXPECT_FALSE(m_simpleEvents[i].hasStateSet());
            EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - i - 1U));
            EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
        }

        return m_sut->size() == 0U;
    }


    const iox::units::Duration m_timeToWait = 2_s;
    Watchdog m_watchdog{m_timeToWait};
    using eventVector_t = iox::cxx::vector<SimpleEventClass, iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1>;
    eventVector_t m_simpleEvents{iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET + 1};
};
std::vector<uint64_t> WaitSet_test::SimpleEventClass::m_invalidateTriggerId;
SimpleEvent1 WaitSet_test::SimpleEventClass::m_simpleEvent1 = SimpleEvent1::INVALID;
SimpleEvent2 WaitSet_test::SimpleEventClass::m_simpleEvent2 = SimpleEvent2::INVALID;
SimpleState1 WaitSet_test::SimpleEventClass::m_simpleState1 = SimpleState1::INVALID;
SimpleState2 WaitSet_test::SimpleEventClass::m_simpleState2 = SimpleState2::INVALID;
SimpleState1 WaitSet_test::SimpleEventClass::m_simpleState1TriggerCallback = SimpleState1::INVALID;
SimpleState2 WaitSet_test::SimpleEventClass::m_simpleState2TriggerCallback = SimpleState2::INVALID;
} // namespace

////////////////////////
// BEGIN attach / detach
////////////////////////

TEST_F(WaitSet_test, AttachEventOnceIsSuccessful)
{
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0]).has_error());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
}

TEST_F(WaitSet_test, AttachMaxEventsIsSuccessful)
{
    EXPECT_TRUE(attachAllEvents());
}

TEST_F(WaitSet_test, AttachMoreThanMaxEventsFails)
{
    EXPECT_TRUE(attachAllEvents());

    EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]).has_error());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
    EXPECT_THAT(m_sut->size(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
}

TEST_F(WaitSet_test, AttachStateOnceIsSuccessful)
{
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0]).has_error());
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
}

TEST_F(WaitSet_test, AttachMaxStatesIsSuccessful)
{
    EXPECT_TRUE(attachAllStates());
}

TEST_F(WaitSet_test, AttachMoreThanMaxStatesFails)
{
    EXPECT_TRUE(attachAllStates());

    EXPECT_TRUE(m_sut->attachState(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]).has_error());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
    EXPECT_THAT(m_sut->size(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
}

TEST_F(WaitSet_test, AttachMoreThanMaxFailsWithMixedEventsStates)
{
    EXPECT_TRUE(attachAllWithEventStateMix());

    EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]).has_error());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
    EXPECT_THAT(m_sut->size(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));
}

TEST_F(WaitSet_test, AttachingSameEventTwiceResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 0U;
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachEvent(m_simpleEvents[0], USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameStateTwiceResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 0U;
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachState(m_simpleEvents[0], USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameEventWithNonNullIdTwiceResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 121U;
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachEvent(m_simpleEvents[0], USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameStateWithNonNullIdTwiceResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 121U;
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachState(m_simpleEvents[0], USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameEventWithDifferentIdResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 2101U;
    constexpr uint64_t ANOTHER_USER_DEFINED_EVENT_ID = 9121U;
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachEvent(m_simpleEvents[0], ANOTHER_USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
}

TEST_F(WaitSet_test, AttachingSameStateWithDifferentIdResultsInError)
{
    constexpr uint64_t USER_DEFINED_EVENT_ID = 2101U;
    constexpr uint64_t ANOTHER_USER_DEFINED_EVENT_ID = 9121U;
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0], USER_DEFINED_EVENT_ID).has_error());
    auto result2 = m_sut->attachState(m_simpleEvents[0], ANOTHER_USER_DEFINED_EVENT_ID);

    ASSERT_TRUE(result2.has_error());
    EXPECT_THAT(result2.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
}

TEST_F(WaitSet_test, DetachingAttachedEventIsSuccessful)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0]).has_error());
    m_sut->detachEvent(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(0U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, DetachingAttachedStateIsSuccessful)
{
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0]).has_error());
    m_sut->detachState(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(0U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, DetachingAttachedEventTwiceWorks)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0]).has_error());
    m_sut->detachEvent(m_simpleEvents[0]);
    m_sut->detachEvent(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(0U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, DetachingAttachedStateTwiceWorks)
{
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0]).has_error());
    m_sut->detachState(m_simpleEvents[0]);
    m_sut->detachState(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(0U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, DetachingMakesSpaceForAnotherEvent)
{
    ASSERT_TRUE(attachAllEvents());

    m_sut->detachEvent(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));

    EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]));
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity()));
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
}

TEST_F(WaitSet_test, DetachingMakesSpaceForAnotherState)
{
    ASSERT_TRUE(attachAllStates());

    m_sut->detachState(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));

    EXPECT_TRUE(m_sut->attachState(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]));
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity()));
    EXPECT_TRUE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
}

TEST_F(WaitSet_test, DetachingMakesSpaceForAnotherAttachmentWithMixedEventsStates)
{
    ASSERT_TRUE(attachAllWithEventStateMix());

    m_sut->detachState(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));

    EXPECT_TRUE(m_sut->attachState(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET]));
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity()));
    EXPECT_TRUE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET].hasEventSet());
}

TEST_F(WaitSet_test, DetachingAllEventAttachmentsOfFullWaitSetIsSuccessful)
{
    EXPECT_TRUE(attachAllEvents());
    EXPECT_TRUE(detachAllEvents());
}

TEST_F(WaitSet_test, DetachingAllStateAttachmentsOfFullWaitSetIsSuccessful)
{
    EXPECT_TRUE(attachAllStates());
    EXPECT_TRUE(detachAllStates());
}

TEST_F(WaitSet_test, DetachingAllMixedAttachmentsOfFullWaitSetIsSuccessful)
{
    EXPECT_TRUE(attachAllWithEventStateMix());
    EXPECT_TRUE(detachAllWithEventStateMix());
}

TEST_F(WaitSet_test, DetachingAttachedEventWithDetachStateChangesNothing)
{
    EXPECT_TRUE(m_sut->attachEvent(m_simpleEvents[0]));

    m_sut->detachState(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, DetachingAttachedStateWithDetachEventChangesNothing)
{
    EXPECT_TRUE(m_sut->attachState(m_simpleEvents[0]));

    m_sut->detachEvent(m_simpleEvents[0]);
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingEventWithEnumIsSuccessful)
{
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT1).has_error());
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleEvent1, Eq(SimpleEvent1::EVENT1));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameEventWithEnumFails)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT1).has_error());

    auto result = m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT1);
    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_THAT(SimpleEventClass::m_simpleEvent1, Eq(SimpleEvent1::EVENT1));
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameEventWithDifferentEnumValueSucceeds)
{
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT1).has_error());
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT2).has_error());

    // SimpleEvents has only one handler for the attachedEvents, if another is attached the first
    // one is detached, therefore the size == 1
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleEvent1, Eq(SimpleEvent1::EVENT2));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameEventWithDifferentEnumTypeSucceeds)
{
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent1::EVENT1).has_error());
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0], SimpleEvent2::EVENT1).has_error());

    // SimpleEvents has only one handler for the attachedEvents, if another is attached the first
    // one is detached, therefore the size == 1
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleEvent2, Eq(SimpleEvent2::EVENT1));
    EXPECT_FALSE(m_simpleEvents[0].hasStateSet());
    EXPECT_TRUE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingStateWithEnumIsSuccessful)
{
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE1).has_error());
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleState1, Eq(SimpleState1::STATE1));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameStateWithEnumFails)
{
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE1).has_error());

    auto result = m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE1);
    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(WaitSetError::ALREADY_ATTACHED));
    EXPECT_THAT(SimpleEventClass::m_simpleState1, Eq(SimpleState1::STATE1));
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameStateWithDifferentEnumValueSucceeds)
{
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE1).has_error());
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE2).has_error());

    // SimpleEvents has only one handler for the attachedStates, if another is attached the first
    // one is detached, therefore the size == 1
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleState1, Eq(SimpleState1::STATE2));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

TEST_F(WaitSet_test, AttachingSameStateWithDifferentEnumTypeSucceeds)
{
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState1::STATE1).has_error());
    EXPECT_FALSE(m_sut->attachState(m_simpleEvents[0], SimpleState2::STATE1).has_error());

    // SimpleEvents has only one handler for the attachedEvents, if another is attached the first
    // one is detached, therefore the size == 1
    EXPECT_THAT(m_sut->size(), Eq(1U));
    EXPECT_THAT(SimpleEventClass::m_simpleState2, Eq(SimpleState2::STATE1));
    EXPECT_TRUE(m_simpleEvents[0].hasStateSet());
    EXPECT_FALSE(m_simpleEvents[0].hasEventSet());
}

////////////////////////
// END
////////////////////////

////////////////////////
// BEGIN lifetime
////////////////////////

TEST_F(WaitSet_test, ResetCallbackIsCalledWhenWaitsetGoesOutOfScope)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0]).has_error());
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[1]).has_error());
    std::vector<uint64_t> uniqueTriggerIds;
    uniqueTriggerIds.emplace_back(m_simpleEvents[0].getUniqueNotificationId());
    uniqueTriggerIds.emplace_back(m_simpleEvents[1].getUniqueStateId());
    m_sut.reset();

    std::sort(uniqueTriggerIds.begin(), uniqueTriggerIds.end());
    std::sort(SimpleEventClass::m_invalidateTriggerId.begin(), SimpleEventClass::m_invalidateTriggerId.end());

    EXPECT_THAT(uniqueTriggerIds, Eq(SimpleEventClass::m_invalidateTriggerId));
}

TEST_F(WaitSet_test, ResetCallbackIsCalledWhenFullWaitsetGoesOutOfScope)
{
    attachAllWithEventStateMix();
    std::vector<uint64_t> uniqueTriggerIds;
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        if (i % 2 == 0U)
        {
            uniqueTriggerIds.emplace_back(m_simpleEvents[i].getUniqueStateId());
        }
        else
        {
            uniqueTriggerIds.emplace_back(m_simpleEvents[i].getUniqueNotificationId());
        }
    }
    m_sut.reset();

    std::sort(uniqueTriggerIds.begin(), uniqueTriggerIds.end());
    std::sort(SimpleEventClass::m_invalidateTriggerId.begin(), SimpleEventClass::m_invalidateTriggerId.end());

    EXPECT_THAT(uniqueTriggerIds, Eq(SimpleEventClass::m_invalidateTriggerId));
}

TEST_F(WaitSet_test, EventAttachmentRemovesItselfFromWaitsetWhenGoingOutOfScope)
{
    for (uint64_t i = 0U; i + 1U < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[i], 100U + i).has_error());
    }

    constexpr uint64_t USER_DEFINED_EVENT_ID = 0U;
    optional<SimpleEventClass> temporaryTrigger;
    temporaryTrigger.emplace();
    ASSERT_FALSE(m_sut->attachEvent(*temporaryTrigger, USER_DEFINED_EVENT_ID).has_error());
    // goes out of scope here and creates space again for an additional trigger
    // if this doesn't work we are unable to acquire another trigger since the
    // waitset is already full
    temporaryTrigger.reset();
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));
    temporaryTrigger.emplace();

    EXPECT_FALSE(m_sut->attachEvent(*temporaryTrigger, USER_DEFINED_EVENT_ID).has_error());
}

TEST_F(WaitSet_test, StateAttachmentRemovesItselfFromWaitsetWhenGoingOutOfScope)
{
    for (uint64_t i = 0U; i + 1U < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(m_sut->attachState(m_simpleEvents[i], 100U + i).has_error());
    }

    constexpr uint64_t USER_DEFINED_EVENT_ID = 0U;
    optional<SimpleEventClass> temporaryTrigger;
    temporaryTrigger.emplace();
    ASSERT_FALSE(m_sut->attachState(*temporaryTrigger, USER_DEFINED_EVENT_ID).has_error());
    // goes out of scope here and creates space again for an additional trigger
    // if this doesn't work we are unable to acquire another trigger since the
    // waitset is already full
    temporaryTrigger.reset();
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));
    temporaryTrigger.emplace();

    EXPECT_FALSE(m_sut->attachState(*temporaryTrigger, USER_DEFINED_EVENT_ID).has_error());
}

TEST_F(WaitSet_test, MultipleAttachmentsRemovingThemselfFromWaitsetWhenGoingOutOfScope)
{
    attachAllWithEventStateMix();

    // here the attachments go out of scope
    m_simpleEvents.clear();

    EXPECT_THAT(m_sut->size(), Eq(0U));
}

TEST_F(WaitSet_test, AttachmentsGoingOutOfScopeReducesSize)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0]).has_error());
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[1]).has_error());
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[2]).has_error());
    {
        SimpleEventClass simpleEvent1, simpleEvent2;
        ASSERT_FALSE(m_sut->attachEvent(simpleEvent1).has_error());
        ASSERT_FALSE(m_sut->attachEvent(simpleEvent2).has_error());
        EXPECT_EQ(m_sut->size(), 5U);
    }

    EXPECT_EQ(m_sut->size(), 3U);
}

////////////////////////
// END
////////////////////////

////////////////////////
// BEGIN trigger and blocking
////////////////////////
TEST_F(WaitSet_test, WaitBlocksWhenNothingTriggered)
{
    std::atomic_bool doStartWaiting{false};
    std::atomic_bool isThreadFinished{false};
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[i], 5U + i).has_error());
    }

    std::thread t([&] {
        doStartWaiting.store(true);
        auto triggerVector = m_sut->wait();
        isThreadFinished.store(true);
    });

    while (!doStartWaiting.load())
        ;


    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(isThreadFinished.load());

    m_simpleEvents[0].trigger();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(isThreadFinished.load());

    t.join();
}

TEST_F(WaitSet_test, TimedWaitReturnsNothingWhenNothingTriggered)
{
    iox::cxx::vector<expected<TriggerHandle, WaitSetError>*, iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET> trigger;
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[i], 5U + i).has_error());
    }

    auto triggerVector = m_sut->timedWait(10_ms);
    ASSERT_THAT(triggerVector.size(), Eq(0U));
}

void WaitReturnsTheOneTriggeredCondition(WaitSet_test* test,
                                         const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(test->m_sut->attachEvent(test->m_simpleEvents[i], 5U + i).has_error());
    }

    test->m_simpleEvents[0].trigger();

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(1U));
    EXPECT_THAT(triggerVector[0U]->getNotificationId(), 5U);
    EXPECT_TRUE(triggerVector[0U]->doesOriginateFrom(&test->m_simpleEvents[0]));
    EXPECT_EQ(triggerVector[0U]->getOrigin<WaitSet_test::SimpleEventClass>(), &test->m_simpleEvents[0]);
}

TEST_F(WaitSet_test, WaitReturnsTheOneTriggeredCondition)
{
    WaitReturnsTheOneTriggeredCondition(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsTheOneTriggeredCondition)
{
    WaitReturnsTheOneTriggeredCondition(this, [&] { return m_sut->timedWait(10_ms); });
}

void WaitReturnsAllTriggeredConditionWhenMultipleAreTriggered(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(test->m_sut->attachEvent(test->m_simpleEvents[i], 100U + i).has_error());
    }

    for (uint64_t i = 0U; i < 24U; ++i)
    {
        test->m_simpleEvents[i].trigger();
    }

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(24U));

    for (uint64_t i = 0U; i < 24U; ++i)
    {
        EXPECT_TRUE(WaitSet_test::doesNotificationInfoVectorContain(triggerVector, 100U + i, test->m_simpleEvents[i]));
    }
}

TEST_F(WaitSet_test, WaitReturnsAllTriggeredConditionWhenMultipleAreTriggered)
{
    WaitReturnsAllTriggeredConditionWhenMultipleAreTriggered(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsAllTriggeredConditionWhenMultipleAreTriggered)
{
    WaitReturnsAllTriggeredConditionWhenMultipleAreTriggered(this, [&] { return m_sut->timedWait(10_ms); });
}


void WaitReturnsAllTriggeredConditionWhenAllAreTriggered(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        ASSERT_FALSE(test->m_sut->attachEvent(test->m_simpleEvents[i], i * 3U + 2U).has_error());
    }

    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        test->m_simpleEvents[i].trigger();
    }

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET));

    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; ++i)
    {
        EXPECT_TRUE(
            WaitSet_test::doesNotificationInfoVectorContain(triggerVector, i * 3U + 2U, test->m_simpleEvents[i]));
    }
}

TEST_F(WaitSet_test, WaitReturnsAllTriggeredConditionWhenAllAreTriggered)
{
    WaitReturnsAllTriggeredConditionWhenAllAreTriggered(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsAllTriggeredConditionWhenAllAreTriggered)
{
    WaitReturnsAllTriggeredConditionWhenAllAreTriggered(this, [&] { return m_sut->timedWait(10_ms); });
}

void WaitReturnsEventTriggersWithOneCorrectCallback(WaitSet_test* test,
                                                    const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    auto result1 = test->m_sut->attachEvent(
        test->m_simpleEvents[0], 1U, createNotificationCallback(WaitSet_test::triggerCallback1));

    ASSERT_THAT(result1.has_error(), Eq(false));

    test->m_simpleEvents[0].trigger();

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(1U));

    (*triggerVector[0U])();

    EXPECT_THAT(test->m_simpleEvents[0].m_triggerCallbackArgument1, Eq(&test->m_simpleEvents[0]));
}

TEST_F(WaitSet_test, WaitReturnsEventTriggersWithOneCorrectCallback)
{
    WaitReturnsEventTriggersWithOneCorrectCallback(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsEventTriggersWithTwoCorrectCallback)
{
    WaitReturnsEventTriggersWithOneCorrectCallback(this, [&] { return m_sut->timedWait(10_ms); });
}

void WaitReturnsEventTriggersWithTwoCorrectCallbacksWithContextData(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    uint64_t contextData1 = 0U;
    uint64_t contextData2 = 0U;
    auto result1 = test->m_sut->attachEvent(
        test->m_simpleEvents[0],
        1U,
        createNotificationCallback(WaitSet_test::triggerCallback1WithContextData, contextData1));
    auto result2 = test->m_sut->attachEvent(
        test->m_simpleEvents[1],
        2U,
        createNotificationCallback(WaitSet_test::triggerCallback2WithContextData, contextData2));

    ASSERT_THAT(result1.has_error(), Eq(false));
    ASSERT_THAT(result2.has_error(), Eq(false));

    test->m_simpleEvents[0].trigger();
    test->m_simpleEvents[1].trigger();

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(2U));

    (*triggerVector[0U])();
    (*triggerVector[1U])();

    EXPECT_THAT(test->m_simpleEvents[0].m_triggerCallbackArgument1, Eq(&test->m_simpleEvents[0]));
    EXPECT_THAT(test->m_simpleEvents[1].m_triggerCallbackArgument2, Eq(&test->m_simpleEvents[1]));
    EXPECT_THAT(test->m_simpleEvents[0].m_contextData1, Eq(&contextData1));
    EXPECT_THAT(test->m_simpleEvents[1].m_contextData2, Eq(&contextData2));
}

TEST_F(WaitSet_test, WaitReturnsEventTriggersWithTwoCorrectCallbacksWithContextData)
{
    WaitReturnsEventTriggersWithTwoCorrectCallbacksWithContextData(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsEventTriggersWithTwoCorrectCallbacksWithContextData)
{
    WaitReturnsEventTriggersWithTwoCorrectCallbacksWithContextData(this, [&] { return m_sut->timedWait(10_ms); });
}

void WaitReturnsStateTriggersWithOneCorrectCallback(WaitSet_test* test,
                                                    const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    auto result1 = test->m_sut->attachState(
        test->m_simpleEvents[0], 1U, createNotificationCallback(WaitSet_test::triggerCallback1));

    ASSERT_THAT(result1.has_error(), Eq(false));

    test->m_simpleEvents[0].trigger();

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(1U));

    (*triggerVector[0U])();

    EXPECT_THAT(test->m_simpleEvents[0].m_triggerCallbackArgument1, Eq(&test->m_simpleEvents[0]));
}

TEST_F(WaitSet_test, WaitReturnsStateTriggersWithOneCorrectCallback)
{
    WaitReturnsStateTriggersWithOneCorrectCallback(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsStateTriggersWithTwoCorrectCallback)
{
    WaitReturnsStateTriggersWithOneCorrectCallback(this, [&] { return m_sut->timedWait(10_ms); });
}

void WaitReturnsStateTriggersWithTwoCorrectCallbacksWithContextData(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    uint64_t contextData1 = 0U;
    uint64_t contextData2 = 0U;
    auto result1 = test->m_sut->attachState(
        test->m_simpleEvents[0],
        1U,
        createNotificationCallback(WaitSet_test::triggerCallback1WithContextData, contextData1));
    auto result2 = test->m_sut->attachState(
        test->m_simpleEvents[1],
        2U,
        createNotificationCallback(WaitSet_test::triggerCallback2WithContextData, contextData2));

    ASSERT_THAT(result1.has_error(), Eq(false));
    ASSERT_THAT(result2.has_error(), Eq(false));

    test->m_simpleEvents[0].trigger();
    test->m_simpleEvents[1].trigger();

    auto triggerVector = waitCall();
    ASSERT_THAT(triggerVector.size(), Eq(2U));

    (*triggerVector[0U])();
    (*triggerVector[1U])();

    EXPECT_THAT(test->m_simpleEvents[0].m_triggerCallbackArgument1, Eq(&test->m_simpleEvents[0]));
    EXPECT_THAT(test->m_simpleEvents[1].m_triggerCallbackArgument2, Eq(&test->m_simpleEvents[1]));
    EXPECT_THAT(test->m_simpleEvents[0].m_contextData1, Eq(&contextData1));
    EXPECT_THAT(test->m_simpleEvents[1].m_contextData2, Eq(&contextData2));
}

TEST_F(WaitSet_test, WaitReturnsStateTriggersWithTwoCorrectCallbacksWithContextData)
{
    WaitReturnsStateTriggersWithTwoCorrectCallbacksWithContextData(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, TimedWaitReturnsStateTriggersWithTwoCorrectCallbacksWithContextData)
{
    WaitReturnsStateTriggersWithTwoCorrectCallbacksWithContextData(this, [&] { return m_sut->timedWait(10_ms); });
}

void NonResetStatesAreReturnedAgain(WaitSet_test* test,
                                    const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllStates();

    test->m_simpleEvents[2].m_autoResetTrigger = false;
    test->m_simpleEvents[2].trigger();

    test->m_simpleEvents[7].m_autoResetTrigger = false;
    test->m_simpleEvents[7].trigger();

    auto eventVector = waitCall();

    // ACT
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(2U));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 2U, test->m_simpleEvents[2]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 7U, test->m_simpleEvents[7]));
}

TEST_F(WaitSet_test, NonResetStatesAreReturnedAgainInTimedWait)
{
    NonResetStatesAreReturnedAgain(this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, NonResetStatesAreReturnedAgainInWait)
{
    NonResetStatesAreReturnedAgain(this, [&] { return m_sut->wait(); });
}

void TriggeredEventsAreNotReturnedTwice(WaitSet_test* test,
                                        const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllEvents();

    test->m_simpleEvents[2].trigger();
    test->m_simpleEvents[7].trigger();

    auto eventVector = waitCall();

    // ACT
    test->m_simpleEvents[3].trigger();
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(1U));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 3U, test->m_simpleEvents[3]));
}

TEST_F(WaitSet_test, TriggeredEventsAreNotReturnedTwiceInTimedWait)
{
    TriggeredEventsAreNotReturnedTwice(this,
                                       [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, TriggeredEventsAreNotReturnedTwiceInWait)
{
    TriggeredEventsAreNotReturnedTwice(this, [&] { return m_sut->wait(); });
}

void InMixSetupOnlyStateTriggerAreReturnedTwice(WaitSet_test* test,
                                                const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllWithEventStateMix();

    for (auto& event : test->m_simpleEvents)
    {
        event.m_autoResetTrigger = false;
        event.trigger();
    }

    auto eventVector = waitCall();

    // ACT
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET / 2U));
    for (uint64_t i = 0; i < iox::MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET; i += 2)
    {
        EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, i, test->m_simpleEvents[i]));
    }
}

TEST_F(WaitSet_test, InMixSetupOnlyStateTriggerAreReturnedTwiceInTimedWait)
{
    InMixSetupOnlyStateTriggerAreReturnedTwice(
        this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, InMixSetupOnlyStateTriggerAreReturnedTwiceInWait)
{
    InMixSetupOnlyStateTriggerAreReturnedTwice(this, [&] { return m_sut->wait(); });
}

void WhenStateIsNotResetAndEventIsTriggeredBeforeItIsReturnedAgain(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllWithEventStateMix();

    test->m_simpleEvents[2].m_autoResetTrigger = false;
    test->m_simpleEvents[2].trigger();

    auto eventVector = waitCall();

    test->m_simpleEvents[1].trigger();

    // ACT
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(2));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 1U, test->m_simpleEvents[1]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 2U, test->m_simpleEvents[2]));
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventIsTriggeredBeforeItIsReturnedAgainInTimedWait)
{
    WhenStateIsNotResetAndEventIsTriggeredBeforeItIsReturnedAgain(
        this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventIsTriggeredBeforeItIsReturnedAgainInWait)
{
    WhenStateIsNotResetAndEventIsTriggeredBeforeItIsReturnedAgain(this, [&] { return m_sut->wait(); });
}

void WhenStateIsNotResetAndEventIsTriggeredAfterItIsReturnedAgain(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllWithEventStateMix();

    test->m_simpleEvents[2].m_autoResetTrigger = false;
    test->m_simpleEvents[2].trigger();

    auto eventVector = waitCall();

    test->m_simpleEvents[3].trigger();

    // ACT
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(2));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 2U, test->m_simpleEvents[2]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 3U, test->m_simpleEvents[3]));
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventIsTriggeredAfterItIsReturnedAgainInTimedWait)
{
    WhenStateIsNotResetAndEventIsTriggeredAfterItIsReturnedAgain(
        this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventIsTriggeredAfterItIsReturnedAgainInWait)
{
    WhenStateIsNotResetAndEventIsTriggeredAfterItIsReturnedAgain(this, [&] { return m_sut->wait(); });
}

void WhenStateIsNotResetAndEventsAreTriggeredItIsReturnedAgain(
    WaitSet_test* test, const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllWithEventStateMix();

    test->m_simpleEvents[2].m_autoResetTrigger = false;
    test->m_simpleEvents[2].trigger();

    test->m_simpleEvents[7].trigger();

    test->m_simpleEvents[12].m_autoResetTrigger = false;
    test->m_simpleEvents[12].trigger();

    auto eventVector = waitCall();

    test->m_simpleEvents[1].trigger();
    test->m_simpleEvents[3].trigger();
    test->m_simpleEvents[6].trigger();
    test->m_simpleEvents[13].trigger();

    // ACT
    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(6));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 1U, test->m_simpleEvents[1]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 2U, test->m_simpleEvents[2]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 3U, test->m_simpleEvents[3]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 6U, test->m_simpleEvents[6]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 12U, test->m_simpleEvents[12]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 13U, test->m_simpleEvents[13]));
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventsAreTriggeredItIsReturnedAgainInTimedWait)
{
    WhenStateIsNotResetAndEventsAreTriggeredItIsReturnedAgain(
        this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, WhenStateIsNotResetAndEventsAreTriggeredItIsReturnedAgainInWait)
{
    WhenStateIsNotResetAndEventsAreTriggeredItIsReturnedAgain(this, [&] { return m_sut->wait(); });
}

void NotifyingWaitSetTwiceWithSameTriggersWorks(WaitSet_test* test,
                                                const std::function<WaitSet<>::NotificationInfoVector()>& waitCall)
{
    test->attachAllEvents();

    test->m_simpleEvents[2].trigger();
    test->m_simpleEvents[7].trigger();

    auto eventVector = waitCall();

    test->m_simpleEvents[2].trigger();
    test->m_simpleEvents[7].trigger();

    eventVector = waitCall();

    ASSERT_THAT(eventVector.size(), Eq(2));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 2U, test->m_simpleEvents[2]));
    EXPECT_TRUE(test->doesNotificationInfoVectorContain(eventVector, 7U, test->m_simpleEvents[7]));
}

TEST_F(WaitSet_test, NotifyingWaitSetTwiceWithSameTriggersWorksInTimedWait)
{
    NotifyingWaitSetTwiceWithSameTriggersWorks(
        this, [&] { return m_sut->timedWait(iox::units::Duration::fromMilliseconds(100)); });
}

TEST_F(WaitSet_test, NotifyingWaitSetTwiceWithSameTriggersWorksInWait)
{
    NotifyingWaitSetTwiceWithSameTriggersWorks(this, [&] { return m_sut->wait(); });
}

TEST_F(WaitSet_test, EventBasedTriggerIsReturnedOnlyOnceWhenItsTriggered)
{
    m_simpleEvents[0].m_isEventBased = true;
    m_simpleEvents[0].m_autoResetTrigger = false;

    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], 3431).has_error());

    m_simpleEvents[0].trigger();

    auto eventVector = m_sut->wait();
    ASSERT_THAT(eventVector.size(), Eq(1));
    EXPECT_TRUE(doesNotificationInfoVectorContain(eventVector, 3431, m_simpleEvents[0]));

    eventVector = m_sut->timedWait(iox::units::Duration::fromMilliseconds(1));
    EXPECT_TRUE(eventVector.empty());
}

TEST_F(WaitSet_test, MixingEventAndStateBasedTriggerHandlesEventTriggeresWithWaitCorrectly)
{
    m_simpleEvents[0].m_autoResetTrigger = false;
    m_simpleEvents[1].m_autoResetTrigger = false;

    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0], 3431).has_error());
    ASSERT_FALSE(m_sut->attachState(m_simpleEvents[1], 8171).has_error());

    m_simpleEvents[0].trigger();
    m_simpleEvents[1].trigger();

    auto eventVector = m_sut->wait();
    ASSERT_THAT(eventVector.size(), Eq(2));
    EXPECT_TRUE(doesNotificationInfoVectorContain(eventVector, 3431, m_simpleEvents[0]));
    EXPECT_TRUE(doesNotificationInfoVectorContain(eventVector, 8171, m_simpleEvents[1]));

    eventVector = m_sut->timedWait(iox::units::Duration::fromMilliseconds(1));
    ASSERT_THAT(eventVector.size(), Eq(1));
    EXPECT_TRUE(doesNotificationInfoVectorContain(eventVector, 8171, m_simpleEvents[1]));
}

TEST_F(WaitSet_test, WaitUnblocksAfterMarkForDestructionCall)
{
    std::atomic_bool doStartWaiting{false};
    std::atomic_bool isThreadFinished{false};
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0U], 0U).has_error());

    std::thread t([&] {
        doStartWaiting.store(true);
        auto triggerVector = m_sut->wait();
        triggerVector = m_sut->wait();
        triggerVector = m_sut->wait();
        isThreadFinished.store(true);
    });

    while (!doStartWaiting.load())
        ;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(isThreadFinished.load());

    m_sut->markForDestruction();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(isThreadFinished.load());

    t.join();
}

TEST_F(WaitSet_test, TimedWaitUnblocksAfterMarkForDestructionCall)
{
    std::atomic_bool doStartWaiting{false};
    std::atomic_bool isThreadFinished{false};
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0U], 0U).has_error());

    std::thread t([&] {
        doStartWaiting.store(true);
        auto triggerVector = m_sut->timedWait(iox::units::Duration::fromSeconds(1337));
        triggerVector = m_sut->timedWait(iox::units::Duration::fromSeconds(1337));
        triggerVector = m_sut->timedWait(iox::units::Duration::fromSeconds(1337));
        isThreadFinished.store(true);
    });

    while (!doStartWaiting.load())
        ;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(isThreadFinished.load());

    m_sut->markForDestruction();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(isThreadFinished.load());

    t.join();
}

