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

#include "iceoryx_binding_c/internal/cpp2c_publisher.hpp"
#include "iceoryx_binding_c/internal/cpp2c_subscriber.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"

using namespace iox;
using namespace iox::popo;


extern "C" {
#include "iceoryx_binding_c/types.h"
}

#include "test.hpp"

using namespace ::testing;

TEST(iox_types_test, WaitSetStorageSizeFits)
{
    EXPECT_THAT(sizeof(WaitSet<>), Le(sizeof(iox_ws_storage_t)));
    EXPECT_THAT(alignof(WaitSet<>), Le(alignof(iox_ws_storage_t)));
}

TEST(iox_types_test, ListenerStorageSizeFits)
{
    EXPECT_THAT(sizeof(Listener), Le(sizeof(iox_listener_storage_t)));
    EXPECT_THAT(alignof(Listener), Le(alignof(iox_listener_storage_t)));
}

TEST(iox_types_test, UserTriggerStorageSizeFits)
{
    EXPECT_THAT(sizeof(UserTrigger), Le(sizeof(iox_user_trigger_storage_t)));
    EXPECT_THAT(alignof(UserTrigger), Le(alignof(iox_user_trigger_storage_t)));
}

TEST(iox_types_test, cpp2c_SubscriberStorageSizeFits)
{
    EXPECT_THAT(sizeof(cpp2c_Subscriber), Le(sizeof(iox_sub_storage_t)));
    EXPECT_THAT(alignof(cpp2c_Subscriber), Le(alignof(iox_sub_storage_t)));
}

TEST(iox_types_test, cpp2c_PublisherStorageSizeFits)
{
    EXPECT_THAT(sizeof(cpp2c_Publisher), Le(sizeof(iox_pub_storage_t)));
    EXPECT_THAT(alignof(cpp2c_Publisher), Le(alignof(iox_pub_storage_t)));
}
