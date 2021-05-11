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

#ifndef IOX_BINDING_C_C2CPP_ENUM_TRANSLATION_H
#define IOX_BINDING_C_C2CPP_ENUM_TRANSLATION_H

#include "c2cpp_binding.h"
#include "iceoryx_binding_c/enums.h"
#include "iceoryx_posh/popo/base_subscriber.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"

namespace c2cpp
{
iox::popo::SubscriberTooSlowPolicy subscriberTooSlowPolicy(const ENUM iox_SubscriberTooSlowPolicy policy);
iox::popo::QueueFullPolicy queueFullPolicy(const ENUM iox_QueueFullPolicy policy);
iox::popo::SubscriberEvent subscriberEvent(const iox_SubscriberEvent value) noexcept;
iox::popo::SubscriberState subscriberState(const iox_SubscriberState value) noexcept;
} // namespace c2cpp

#endif
