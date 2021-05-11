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


#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/concurrent/smart_lock.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"
#include "test.hpp"

#include <array>
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
enum SimpleEvent : iox::popo::EventEnumIdentifier
{
    StoepselBachelorParty,
    Hypnotoad
};

uint64_t g_invalidateTriggerId = 0U;
class SimpleEventClass
{
  public:
    SimpleEventClass() = default;
    SimpleEventClass(const SimpleEventClass&) = delete;
    SimpleEventClass(SimpleEventClass&&) = delete;

    SimpleEventClass& operator=(const SimpleEventClass&) = delete;
    SimpleEventClass& operator=(SimpleEventClass&&) = delete;

    ~SimpleEventClass()
    {
        m_handleStoepsel.reset();
        m_handleHypnotoad.reset();
    }

    void enableEvent(iox::popo::TriggerHandle&& handle, const SimpleEvent event) noexcept
    {
        switch (event)
        {
        case SimpleEvent::StoepselBachelorParty:
            m_handleStoepsel = std::move(handle);
            break;
        case SimpleEvent::Hypnotoad:
            m_handleHypnotoad = std::move(handle);
            break;
        }
    }

    void enableEvent(iox::popo::TriggerHandle&& handle) noexcept
    {
        m_handleNoEventEnum = std::move(handle);
    }

    void invalidateTrigger(const uint64_t id) noexcept
    {
        g_invalidateTriggerId = id;
        if (m_handleHypnotoad.getUniqueId() == id)
        {
            m_handleHypnotoad.invalidate();
        }
        else if (m_handleStoepsel.getUniqueId() == id)
        {
            m_handleStoepsel.invalidate();
        }
        else if (m_handleNoEventEnum.getUniqueId() == id)
        {
            m_handleNoEventEnum.invalidate();
        }
    }

    void disableEvent(const SimpleEvent event) noexcept
    {
        switch (event)
        {
        case SimpleEvent::StoepselBachelorParty:
            m_handleStoepsel.reset();
            break;
        case SimpleEvent::Hypnotoad:
            m_handleHypnotoad.reset();
            break;
        }
    }

    void disableEvent() noexcept
    {
        m_handleNoEventEnum.reset();
    }
    void triggerStoepsel() noexcept
    {
        m_handleStoepsel.trigger();
    }
    void triggerNoEventType() noexcept
    {
        m_handleNoEventEnum.trigger();
    }

    iox::popo::TriggerHandle m_handleHypnotoad;
    iox::popo::TriggerHandle m_handleStoepsel;
    iox::popo::TriggerHandle m_handleNoEventEnum;
};

class TestListener : public Listener
{
  public:
    TestListener(ConditionVariableData& data) noexcept
        : Listener(data)
    {
    }
};

struct EventAndSutPair_t
{
    SimpleEventClass* object;
    TestListener* sut;
};

struct TriggerSourceAndCount
{
    std::atomic<SimpleEventClass*> m_source{nullptr};
    std::atomic<uint64_t> m_count{0U};
};

iox::concurrent::smart_lock<std::vector<EventAndSutPair_t>> g_toBeAttached;
iox::concurrent::smart_lock<std::vector<EventAndSutPair_t>> g_toBeDetached;
std::array<TriggerSourceAndCount, iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER> g_triggerCallbackArg;
uint64_t g_triggerCallbackRuntimeInMs = 0U;
iox::cxx::optional<iox::posix::Semaphore> g_callbackBlocker;

class Listener_test : public Test
{
  public:
    template <uint64_t N>
    static void triggerCallback(SimpleEventClass* const event) noexcept
    {
        g_triggerCallbackArg[N].m_source = event;
        ++g_triggerCallbackArg[N].m_count;

        if (g_callbackBlocker)
        {
            IOX_DISCARD_RESULT(g_callbackBlocker->wait());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(g_triggerCallbackRuntimeInMs));
    }

    static void triggerCallbackWithUserType(SimpleEventClass* const event, uint64_t* userType) noexcept
    {
        g_triggerCallbackArg[0].m_source = event;
        ++(*userType);
    }

