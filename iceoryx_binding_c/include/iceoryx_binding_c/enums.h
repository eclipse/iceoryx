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

#ifndef IOX_BINDING_C_ENUMS_H
#define IOX_BINDING_C_ENUMS_H

/// @brief describes states which can be triggered by a subscriber
enum iox_SubscriberState
{
    SubscriberState_HAS_DATA,
};

/// @brief describes events which can be triggered by a subscriber
enum iox_SubscriberEvent
{
    SubscriberEvent_DATA_RECEIVED,
};

/// @brief describes the current state of a subscriber
enum iox_SubscribeState
{
    SubscribeState_NOT_SUBSCRIBED = 0,
    SubscribeState_SUBSCRIBE_REQUESTED,
    SubscribeState_SUBSCRIBED,
    SubscribeState_UNSUBSCRIBE_REQUESTED,
    SubscribeState_WAIT_FOR_OFFER,
    SubscribeState_UNDEFINED_ERROR,
};

/// @brief describes the state of getChunk in the subscriber
enum iox_ChunkReceiveResult
{
    ChunkReceiveResult_TOO_MANY_CHUNKS_HELD_IN_PARALLEL,
    ChunkReceiveResult_NO_CHUNK_AVAILABLE,
    ChunkReceiveResult_UNDEFINED_ERROR,
    ChunkReceiveResult_SUCCESS,
};

/// @brief used by subscriber; describes whether a publisher blocks when subscriber queue is full
enum iox_QueueFullPolicy
{
    QueueFullPolicy_BLOCK_PUBLISHER,
    QueueFullPolicy_DISCARD_OLDEST_DATA,
};

/// @brief used by publisher; describes whether a publisher blocks when subscriber queue is full
enum iox_SubscriberTooSlowPolicy
{
    SubscriberTooSlowPolicy_WAIT_FOR_SUBSCRIBER,
    SubscriberTooSlowPolicy_DISCARD_OLDEST_DATA,
};

/// @brief state of allocateChunk
enum iox_AllocationResult
{
    AllocationResult_RUNNING_OUT_OF_CHUNKS,
    AllocationResult_TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL,
    AllocationResult_INVALID_CHUNK,
    AllocationResult_INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER,
    AllocationResult_UNDEFINED_ERROR,
    AllocationResult_INVALID_PARAMETER_FOR_CHUNK,
    AllocationResult_SUCCESS,
};

enum iox_WaitSetResult
{
    WaitSetResult_WAIT_SET_FULL,
    WaitSetResult_ALREADY_ATTACHED,
    WaitSetResult_UNDEFINED_ERROR,
    WaitSetResult_SUCCESS
};

enum iox_ListenerResult
{
    ListenerResult_LISTENER_FULL,
    ListenerResult_EVENT_ALREADY_ATTACHED,
    ListenerResult_EMPTY_INVALIDATION_CALLBACK,
    ListenerResult_UNDEFINED_ERROR,
    ListenerResult_SUCCESS
};

#endif
