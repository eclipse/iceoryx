// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 Apex.AI Inc. All rights reserved.
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

#ifndef IOX_BINDING_C_WAIT_SET_H
#define IOX_BINDING_C_WAIT_SET_H

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/c2cpp_binding.h"
#include "iceoryx_binding_c/notification_info.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/user_trigger.h"

#include <time.h>

/// @brief wait set handle
typedef CLASS cpp2c_WaitSet* iox_ws_t;

/// @brief initialize wait set handle
/// @param[in] self pointer to preallocated memory of size = sizeof(iox_ws_storage_t)
/// @return handle to wait set
iox_ws_t iox_ws_init(iox_ws_storage_t* self);

/// @brief deinitialize wait set handle
/// @param[in] self the handle which should be deinitialized
void iox_ws_deinit(iox_ws_t const self);

/// @brief waits until an event occurred or the timeout was reached
/// @param[in] self handle to the wait set
/// @param[in] timeout duration how long this method should wait
/// @param[in] notificationInfoArray preallocated memory to an array of iox_notification_info_t in which
///             the notification infos, which are describing the occurred event, can be written to
/// @param[in] notificationInfoArrayCapacity the capacity of the preallocated notificationInfoArray
/// @param[in] missedElements if the notificationInfoArray has insufficient size the number of missed elements
///             which could not be written into the array are stored here
/// @return number of elements which were written into the notificationInfoArray
uint64_t iox_ws_timed_wait(iox_ws_t const self,
                           struct timespec timeout,
                           iox_notification_info_t* const notificationInfoArray,
                           const uint64_t notificationInfoArrayCapacity,
                           uint64_t* missedElements);

/// @brief waits until an event occurred
/// @param[in] self handle to the wait set
/// @param[in] notificationInfoArray preallocated memory to an array of iox_notification_info_t in which
///             the notification infos, which are describing the occurred event, can be written to
/// @param[in] notificationInfoArrayCapacity the capacity of the preallocated notificationInfoArray
/// @param[in] missedElements if the notificationInfoArray has insufficient size the number of missed elements
///             which could not be written into the array are stored here
/// @return number of elements which were written into the notificationInfoArray
uint64_t iox_ws_wait(iox_ws_t const self,
                     iox_notification_info_t* const notificationInfoArray,
                     const uint64_t notificationInfoArrayCapacity,
                     uint64_t* missedElements);

/// @brief returns the number of registered events/states
uint64_t iox_ws_size(iox_ws_t const self);

/// @brief returns the maximum amount of events/states which can be registered at the waitset
uint64_t iox_ws_capacity(iox_ws_t const self);

/// @brief Non-reversible call. After this call iox_ws_wait() and iox_ws_timed_wait() do
///        not block any longer and never return triggered events/states. This
///        function can be used to manually initialize destruction and to wakeup
///        any thread which is waiting in iox_ws_wait() or iox_ws_timed_wait().
void iox_ws_mark_for_destruction(iox_ws_t const self);

/// @brief attaches a subscriber state to a waitset
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber of the state which should be attached
/// @param[in] subscriberState the state which should be attached
/// @param[in] id an arbitrary id which will be tagged to the state
/// @param[in] callback a callback which is attached to the state
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_subscriber_state(iox_ws_t const self,
                                                      iox_sub_t const subscriber,
                                                      const ENUM iox_SubscriberState subscriberState,
                                                      const uint64_t id,
                                                      void (*callback)(iox_sub_t));

/// @brief attaches a subscriber state to a waitset. The callback has an additional contextData argument to provide
/// access to user defined data.
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber of the state which should be attached
/// @param[in] subscriberState the state which should be attached
/// @param[in] id an arbitrary id which will be tagged to the state
/// @param[in] callback a callback which is attached to the state
/// @param[in] contextData a void pointer which is provided as second argument to the callback
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_subscriber_state_with_context_data(iox_ws_t const self,
                                                                        iox_sub_t const subscriber,
                                                                        const ENUM iox_SubscriberState subscriberState,
                                                                        const uint64_t id,
                                                                        void (*callback)(iox_sub_t, void*),
                                                                        void* const contextData);

/// @brief attaches a subscriber event to a waitset
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber of the event which should be attached
/// @param[in] subscriberEvent the event which should be attached
/// @param[in] eventId an arbitrary id which will be tagged to the event
/// @param[in] callback a callback which is attached to the event
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_subscriber_event(iox_ws_t const self,
                                                      iox_sub_t const subscriber,
                                                      const ENUM iox_SubscriberEvent subscriberEvent,
                                                      const uint64_t eventId,
                                                      void (*callback)(iox_sub_t));

/// @brief attaches a subscriber event to a waitset. The callback has an additional contextData argument to provide
/// access to user defined data.
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber of the event which should be attached
/// @param[in] subscriberEvent the event which should be attached
/// @param[in] eventId an arbitrary id which will be tagged to the event
/// @param[in] callback a callback which is attached to the event
/// @param[in] contextData a void pointer which is provided as second argument to the callback
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_subscriber_event_with_context_data(iox_ws_t const self,
                                                                        iox_sub_t const subscriber,
                                                                        const ENUM iox_SubscriberEvent subscriberEvent,
                                                                        const uint64_t eventId,
                                                                        void (*callback)(iox_sub_t, void*),
                                                                        void* const contextData);

/// @brief attaches a user trigger event to a waitset
/// @param[in] self handle to the waitset
/// @param[in] userTrigger the user trigger of the event which should be attached
/// @param[in] eventId an arbitrary id which will be tagged to the event
/// @param[in] callback a callback which is attached to the event
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_user_trigger_event(iox_ws_t const self,
                                                        iox_user_trigger_t const userTrigger,
                                                        const uint64_t eventId,
                                                        void (*callback)(iox_user_trigger_t));

/// @brief attaches a user trigger event to a waitset. The callback has an additional contextData argument to provide
/// access to user defined data.
/// @param[in] self handle to the waitset
/// @param[in] userTrigger the user trigger of the event which should be attached
/// @param[in] eventId an arbitrary id which will be tagged to the event
/// @param[in] callback a callback which is attached to the event
/// @param[in] contextData a void pointer which is provided as second argument to the callback
/// @return if the attaching was successfull it returns WaitSetResult_SUCCESS, otherwise
///             an enum which describes the error
ENUM iox_WaitSetResult iox_ws_attach_user_trigger_event_with_context_data(iox_ws_t const self,
                                                                          iox_user_trigger_t const userTrigger,
                                                                          const uint64_t eventId,
                                                                          void (*callback)(iox_user_trigger_t, void*),
                                                                          void* const contextData);

/// @brief detaches a subscriber event from a waitset
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber from which the event should be detached
/// @param[in] subscriberEvent the event which should be detached from the subscriber
void iox_ws_detach_subscriber_event(iox_ws_t const self,
                                    iox_sub_t const subscriber,
                                    const ENUM iox_SubscriberEvent subscriberEvent);

/// @brief detaches a subscriber state from a waitset
/// @param[in] self handle to the waitset
/// @param[in] subscriber the subscriber from which the state should be detached
/// @param[in] subscriberState the state which should be detached from the subscriber
void iox_ws_detach_subscriber_state(iox_ws_t const self,
                                    iox_sub_t const subscriber,
                                    const ENUM iox_SubscriberState subscriberState);

/// @brief detaches a user trigger event from a waitset
/// @param[in] self handle to the waitset
/// @param[in] usertrigger the user trigger which should be detached
void iox_ws_detach_user_trigger_event(iox_ws_t const self, iox_user_trigger_t const userTrigger);

#endif
