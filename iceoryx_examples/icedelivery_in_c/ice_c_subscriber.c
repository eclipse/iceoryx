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

#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "sleep_for.h"
#include "topic_data.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

bool killswitch = false;

const char APP_NAME[] = "iox-c-subscriber";

static void sigHandler(int signalValue)
{
    // Ignore unused variable warning
    (void)signalValue;
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

void receiving()
{
    iox_runtime_init(APP_NAME);

    // When starting the subscriber late it will miss the first samples which the
    // publisher has send. The history ensures that we at least get the last 10
    // samples send by the publisher when we subscribe.
    iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.historyRequest = 10U;
    options.queueCapacity = 5U;
    options.nodeName = "iox-c-subscriber-node";
    iox_sub_storage_t subscriberStorage;

    iox_sub_t subscriber = iox_sub_init(&subscriberStorage, "Radar", "FrontLeft", "Object", &options);

    while (!killswitch)
    {
        if (SubscribeState_SUBSCRIBED == iox_sub_get_subscription_state(subscriber))
        {
            const void* userPayload = NULL;
            // we will receive here more then one sample since the publisher is sending a
            // new sample every 400ms and we check for new samples only every second
            while (ChunkReceiveResult_SUCCESS == iox_sub_take_chunk(subscriber, &userPayload))
            {
                const struct RadarObject* sample = (const struct RadarObject*)(userPayload);
                printf("%s got value: %.0f\n", APP_NAME, sample->x);
                fflush(stdout);
                iox_sub_release_chunk(subscriber, userPayload);
            }
            printf("\n");
        }
        else
        {
            printf("Not subscribed!\n");
        }

        sleep_for(1000);
    }

    iox_sub_deinit(subscriber);
}

int main()
{
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    receiving();

    return 0;
}
