// Copyright (c) 2021 Apex.AI Inc. All rights reserved.
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

#ifndef IOX_BINDING_C_LISTENER_H
#define IOX_BINDING_C_LISTENER_H

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/internal/c2cpp_binding.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/user_trigger.h"

typedef CLASS Listener* iox_listener_t;


/// @brief initializes a listener struct from a storage struct pointer
/// @param[in] self pointer to raw memory which can hold a listener
/// @return an initialized iox_listener_t
iox_listener_t iox_listener_init(iox_listener_storage_t* self);

/// @brief after using an iox_listener_t it must be cleaned up with this function
/// @param[in] self the listener which should be deinitialized
void iox_listener_deinit(iox_listener_t const self);

/// @brief Attaches a subscriber event to the listener
/// @param[in] self listener to which the event should be attached to
/// @param[in] subscriber subscriber which emits the event
/// @param[in] subscriberEvent the event which should trigger the listener
/// @param[in] callback the callback which is called when an event triggers the listener
/// @return when successful iox_ListenerResult::ListenerResult_SUCCESS otherwise an enum which describes the error
ENUM iox_ListenerResult iox_listener_attach_subscriber_event(iox_listener_t const self,
                                                             iox_sub_t const subscriber,
                                                             const ENUM iox_SubscriberEvent subscriberEvent,
                                                             void (*callback)(iox_sub_t));

/// @brief Attaches a subscriber event to the listener. The callback has an additional contextData argument to provide
/// access to user defined data.
/// @param[in] self listener to which the event should be attached to
/// @param[in] subscriber subscriber which emits the event
/// @param[in] subscriberEvent the event which should trigger the listener
/// @param[in] callback the callback which is called when an event triggers the listener
/// @param[in] contextData a void pointer which is provided as second argument to the callback
/// @return when successful iox_ListenerResult::ListenerResult_SUCCESS otherwise an enum which describes the error
ENUM iox_ListenerResult
iox_listener_attach_subscriber_event_with_context_data(iox_listener_t const self,
                                                       iox_sub_t const subscriber,
                                                       const ENUM iox_SubscriberEvent subscriberEvent,
                                                       void (*callback)(iox_sub_t, void*),
                                                       void* const contextData);

/// @brief Attaches a user trigger to the listener
/// @param[in] self listener to which the event should be attached to
/// @param[in] userTrigger user trigger which emits the event
/// @param[in] callback the callback which is called when the user trigger triggers the listener
/// @return when successful iox_ListenerResult::ListenerResult_SUCCESS otherwise an enum which describes the error
ENUM iox_ListenerResult iox_listener_attach_user_trigger_event(iox_listener_t const self,
                                                               iox_user_trigger_t const userTrigger,
                                                               void (*callback)(iox_user_trigger_t));

/// @brief Attaches a user trigger to the listener. The callback has an additional contextData argument to provide
/// access to user defined data.
/// @param[in] self listener to which the event should be attached to
/// @param[in] userTrigger user trigger which emits the event
/// @param[in] callback the callback which is called when the user trigger triggers the listener
/// @param[in] contextData a void pointer which is provided as second argument to the callback
/// @return when successful iox_ListenerResult::ListenerResult_SUCCESS otherwise an enum which describes the error
ENUM iox_ListenerResult iox_listener_attach_user_trigger_event_with_context_data(iox_listener_t const self,
                                                                                 iox_user_trigger_t const userTrigger,
                                                                                 void (*callback)(iox_user_trigger_t,
                                                                                                  void*),
                                                                                 void* const contextData);

/// @brief Detaches a subscriber event from the listener
/// @param[in] self listener from which the event should be detached
/// @param[in] subscriber the subscriber which emits the event
/// @param[in] subscriberEvent the subscriber event which is registered at the listener
void iox_listener_detach_subscriber_event(iox_listener_t const self,
                                          iox_sub_t const subscriber,
                                          const ENUM iox_SubscriberEvent subscriberEvent);

/// @brief Detaches a user trigger from the listener
/// @param[in] self listener from which the event should be detached
/// @param[in] userTrigger the user trigger which emits the event
void iox_listener_detach_user_trigger_event(iox_listener_t const self, iox_user_trigger_t const userTrigger);


/// @brief Returns the size, the number of attached events of a listener.
/// @param[in] self listener where the size should be acquired
/// @return the size of the listener
uint64_t iox_listener_size(iox_listener_t const self);

/// @brief Returns the capacity of a listener (how many events can be attached).
/// @param[in] self listener where the capacity should be acquired
/// @return the capacity of the listener
uint64_t iox_listener_capacity(iox_listener_t const self);

#endif
