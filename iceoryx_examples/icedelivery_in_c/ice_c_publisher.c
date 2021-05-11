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

#include "iceoryx_binding_c/publisher.h"
#include "iceoryx_binding_c/runtime.h"
#include "sleep_for.h"
#include "topic_data.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

bool killswitch = false;

const char APP_NAME[] = "iox-c-publisher";

static void sigHandler(int signalValue)
{
    // Ignore unused variable warning
    (void)signalValue;
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}

void sending()
{
    iox_runtime_init(APP_NAME);

    iox_pub_options_t options;
    iox_pub_options_init(&options);
    options.historyCapacity = 10U;
    options.nodeName = "iox-c-publisher-node";
    iox_pub_storage_t publisherStorage;
    iox_pub_t publisher = iox_pub_init(&publisherStorage, "Radar", "FrontLeft", "Object", &options);

    double ct = 0.0;

    while (!killswitch)
    {
        void* userPayload = NULL;
        if (AllocationResult_SUCCESS == iox_pub_loan_chunk(publisher, &userPayload, sizeof(struct RadarObject)))
        {
            struct RadarObject* sample = (struct RadarObject*)userPayload;

            sample->x = ct;
            sample->y = ct;
            sample->z = ct;

            printf("%s sent value: %.0f\n", APP_NAME, ct);
            fflush(stdout);

            iox_pub_publish_chunk(publisher, userPayload);

            ++ct;

            sleep_for(400);
        }
        else
        {
            printf("Failed to allocate chunk!");
        }
    }

    iox_pub_deinit(publisher);
}

int main()
{
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    sending();

    return 0;
}