    static void attachCallback(SimpleEventClass* const) noexcept
    {
        for (auto& e : g_toBeAttached.getCopy())
        {
            ASSERT_FALSE(e.sut
                             ->attachEvent(*e.object,
                                           SimpleEvent::StoepselBachelorParty,
                                           createNotificationCallback(triggerCallback<0U>))
                             .has_error());
        }
    }

    static void detachCallback(SimpleEventClass* const) noexcept
    {
        for (auto& e : g_toBeDetached.getCopy())
        {
            e.sut->detachEvent(*e.object, SimpleEvent::StoepselBachelorParty);
        }
    }

    static void notifyAndThenDetachStoepselCallback(SimpleEventClass* const) noexcept
    {
        for (auto& e : g_toBeDetached.getCopy())
        {
            e.object->triggerStoepsel();
            e.sut->detachEvent(*e.object, SimpleEvent::StoepselBachelorParty);
        }
    }


    void SetUp()
    {
        g_callbackBlocker.reset();
        for (auto& e : g_triggerCallbackArg)
        {
            e.m_source = nullptr;
            e.m_count = 0U;
        }
        m_sut.emplace(m_condVarData);
        g_invalidateTriggerId = 0U;
        g_triggerCallbackRuntimeInMs = 0U;
        g_toBeAttached->clear();
        g_toBeDetached->clear();
    };

    void activateTriggerCallbackBlocker() noexcept
    {
        g_callbackBlocker.emplace(
            iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value());
    }

    void unblockTriggerCallback(const uint64_t numberOfUnblocks) noexcept
    {
        for (uint64_t i = 0U; i < numberOfUnblocks; ++i)
        {
            ASSERT_TRUE(static_cast<bool>(g_callbackBlocker));
            IOX_DISCARD_RESULT(g_callbackBlocker->post());
        }
    }

    void fillUpWithSimpleEvents()
    {
        for (uint64_t i = 0U; i < m_sut->capacity(); ++i)
        {
            EXPECT_FALSE(
                m_sut->attachEvent(m_simpleEvents[i], createNotificationCallback(Listener_test::triggerCallback<0U>))
                    .has_error());
            EXPECT_THAT(m_sut->size(), Eq(i + 1U));
        }
    }
    bool fillUpWithSimpleEventsWithEnum(const SimpleEvent eventType)
    {
        for (uint64_t i = 0U; i < m_sut->capacity(); ++i)
        {
            bool hasError = m_sut
                                ->attachEvent(m_simpleEvents[i],
                                              eventType,
                                              createNotificationCallback(Listener_test::triggerCallback<0U>))
                                .has_error();
            EXPECT_FALSE(hasError);
            if (hasError)
            {
                return false;
            }

            EXPECT_THAT(m_sut->size(), Eq(i + 1U));
        }
        return true;
    }


    void TearDown(){};


    static constexpr uint64_t OVERFLOW_TEST_APPENDIX = 1U;
    using eventArray_t = SimpleEventClass[iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER + OVERFLOW_TEST_APPENDIX];
    eventArray_t m_simpleEvents;
    ConditionVariableData m_condVarData{"Maulbeerblättle"};
    iox::cxx::optional<TestListener> m_sut;

    const iox::units::Duration m_fatalTimeout = 2_s;
    Watchdog m_watchdog{m_fatalTimeout};
    static constexpr uint64_t CALLBACK_WAIT_IN_MS = 100U;
};

constexpr uint64_t Listener_test::CALLBACK_WAIT_IN_MS;
constexpr uint64_t Listener_test::OVERFLOW_TEST_APPENDIX;

template <uint64_t N>
struct AttachEvent
{
    template <typename EventType>
    static void doIt(TestListener& sut, std::vector<SimpleEventClass>& events, const EventType event)
    {
        EXPECT_THAT(sut.attachEvent(events[N], event, createNotificationCallback(Listener_test::triggerCallback<N>))
                        .has_error(),
                    Eq(false));
        AttachEvent<N - 1U>::doIt(sut, events, event);
    }
};

template <>
struct AttachEvent<0U>
{
    template <typename EventType>
    static void doIt(TestListener& sut, std::vector<SimpleEventClass>& events, const EventType event)
    {
        EXPECT_THAT(sut.attachEvent(events[0U], event, createNotificationCallback(Listener_test::triggerCallback<0U>))
                        .has_error(),
                    Eq(false));
    }
};
} // namespace


