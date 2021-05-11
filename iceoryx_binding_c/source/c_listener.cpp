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

#include "iceoryx_binding_c/internal/c2cpp_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"

using namespace iox;
using namespace iox::popo;

extern "C" {
#include "iceoryx_binding_c/listener.h"
}

iox_listener_t iox_listener_init(iox_listener_storage_t* self)
{
    if (self == nullptr)
    {
        LogWarn() << "listener initialization skipped - null pointer provided for iox_listener_storage_t";
        return nullptr;
    }
    auto me = new (self) Listener();
    return reinterpret_cast<iox_listener_t>(me);
}

void iox_listener_deinit(iox_listener_t const self)
{
    self->~Listener();
}

ENUM iox_ListenerResult iox_listener_attach_subscriber_event(iox_listener_t const self,
                                                             iox_sub_t const subscriber,
                                                             const ENUM iox_SubscriberEvent subscriberEvent,
                                                             void (*callback)(iox_sub_t))
{
    auto result =
        self->attachEvent(*subscriber, c2cpp::subscriberEvent(subscriberEvent), createNotificationCallback(*callback));
    if (result.has_error())
    {
        return cpp2c::listenerResult(result.get_error());
    }
    return ListenerResult_SUCCESS;
}

ENUM iox_ListenerResult
iox_listener_attach_subscriber_event_with_context_data(iox_listener_t const self,
                                                       iox_sub_t const subscriber,
                                                       const ENUM iox_SubscriberEvent subscriberEvent,
                                                       void (*callback)(iox_sub_t, void*),
                                                       void* const contextData)
{
    NotificationCallback<cpp2c_Subscriber, void> notificationCallback;
    notificationCallback.m_callback = callback;
    notificationCallback.m_contextData = contextData;

    auto result = self->attachEvent(*subscriber, c2cpp::subscriberEvent(subscriberEvent), notificationCallback);
    if (result.has_error())
    {
        return cpp2c::listenerResult(result.get_error());
    }
    return ListenerResult_SUCCESS;
}

ENUM iox_ListenerResult iox_listener_attach_user_trigger_event(iox_listener_t const self,
                                                               iox_user_trigger_t const userTrigger,
                                                               void (*callback)(iox_user_trigger_t))
{
    auto result = self->attachEvent(*userTrigger, createNotificationCallback(*callback));
    if (result.has_error())
    {
        return cpp2c::listenerResult(result.get_error());
    }
    return ListenerResult_SUCCESS;
}

ENUM iox_ListenerResult iox_listener_attach_user_trigger_event_with_context_data(iox_listener_t const self,
                                                                                 iox_user_trigger_t const userTrigger,
                                                                                 void (*callback)(iox_user_trigger_t,
                                                                                                  void*),
                                                                                 void* const contextData)
{
    NotificationCallback<UserTrigger, void> notificationCallback;
    notificationCallback.m_callback = callback;
    notificationCallback.m_contextData = contextData;

    auto result = self->attachEvent(*userTrigger, notificationCallback);
    if (result.has_error())
    {
        return cpp2c::listenerResult(result.get_error());
    }
    return ListenerResult_SUCCESS;
}

void iox_listener_detach_subscriber_event(iox_listener_t const self,
                                          iox_sub_t const subscriber,
                                          const ENUM iox_SubscriberEvent subscriberEvent)
{
    self->detachEvent(*subscriber, c2cpp::subscriberEvent(subscriberEvent));
}

void iox_listener_detach_user_trigger_event(iox_listener_t const self, iox_user_trigger_t const userTrigger)
{
    self->detachEvent(*userTrigger);
}

uint64_t iox_listener_size(iox_listener_t const self)
{
    return self->size();
}

uint64_t iox_listener_capacity(iox_listener_t const self)
{
    return self->capacity();
}
