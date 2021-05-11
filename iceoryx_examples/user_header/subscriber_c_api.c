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

//! [iceoryx includes]
#include "user_header_and_payload_types.h"

#include "iceoryx_binding_c/chunk.h"
#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/subscriber.h"
#include "sleep_for.h"
//! [iceoryx includes]

#include <signal.h>
#include <stdio.h>

//! [signal handling]
#ifdef _WIN32
/// @todo iox-#33 needs proper fix but it seems MSVC doesn't have stdatomic.h
volatile bool killswitch = false;
#else
#include <stdatomic.h>
atomic_bool killswitch = false;
#endif

static void sigHandler(int signalValue)
{
    // Ignore unused variable warning
    (void)signalValue;
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}
//! [signal handling]

int main()
{
    //! [register sigHandler]
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    //! [register sigHandler]

    //! [initialize runtime]
    const char* APP_NAME = "iox-c-user-header-subscriber";
    iox_runtime_init(APP_NAME);
    //! [initialize runtime]

    //! [create subscriber]
    iox_sub_storage_t subscriberStorage;
    iox_sub_t subscriber = iox_sub_init(&subscriberStorage, "Example", "User-Header", "Timestamp", NULL);
    //! [create subscriber]

    //! [poll subscriber for samples in a loop]
    while (!killswitch)
    {
        //! [take chunk]
        const void* userPayload;
        if (iox_sub_take_chunk(subscriber, &userPayload) == ChunkReceiveResult_SUCCESS)
        {
            const iox_chunk_header_t* chunkHeader = iox_chunk_header_from_user_payload_const(userPayload);
            const Header* header = (const Header*)(iox_chunk_header_to_user_header_const(chunkHeader));

            const Data* data = (const Data*)userPayload;

            // explicit cast to unsigned long since on macOS an uint64_t is a different built-in type than on Linux
            printf("%s got value: %lu with timestamp %ldms\n",
                   APP_NAME,
                   (unsigned long)data->fibonacci,
                   (unsigned long)header->publisherTimestamp);
            fflush(stdout);

            iox_sub_release_chunk(subscriber, userPayload);
        }
        //! [take chunk]

        const uint32_t MILLISECONDS_SLEEP = 100;
        sleep_for(MILLISECONDS_SLEEP);
    }
    //! [poll subscriber for samples in a loop]

    return 0;
}
