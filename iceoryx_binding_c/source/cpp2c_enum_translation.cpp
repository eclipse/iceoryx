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

#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/enums.h"

using namespace iox;
using namespace iox::popo;

namespace cpp2c
{
iox_SubscribeState subscribeState(const iox::SubscribeState value) noexcept
{
    switch (value)
    {
    case iox::SubscribeState::NOT_SUBSCRIBED:
        return iox_SubscribeState::SubscribeState_NOT_SUBSCRIBED;
    case iox::SubscribeState::SUBSCRIBE_REQUESTED:
        return iox_SubscribeState::SubscribeState_SUBSCRIBE_REQUESTED;
    case iox::SubscribeState::SUBSCRIBED:
        return iox_SubscribeState::SubscribeState_SUBSCRIBED;
    case iox::SubscribeState::UNSUBSCRIBE_REQUESTED:
        return iox_SubscribeState::SubscribeState_UNSUBSCRIBE_REQUESTED;
    case iox::SubscribeState::WAIT_FOR_OFFER:
        return iox_SubscribeState::SubscribeState_WAIT_FOR_OFFER;
    }
    return iox_SubscribeState::SubscribeState_UNDEFINED_ERROR;
}

iox_ChunkReceiveResult chunkReceiveResult(const iox::popo::ChunkReceiveResult value) noexcept
{
    switch (value)
    {
    case iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE:
        return ChunkReceiveResult_NO_CHUNK_AVAILABLE;
    case iox::popo::ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL:
        return ChunkReceiveResult_TOO_MANY_CHUNKS_HELD_IN_PARALLEL;
    case iox::popo::ChunkReceiveResult::INVALID_STATE:
        return ChunkReceiveResult_UNDEFINED_ERROR;
    }
    return ChunkReceiveResult_UNDEFINED_ERROR;
}

iox_AllocationResult allocationResult(const iox::popo::AllocationError value) noexcept
{
    switch (value)
    {
    case AllocationError::RUNNING_OUT_OF_CHUNKS:
        return AllocationResult_RUNNING_OUT_OF_CHUNKS;
    case AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL:
        return AllocationResult_TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL;
    case AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER:
        return AllocationResult_INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER;
    case AllocationError::INVALID_STATE:
        return AllocationResult_UNDEFINED_ERROR;
    }
    return AllocationResult_UNDEFINED_ERROR;
}

iox_WaitSetResult waitSetResult(const iox::popo::WaitSetError value) noexcept
{
    switch (value)
    {
    case WaitSetError::WAIT_SET_FULL:
        return WaitSetResult_WAIT_SET_FULL;
    case WaitSetError::ALREADY_ATTACHED:
        return WaitSetResult_ALREADY_ATTACHED;
    case WaitSetError::INVALID_STATE:
        return WaitSetResult_UNDEFINED_ERROR;
    }
    return WaitSetResult_UNDEFINED_ERROR;
}

iox_ListenerResult listenerResult(const iox::popo::ListenerError value) noexcept
{
    switch (value)
    {
    case ListenerError::EVENT_ALREADY_ATTACHED:
        return ListenerResult_EVENT_ALREADY_ATTACHED;
    case ListenerError::LISTENER_FULL:
        return ListenerResult_LISTENER_FULL;
    case ListenerError::INVALID_STATE:
        return ListenerResult_UNDEFINED_ERROR;
    case ListenerError::EMPTY_INVALIDATION_CALLBACK:
        return ListenerResult_EMPTY_INVALIDATION_CALLBACK;
    }
    return ListenerResult_UNDEFINED_ERROR;
}

iox_SubscriberTooSlowPolicy subscriberTooSlowPolicy(const iox::popo::SubscriberTooSlowPolicy policy)
{
    switch (policy)
    {
    case SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER:
        return SubscriberTooSlowPolicy_WAIT_FOR_SUBSCRIBER;
    case SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA:
        return SubscriberTooSlowPolicy_DISCARD_OLDEST_DATA;
    }
    return SubscriberTooSlowPolicy_DISCARD_OLDEST_DATA;
}
iox_QueueFullPolicy queueFullPolicy(const iox::popo::QueueFullPolicy policy)
{
    switch (policy)
    {
    case QueueFullPolicy::BLOCK_PUBLISHER:
        return QueueFullPolicy_BLOCK_PUBLISHER;
    case QueueFullPolicy::DISCARD_OLDEST_DATA:
        return QueueFullPolicy_DISCARD_OLDEST_DATA;
    }
    return QueueFullPolicy_DISCARD_OLDEST_DATA;
}

} // namespace cpp2c