//////////////////////////////////
// BEGIN attach / detach
//////////////////////////////////
TEST_F(Listener_test, CapacityIsEqualToMAX_NUMBER_OF_EVENTS_PER_LISTENER)
{
    EXPECT_THAT(m_sut->capacity(), Eq(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER));
}

TEST_F(Listener_test, IsEmptyWhenConstructed)
{
    EXPECT_THAT(m_sut->size(), Eq(0U));
}

TEST_F(Listener_test, AttachingWithoutEnumIfEnoughSpaceAvailableWorks)
{
    EXPECT_FALSE(m_sut->attachEvent(m_simpleEvents[0U], createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    EXPECT_THAT(m_sut->size(), Eq(1U));
}

TEST_F(Listener_test, AttachWithoutEnumTillCapacityIsFullWorks)
{
    fillUpWithSimpleEvents();
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity()));
}

TEST_F(Listener_test, DetachDecreasesSize)
{
    fillUpWithSimpleEvents();
    m_sut->detachEvent(m_simpleEvents[0U]);
    EXPECT_THAT(m_sut->size(), Eq(m_sut->capacity() - 1U));
}

TEST_F(Listener_test, AttachWithoutEnumOneMoreThanCapacityFails)
{
    fillUpWithSimpleEvents();
    auto result = m_sut->attachEvent(m_simpleEvents[m_sut->capacity()],
                                     createNotificationCallback(Listener_test::triggerCallback<0U>));

    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(ListenerError::LISTENER_FULL));
}

TEST_F(Listener_test, AttachingWithEnumIfEnoughSpaceAvailableWorks)
{
    EXPECT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::Hypnotoad,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    EXPECT_THAT(m_sut->size(), Eq(1U));
}

TEST_F(Listener_test, AttachWithEnumTillCapacityIsFullWorks)
{
    EXPECT_TRUE(fillUpWithSimpleEventsWithEnum(SimpleEvent::Hypnotoad));
}

TEST_F(Listener_test, AttachWithEnumOneMoreThanCapacityFails)
{
    fillUpWithSimpleEventsWithEnum(SimpleEvent::Hypnotoad);
    auto result = m_sut->attachEvent(m_simpleEvents[m_sut->capacity()],
                                     SimpleEvent::Hypnotoad,
                                     createNotificationCallback(Listener_test::triggerCallback<0U>));

    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(ListenerError::LISTENER_FULL));
}

TEST_F(Listener_test, DetachMakesSpaceForAnotherAttachWithEventEnum)
{
    fillUpWithSimpleEventsWithEnum(SimpleEvent::Hypnotoad);

    m_sut->detachEvent(m_simpleEvents[0U], SimpleEvent::Hypnotoad);
    EXPECT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[m_sut->capacity()],
                                   SimpleEvent::Hypnotoad,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
}

TEST_F(Listener_test, DetachMakesSpaceForAnotherAttachWithoutEventEnum)
{
    fillUpWithSimpleEvents();

    m_sut->detachEvent(m_simpleEvents[0U]);
    EXPECT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[m_sut->capacity()],
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
}

TEST_F(Listener_test, AttachingEventWithoutEventTypeLeadsToAttachedNoEventEnumTriggerHandle)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0U], createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    EXPECT_TRUE(m_simpleEvents[0U].m_handleNoEventEnum.isValid());
}

TEST_F(Listener_test, AttachingEventWithEventTypeLeadsToAttachedTriggerHandle)
{
    ASSERT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    EXPECT_TRUE(m_simpleEvents[0U].m_handleStoepsel.isValid());
}

TEST_F(Listener_test, OverridingAlreadyAttachedEventWithEnumFails)
{
    ASSERT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    auto result = m_sut->attachEvent(m_simpleEvents[0U],
                                     SimpleEvent::StoepselBachelorParty,
                                     createNotificationCallback(Listener_test::triggerCallback<0U>));
    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(ListenerError::EVENT_ALREADY_ATTACHED));
}

