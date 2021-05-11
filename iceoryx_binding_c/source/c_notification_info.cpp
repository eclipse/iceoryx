// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/popo/notification_info.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"

using namespace iox;
using namespace iox::popo;

extern "C" {
#include "iceoryx_binding_c/notification_info.h"
}

#include <type_traits>

uint64_t iox_notification_info_get_notification_id(iox_notification_info_t const self)
{
    return self->getNotificationId();
}

bool iox_notification_info_does_originate_from_subscriber(iox_notification_info_t const self,
                                                          iox_sub_t const subscriber)
{
    return self->doesOriginateFrom(subscriber);
}

bool iox_notification_info_does_originate_from_user_trigger(iox_notification_info_t const self,
                                                            iox_user_trigger_t const user_trigger)
{
    return self->doesOriginateFrom(user_trigger);
}

iox_sub_t iox_notification_info_get_subscriber_origin(iox_notification_info_t const self)
{
    return self->getOrigin<cpp2c_Subscriber>();
}

iox_user_trigger_t iox_notification_info_get_user_trigger_origin(iox_notification_info_t const self)
{
    return self->getOrigin<UserTrigger>();
}

void iox_notification_info_call(iox_notification_info_t const self)
{
    (*self)();
}

