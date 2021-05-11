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

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;

template <typename CPP, typename C>
struct EnumMapping
{
    CPP cpp;
    C c;
};

TEST(cpp2c_enum_translation_test, SubscribeStateCorrectAndFullTranslation)
{
    constexpr EnumMapping<iox::SubscribeState, iox_SubscribeState> SUBSCRIBE_STATES[]{
        {iox::SubscribeState::NOT_SUBSCRIBED, SubscribeState_NOT_SUBSCRIBED},
        {iox::SubscribeState::SUBSCRIBE_REQUESTED, SubscribeState_SUBSCRIBE_REQUESTED},
        {iox::SubscribeState::SUBSCRIBED, SubscribeState_SUBSCRIBED},
        {iox::SubscribeState::UNSUBSCRIBE_REQUESTED, SubscribeState_UNSUBSCRIBE_REQUESTED},
        {iox::SubscribeState::WAIT_FOR_OFFER, SubscribeState_WAIT_FOR_OFFER}};

    for (const auto subscribeState : SUBSCRIBE_STATES)
    {
        switch (subscribeState.cpp)
        {
        case iox::SubscribeState::NOT_SUBSCRIBED:
            EXPECT_EQ(cpp2c::subscribeState(subscribeState.cpp), subscribeState.c);
            break;
        case iox::SubscribeState::SUBSCRIBE_REQUESTED:
            EXPECT_EQ(cpp2c::subscribeState(subscribeState.cpp), subscribeState.c);
            break;
        case iox::SubscribeState::SUBSCRIBED:
            EXPECT_EQ(cpp2c::subscribeState(subscribeState.cpp), subscribeState.c);
            break;
        case iox::SubscribeState::UNSUBSCRIBE_REQUESTED:
            EXPECT_EQ(cpp2c::subscribeState(subscribeState.cpp), subscribeState.c);
            break;
        case iox::SubscribeState::WAIT_FOR_OFFER:
            EXPECT_EQ(cpp2c::subscribeState(subscribeState.cpp), subscribeState.c);
            break;
            // default intentionally left out in order to get a compiler warning if the enum gets extended and we forgot
            // to extend the test
        }
    }

    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::subscribeState(static_cast<iox::SubscribeState>(-1)), SubscribeState_UNDEFINED_ERROR);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, ChunkReceiveResult)
{
    constexpr EnumMapping<iox::popo::ChunkReceiveResult, iox_ChunkReceiveResult> CHUNK_RECEIVE_RESULTS[]{
        {iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE, ChunkReceiveResult_NO_CHUNK_AVAILABLE},
        {iox::popo::ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL,
         ChunkReceiveResult_TOO_MANY_CHUNKS_HELD_IN_PARALLEL},
        {iox::popo::ChunkReceiveResult::INVALID_STATE, ChunkReceiveResult_UNDEFINED_ERROR}};

    for (const auto chunkReceiveResult : CHUNK_RECEIVE_RESULTS)
    {
        switch (chunkReceiveResult.cpp)
        {
        case iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE:
            EXPECT_EQ(cpp2c::chunkReceiveResult(chunkReceiveResult.cpp), chunkReceiveResult.c);
            break;
        case iox::popo::ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL:
            EXPECT_EQ(cpp2c::chunkReceiveResult(chunkReceiveResult.cpp), chunkReceiveResult.c);
            break;
        case iox::popo::ChunkReceiveResult::INVALID_STATE:
            EXPECT_EQ(cpp2c::chunkReceiveResult(chunkReceiveResult.cpp), chunkReceiveResult.c);
            break;
            // default intentionally left out in order to get a compiler warning if the enum gets extended and we forgot
            // to extend the test
        }
    }

    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::chunkReceiveResult(static_cast<iox::popo::ChunkReceiveResult>(-1)),
              ChunkReceiveResult_UNDEFINED_ERROR);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, AllocationResult)
{
    constexpr EnumMapping<iox::popo::AllocationError, iox_AllocationResult> ALLOCATION_ERRORS[]{
        {iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS, AllocationResult_RUNNING_OUT_OF_CHUNKS},
        {iox::popo::AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL,
         AllocationResult_TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL},
        {iox::popo::AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER,
         AllocationResult_INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER},
        {iox::popo::AllocationError::INVALID_STATE, AllocationResult_UNDEFINED_ERROR}};

    for (const auto allocationError : ALLOCATION_ERRORS)
    {
        switch (allocationError.cpp)
        {
        case iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS:
            EXPECT_EQ(cpp2c::allocationResult(allocationError.cpp), allocationError.c);
            break;
        case iox::popo::AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL:
            EXPECT_EQ(cpp2c::allocationResult(allocationError.cpp), allocationError.c);
            break;
        case iox::popo::AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER:
            EXPECT_EQ(cpp2c::allocationResult(allocationError.cpp), allocationError.c);
            break;
        case iox::popo::AllocationError::INVALID_STATE:
            EXPECT_EQ(cpp2c::allocationResult(allocationError.cpp), allocationError.c);
            break;
            break;
            // default intentionally left out in order to get a compiler warning if the enum gets extended and we forgot
            // to extend the test
        }
    }

    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::allocationResult(static_cast<iox::popo::AllocationError>(-1)), AllocationResult_UNDEFINED_ERROR);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, WaitSetResult)
{
    constexpr EnumMapping<iox::popo::WaitSetError, iox_WaitSetResult> WAIT_SET_ERRORS[]{
        {iox::popo::WaitSetError::WAIT_SET_FULL, WaitSetResult_WAIT_SET_FULL},
        {iox::popo::WaitSetError::ALREADY_ATTACHED, WaitSetResult_ALREADY_ATTACHED},
        {iox::popo::WaitSetError::INVALID_STATE, WaitSetResult_UNDEFINED_ERROR}};

    for (const auto waitSetError : WAIT_SET_ERRORS)
    {
        switch (waitSetError.cpp)
        {
        case iox::popo::WaitSetError::WAIT_SET_FULL:
            EXPECT_EQ(cpp2c::waitSetResult(waitSetError.cpp), waitSetError.c);
            break;
        case iox::popo::WaitSetError::ALREADY_ATTACHED:
            EXPECT_EQ(cpp2c::waitSetResult(waitSetError.cpp), waitSetError.c);
            break;
        case iox::popo::WaitSetError::INVALID_STATE:
            EXPECT_EQ(cpp2c::waitSetResult(waitSetError.cpp), waitSetError.c);
            break;
            // default intentionally left out in order to get a compiler warning if the enum gets extended and we forgot
            // to extend the test
        }
    }

    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::waitSetResult(static_cast<iox::popo::WaitSetError>(-1)), WaitSetResult_UNDEFINED_ERROR);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, ListenerResult)
{
    constexpr EnumMapping<iox::popo::ListenerError, iox_ListenerResult> LISTENER_ERRORS[]{
        {iox::popo::ListenerError::LISTENER_FULL, ListenerResult_LISTENER_FULL},
        {iox::popo::ListenerError::EVENT_ALREADY_ATTACHED, ListenerResult_EVENT_ALREADY_ATTACHED},
        {iox::popo::ListenerError::EMPTY_INVALIDATION_CALLBACK, ListenerResult_EMPTY_INVALIDATION_CALLBACK},
        {iox::popo::ListenerError::INVALID_STATE, ListenerResult_UNDEFINED_ERROR}};

    for (const auto listenerError : LISTENER_ERRORS)
    {
        switch (listenerError.cpp)
        {
        case iox::popo::ListenerError::LISTENER_FULL:
            EXPECT_EQ(cpp2c::listenerResult(listenerError.cpp), listenerError.c);
            break;
        case iox::popo::ListenerError::EVENT_ALREADY_ATTACHED:
            EXPECT_EQ(cpp2c::listenerResult(listenerError.cpp), listenerError.c);
            break;
        case iox::popo::ListenerError::EMPTY_INVALIDATION_CALLBACK:
            EXPECT_EQ(cpp2c::listenerResult(listenerError.cpp), listenerError.c);
            break;
        case iox::popo::ListenerError::INVALID_STATE:
            EXPECT_EQ(cpp2c::listenerResult(listenerError.cpp), listenerError.c);
            break;
            // default intentionally left out in order to get a compiler warning if the enum gets extended and we forgot
            // to extend the test
        }
    }

    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::listenerResult(static_cast<iox::popo::ListenerError>(-1)), ListenerResult_UNDEFINED_ERROR);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, SubscriberTooSlowPolicy)
{
    EXPECT_EQ(cpp2c::subscriberTooSlowPolicy(iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER),
              SubscriberTooSlowPolicy_WAIT_FOR_SUBSCRIBER);
    EXPECT_EQ(cpp2c::subscriberTooSlowPolicy(iox::popo::SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA),
              SubscriberTooSlowPolicy_DISCARD_OLDEST_DATA);
    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::subscriberTooSlowPolicy(static_cast<iox::popo::SubscriberTooSlowPolicy>(-1)),
              SubscriberTooSlowPolicy_DISCARD_OLDEST_DATA);
#pragma GCC diagnostic pop
}

TEST(cpp2c_enum_translation_test, QueueFullPolicy)
{
    EXPECT_EQ(cpp2c::queueFullPolicy(iox::popo::QueueFullPolicy::BLOCK_PUBLISHER), QueueFullPolicy_BLOCK_PUBLISHER);
    EXPECT_EQ(cpp2c::queueFullPolicy(iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA),
              QueueFullPolicy_DISCARD_OLDEST_DATA);
    // ignore the warning since we would like to test the behavior of an invalid enum value
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    EXPECT_EQ(cpp2c::queueFullPolicy(static_cast<iox::popo::QueueFullPolicy>(-1)), QueueFullPolicy_DISCARD_OLDEST_DATA);
#pragma GCC diagnostic pop
}

} // namespace