TEST_F(Listener_test, OverridingAlreadyAttachedEventWithoutEnumFails)
{
    ASSERT_FALSE(m_sut->attachEvent(m_simpleEvents[0U], createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    auto result =
        m_sut->attachEvent(m_simpleEvents[0U], createNotificationCallback(Listener_test::triggerCallback<0U>));
    ASSERT_TRUE(result.has_error());
    EXPECT_THAT(result.get_error(), Eq(ListenerError::EVENT_ALREADY_ATTACHED));
}

TEST_F(Listener_test, AttachingSameClassWithTwoDifferentEventsWorks)
{
    ASSERT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::Hypnotoad,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    EXPECT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
}

TEST_F(Listener_test, DetachingSameClassWithDifferentEventEnumChangesNothing)
{
    ASSERT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::Hypnotoad,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    m_sut->detachEvent(m_simpleEvents[0U], SimpleEvent::StoepselBachelorParty);
    EXPECT_THAT(m_sut->size(), Eq(1U));
}

TEST_F(Listener_test, DetachingDifferentClassWithSameEventEnumChangesNothing)
{
    ASSERT_FALSE(m_sut
                     ->attachEvent(m_simpleEvents[0U],
                                   SimpleEvent::Hypnotoad,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    m_sut->detachEvent(m_simpleEvents[1U], SimpleEvent::Hypnotoad);
    EXPECT_THAT(m_sut->size(), Eq(1U));
}

TEST_F(Listener_test, AttachingWithoutEnumTillCapacityFilledSetsUpNoEventEnumTriggerHandle)
{
    fillUpWithSimpleEvents();

    for (uint64_t i = 0U; i < m_sut->capacity(); ++i)
    {
        EXPECT_TRUE(m_simpleEvents[i].m_handleNoEventEnum.isValid());
    }
}

TEST_F(Listener_test, DTorDetachesAllAttachedEventsWithoutEnum)
{
    fillUpWithSimpleEvents();

    auto capacity = m_sut->capacity();
    m_sut.reset();

    for (uint64_t i = 0U; i < capacity; ++i)
    {
        EXPECT_FALSE(m_simpleEvents[i].m_handleNoEventEnum.isValid());
    }
}

TEST_F(Listener_test, DTorDetachesAllAttachedEventsWithEnum)
{
    fillUpWithSimpleEventsWithEnum(SimpleEvent::Hypnotoad);

    auto capacity = m_sut->capacity();
    m_sut.reset();

    for (uint64_t i = 0U; i < capacity; ++i)
    {
        EXPECT_FALSE(m_simpleEvents[i].m_handleHypnotoad.isValid());
    }
}

TEST_F(Listener_test, AttachedEventDTorDetachesItself)
{
    {
        SimpleEventClass fuu;
        ASSERT_FALSE(
            m_sut->attachEvent(fuu, createNotificationCallback(Listener_test::triggerCallback<0U>)).has_error());
    }

    EXPECT_THAT(m_sut->size(), Eq(0U));
}

TEST_F(Listener_test, AttachingSimpleEventWithoutEnumSetsNoEventEnumTriggerHandle)
{
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut->attachEvent(fuu, createNotificationCallback(Listener_test::triggerCallback<0U>)).has_error());

    EXPECT_TRUE(static_cast<bool>(fuu.m_handleNoEventEnum));
}

TEST_F(Listener_test, DetachingSimpleEventResetsTriggerHandle)
{
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut->attachEvent(fuu, createNotificationCallback(Listener_test::triggerCallback<0U>)).has_error());
    m_sut->detachEvent(fuu);

    EXPECT_FALSE(static_cast<bool>(fuu.m_handleNoEventEnum));
}

TEST_F(Listener_test, AttachingEventWithEnumSetsTriggerHandle)
{
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    EXPECT_TRUE(static_cast<bool>(fuu.m_handleStoepsel));
}

TEST_F(Listener_test, DetachingEventWithEnumResetsTriggerHandle)
{
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    m_sut->detachEvent(fuu, SimpleEvent::StoepselBachelorParty);

    EXPECT_FALSE(static_cast<bool>(fuu.m_handleStoepsel));
}

TEST_F(Listener_test, DetachingNonAttachedEventResetsNothing)
{
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    m_sut->detachEvent(fuu, SimpleEvent::Hypnotoad);

    EXPECT_TRUE(static_cast<bool>(fuu.m_handleStoepsel));
}

///////////////////////////////////
// END
///////////////////////////////////

///////////////////////////////////
// BEGIN calling callbacks
///////////////////////////////////
TIMING_TEST_F(Listener_test, CallbackIsCalledAfterNotify, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, CallbackWithEventAndUserTypeIsCalledAfterNotify, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    uint64_t userType = 0U;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallbackWithUserType, userType))
                     .has_error());

    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(userType == 1U);
});

