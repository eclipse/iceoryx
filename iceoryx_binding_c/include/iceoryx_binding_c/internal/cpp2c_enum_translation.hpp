// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_BINDING_C_CPP2C_ENUM_TRANSLATION_H
#define IOX_BINDING_C_CPP2C_ENUM_TRANSLATION_H

#include "iceoryx_binding_c/enums.h"

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"

namespace cpp2c
{
iox_SubscribeState subscribeState(const iox::SubscribeState value) noexcept;
iox_ChunkReceiveResult chunkReceiveResult(const iox::popo::ChunkReceiveResult value) noexcept;
iox_AllocationResult allocationResult(const iox::popo::AllocationError value) noexcept;
iox_WaitSetResult waitSetResult(const iox::popo::WaitSetError value) noexcept;
iox_ListenerResult listenerResult(const iox::popo::ListenerError value) noexcept;
iox_SubscriberTooSlowPolicy subscriberTooSlowPolicy(const iox::popo::SubscriberTooSlowPolicy policy);
iox_QueueFullPolicy queueFullPolicy(const iox::popo::QueueFullPolicy policy);
} // namespace cpp2c

#endif
