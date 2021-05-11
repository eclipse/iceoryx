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

#ifndef IOX_BINDING_C_SUBSCRIBER_H
#define IOX_BINDING_C_SUBSCRIBER_H

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/c2cpp_binding.h"
#include "iceoryx_binding_c/service_description.h"
#include "iceoryx_binding_c/types.h"

/// @brief Subscriber handle
typedef struct cpp2c_Subscriber* iox_sub_t;

/// @brief options to be set for a subscriber
typedef struct
{
    /// @brief size of the history chunk queue
    uint64_t queueCapacity;

    /// @brief number of chunks received after subscription if chunks are available
    uint64_t historyRequest;

    /// @brief name of the node the subscriber belongs to
    /// @note nullptr indicates that the default node name is used
    const char* nodeName;

    /// @brief The option whether the subscriber shall try to subscribe when creating it
    bool subscribeOnCreate;

    /// @brief describes whether a publisher blocks when subscriber queue is full
    ENUM iox_QueueFullPolicy queueFullPolicy;

    /// @brief this value will be set exclusively by iox_sub_options_init and is not supposed to be modified otherwise
    uint64_t initCheck;
} iox_sub_options_t;

/// @brief initialize subscriber options to default values
/// @param[in] options pointer to options to be initialized,
///                    emit warning if it is a null pointer
/// @attention This must always be called on a newly created options struct to
///            prevent uninitialized values. The options may get extended
///            in the future.
void iox_sub_options_init(iox_sub_options_t* const options);

/// @brief check whether the subscriber options were initialized by iox_sub_options_init
/// @param[in] options pointer to options to be checked
/// @return true if options are not null and were initialized, false otherwise
bool iox_sub_options_is_initialized(const iox_sub_options_t* const options);

/// @brief initialize subscriber handle
/// @param[in] self pointer to preallocated memory of size = sizeof(iox_sub_storage_t)
/// @param[in] service serviceString
/// @param[in] instance instanceString
/// @param[in] event eventString
/// @param[in] options subscriber options set by the user,
///                    if it is a null pointer default options are used
/// @return handle of the subscriber
iox_sub_t iox_sub_init(iox_sub_storage_t* self,
                       const char* const service,
                       const char* const instance,
                       const char* const event,
                       const iox_sub_options_t* const options);

/// @brief deinitialize a subscriber handle
/// @param[in] self the handle which should be removed
void iox_sub_deinit(iox_sub_t const self);

/// @brief subscribes to the service
/// @param[in] self handle to the subscriber
void iox_sub_subscribe(iox_sub_t const self);

/// @brief unsubscribes from a service
/// @param[in] self handle to the subscriber
void iox_sub_unsubscribe(iox_sub_t const self);

/// @brief what is the subscription state?
/// @param[in] self handle to the subscriber
/// @return SubscribeState_SUBSCRIBED when successfully subscribed otherwise an enum which
///         describes the current state
ENUM iox_SubscribeState iox_sub_get_subscription_state(iox_sub_t const self);

/// @brief retrieve a received chunk
/// @param[in] self handle to the subscriber
/// @param[in] userPayload pointer in which the pointer to the user-payload of the chunk is stored
/// @return if a chunk could be received it returns ChunkReceiveResult_SUCCESS otherwise
///         an enum which describes the error
ENUM iox_ChunkReceiveResult iox_sub_take_chunk(iox_sub_t const self, const void** const userPayload);

/// @brief release a previously acquired chunk (via iox_sub_getChunk)
/// @param[in] self handle to the subscriber
/// @param[in] userPayload pointer to the user-payload of chunk which should be released
void iox_sub_release_chunk(iox_sub_t const self, const void* const userPayload);

/// @brief release all chunks which are stored in the chunk queue
/// @param[in] self handle to the subscriber
void iox_sub_release_queued_chunks(iox_sub_t const self);

/// @brief are new chunks available?
/// @param[in] self handle to the subscriber
/// @return true if there are chunks, otherwise false
bool iox_sub_has_chunks(iox_sub_t const self);

/// @brief are chunks lost?
/// @param[in] self handle to the subscriber
/// @return true if there are lost chunks due to overflowing queue, otherwise false
bool iox_sub_has_lost_chunks(iox_sub_t const self);

/// @brief returns the service description of the subscriber
/// @param[in] self handle to the subscriber
/// @return the service description
iox_service_description_t iox_sub_get_service_description(iox_sub_t const self);
#endif