TIMING_TEST_F(Listener_test, CallbackWithUserTypeIsCalledAfterNotify, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    uint64_t userType = 0U;
    ASSERT_FALSE(
        m_sut->attachEvent(fuu, createNotificationCallback(Listener_test::triggerCallbackWithUserType, userType))
            .has_error());

    fuu.triggerNoEventType();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(userType == 1U);
});

TIMING_TEST_F(Listener_test, CallbackIsCalledOnlyOnceWhenTriggered, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu1;
    SimpleEventClass fuu2;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu1,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu2,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<1U>))
                     .has_error());

    fuu1.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));
    fuu2.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu1);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 1U);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_source == &fuu2);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, TriggerWhileInCallbackLeadsToAnotherCallback, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    constexpr uint64_t NUMBER_OF_TRIGGER_UNBLOCKS = 10U;

    activateTriggerCallbackBlocker();
    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    fuu.triggerStoepsel();
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(NUMBER_OF_TRIGGER_UNBLOCKS);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 2U);
});

TIMING_TEST_F(Listener_test, TriggerWhileInCallbackLeadsToAnotherCallbackOnce, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    SimpleEventClass bar;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    ASSERT_FALSE(m_sut
                     ->attachEvent(bar,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<1U>))
                     .has_error());

    constexpr uint64_t NUMBER_OF_TRIGGER_UNBLOCKS = 10U;

    activateTriggerCallbackBlocker();
    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    fuu.triggerStoepsel();
    bar.triggerStoepsel();
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(NUMBER_OF_TRIGGER_UNBLOCKS);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 2U);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_source == &bar);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, TriggerMultipleTimesWhileInCallbackLeadsToAnotherCallback, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    constexpr uint64_t NUMBER_OF_RETRIGGERS = 10U;

    activateTriggerCallbackBlocker();
    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    for (uint64_t i = 0U; i < NUMBER_OF_RETRIGGERS; ++i)
    {
        fuu.triggerStoepsel();
    }
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(NUMBER_OF_RETRIGGERS);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 2U);
});

TIMING_TEST_F(Listener_test, TriggerMultipleTimesWhileInCallbackLeadsToAnotherCallbackOnce, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    SimpleEventClass bar;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());
    ASSERT_FALSE(m_sut
                     ->attachEvent(bar,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<1U>))
                     .has_error());

    constexpr uint64_t NUMBER_OF_RETRIGGERS = 10U;

    activateTriggerCallbackBlocker();
    fuu.triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    for (uint64_t i = 0U; i < NUMBER_OF_RETRIGGERS; ++i)
    {
        fuu.triggerStoepsel();
    }
    bar.triggerStoepsel();
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(NUMBER_OF_RETRIGGERS + 1U);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &fuu);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 2U);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_source == &bar);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, NoTriggerLeadsToNoCallback, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass fuu;
    ASSERT_FALSE(m_sut
                     ->attachEvent(fuu,
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(Listener_test::triggerCallback<0U>))
                     .has_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == nullptr);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 0U);
});

TIMING_TEST_F(Listener_test, TriggeringAllEventsCallsAllCallbacks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);

    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);

    activateTriggerCallbackBlocker();
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    for (auto& e : events)
    {
        e.triggerStoepsel();
    }

    // 10 times more callback runs allowed to allow potential overtriggering
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(10U * iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0].m_source == &events[0U]);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0].m_count == 2U);
    for (uint64_t i = 1U; i < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_source == &events[i]);
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_count == 1U);
    }
});

TIMING_TEST_F(Listener_test, TriggeringAllEventsCallsAllCallbacksOnce, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);

    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);

    activateTriggerCallbackBlocker();
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    for (auto& e : events)
    {
        e.triggerStoepsel();
    }

    // 10 times more callback runs allowed to allow potential overtriggering
    m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    unblockTriggerCallback(10U * iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0].m_source == &events[0U]);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0].m_count == 3U);
    for (uint64_t i = 1U; i < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_source == &events[i]);
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_count == 1U);
    }
});
//////////////////////////////////
// END
//////////////////////////////////

//////////////////////////////////
// BEGIN concurrent attach / detach
//////////////////////////////////
TIMING_TEST_F(Listener_test, AttachingWhileCallbackIsRunningWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);

    ASSERT_FALSE(m_sut
                     ->attachEvent(events[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<0U>))
                     .has_error());

    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    ASSERT_FALSE(m_sut
                     ->attachEvent(events[1U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<1U>))
                     .has_error());
    events[1U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS * 2U));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_source == &events[1U]);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, AttachingMultipleWhileCallbackIsRunningWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);

    ASSERT_FALSE(
        m_sut
            ->attachEvent(events[iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U],
                          SimpleEvent::StoepselBachelorParty,
                          createNotificationCallback(triggerCallback<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U>))
            .has_error());

    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 2U>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);

    g_triggerCallbackRuntimeInMs = 0U;
    for (uint64_t i = 0U; i + 1U < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        events[i].triggerStoepsel();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 2U));

    for (uint64_t i = 0U; i + 1U < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_source == &events[i]);
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_count == 1U);
    }
});

TIMING_TEST_F(Listener_test, DetachingWhileCallbackIsRunningWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);

    ASSERT_FALSE(m_sut
                     ->attachEvent(events[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<0U>))
                     .has_error());

    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    g_triggerCallbackArg[0U].m_source = nullptr;
    m_sut->detachEvent(events[0U], SimpleEvent::StoepselBachelorParty);
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 1U);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == nullptr);
});

TIMING_TEST_F(Listener_test, DetachingWhileCallbackIsRunningBlocksDetach, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    ASSERT_FALSE(m_sut
                     ->attachEvent(events[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<0U>))
                     .has_error());
    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 4U));

    auto begin = std::chrono::system_clock::now();
    m_sut->detachEvent(events[0U], SimpleEvent::StoepselBachelorParty);
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    TIMING_TEST_EXPECT_TRUE(static_cast<uint64_t>(elapsed.count()) > CALLBACK_WAIT_IN_MS / 2U);
});

TIMING_TEST_F(Listener_test, EventDestructorBlocksWhenCallbackIsRunning, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    SimpleEventClass* event = new SimpleEventClass();
    ASSERT_FALSE(
        m_sut->attachEvent(*event, SimpleEvent::StoepselBachelorParty, createNotificationCallback(triggerCallback<0U>))
            .has_error());
    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    event->triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 4U));

    auto begin = std::chrono::system_clock::now();
    delete event;
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    TIMING_TEST_EXPECT_TRUE(static_cast<uint64_t>(elapsed.count()) > CALLBACK_WAIT_IN_MS / 2U);
});


TIMING_TEST_F(Listener_test, DetachingMultipleWhileCallbackIsRunningWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);

    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    g_triggerCallbackRuntimeInMs = 0U;
    for (auto& e : events)
    {
        m_sut->detachEvent(e, SimpleEvent::StoepselBachelorParty);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));
    for (auto& t : g_triggerCallbackArg)
    {
        t.m_source = nullptr;
    }
    for (auto& e : events)
    {
        e.triggerStoepsel();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    for (uint64_t i = 0U; i < iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER; ++i)
    {
        TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[i].m_source == nullptr);
    }
});

TIMING_TEST_F(Listener_test, AttachingDetachingRunsIndependentOfCallback, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    ASSERT_FALSE(
        m_sut
            ->attachEvent(events[iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U],
                          SimpleEvent::StoepselBachelorParty,
                          createNotificationCallback(triggerCallback<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U>))
            .has_error());
    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 1U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 2U));

    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 2U>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);

    for (auto& e : events)
    {
        m_sut->detachEvent(e, SimpleEvent::StoepselBachelorParty);
    }

    // EXPECT_* (assert step) is inside of doIt call. We expect that every event can
    // be attached
    AttachEvent<iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER - 2U>::doIt(*m_sut, events, SimpleEvent::StoepselBachelorParty);
});
//////////////////////////////////
// END
//////////////////////////////////

//////////////////////////////////
// BEGIN attach / detach in callbacks
//////////////////////////////////
TIMING_TEST_F(Listener_test, DetachingSelfInCallbackWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    g_toBeDetached->clear();

    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    g_toBeDetached->push_back({&events[0U], &*m_sut});
    ASSERT_FALSE(
        m_sut->attachEvent(events[0U], SimpleEvent::StoepselBachelorParty, createNotificationCallback(detachCallback))
            .has_error());

    events[0U].triggerStoepsel();

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(m_sut->size() == 0U);
});

TIMING_TEST_F(Listener_test, DetachingNonSelfEventInCallbackWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    g_toBeDetached->clear();

    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    g_toBeDetached->push_back({&events[1U], &*m_sut});
    ASSERT_FALSE(
        m_sut->attachEvent(events[0U], SimpleEvent::StoepselBachelorParty, createNotificationCallback(detachCallback))
            .has_error());
    ASSERT_FALSE(m_sut
                     ->attachEvent(events[1U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<1U>))
                     .has_error());

    events[0U].triggerStoepsel();

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(m_sut->size() == 1U);
});

TIMING_TEST_F(Listener_test, DetachedCallbacksAreNotBeingCalledWhenTriggeredBefore, Repeat(5), [&] {
    // idea of the test is that an event which was detached but is technically still attached
    // since the detach blocks cannot be retriggered again so that the callback is called again.
    // once detach is called either the callback is currently running and detach is blocked or
    // the callback is removed and can never be called again.
    //
    // To test this we attach two events. events[0] detaches events[1] in his callback.
    // events[1] is triggered and the callback has a certain runtime so that we make sure that the callback is
    // running while we retrigger events[0] and events[1].
    // Now events[0] remove events[1] before its trigger callback is executed and therefore the
    // callback is not allowed to be called even so that the trigger came before the detach occurred
    m_sut.emplace(m_condVarData);
    g_toBeDetached->clear();

    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    g_toBeDetached->push_back({&events[1U], &*m_sut});
    ASSERT_FALSE(m_sut
                     ->attachEvent(events[0U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(notifyAndThenDetachStoepselCallback))
                     .has_error());
    ASSERT_FALSE(m_sut
                     ->attachEvent(events[1U],
                                   SimpleEvent::StoepselBachelorParty,
                                   createNotificationCallback(triggerCallback<1U>))
                     .has_error());

    g_triggerCallbackRuntimeInMs = 3U * CALLBACK_WAIT_IN_MS / 2U;
    events[1U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 2U));
    g_triggerCallbackArg[1U].m_source = nullptr;
    g_triggerCallbackRuntimeInMs = 0U;

    events[1U].triggerStoepsel();
    events[0U].triggerStoepsel();

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == nullptr);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_source == nullptr);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 0U);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[1U].m_count == 1U);
});

TIMING_TEST_F(Listener_test, AttachingInCallbackWorks, Repeat(5), [&] {
    m_sut.emplace(m_condVarData);
    g_toBeAttached->clear();

    std::vector<SimpleEventClass> events(iox::MAX_NUMBER_OF_EVENTS_PER_LISTENER);
    g_toBeAttached->push_back({&events[1U], &*m_sut});
    ASSERT_FALSE(
        m_sut->attachEvent(events[0U], SimpleEvent::StoepselBachelorParty, createNotificationCallback(attachCallback))
            .has_error());

    events[0U].triggerStoepsel();
    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 2U));
    events[1U].triggerStoepsel();

    std::this_thread::sleep_for(std::chrono::milliseconds(CALLBACK_WAIT_IN_MS / 2U));

    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_source == &events[1U]);
    TIMING_TEST_EXPECT_TRUE(g_triggerCallbackArg[0U].m_count == 1U);
});
//////////////////////////////////
// END
//////////////////////////////////

