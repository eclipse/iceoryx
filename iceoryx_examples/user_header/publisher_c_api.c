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

//! [additional include for user-header]
#include "iceoryx_binding_c/chunk.h"
//! [additional include for user-header]
#include "iceoryx_binding_c/publisher.h"
#include "iceoryx_binding_c/runtime.h"
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
    const char* APP_NAME = "iox-c-user-header-publisher";
    iox_runtime_init(APP_NAME);
    //! [initialize runtime]

    //! [create publisher]
    iox_pub_storage_t publisherStorage;
    iox_pub_t publisher = iox_pub_init(&publisherStorage, "Example", "User-Header", "Timestamp", NULL);
    //! [create publisher]

    //! [send samples in a loop]
    uint64_t timestamp = 37;
    uint64_t fibonacciLast = 0;
    uint64_t fibonacciCurrent = 1;
    while (!killswitch)
    {
        uint64_t fibonacciNext = fibonacciCurrent + fibonacciLast;
        fibonacciLast = fibonacciCurrent;
        fibonacciCurrent = fibonacciNext;

        //! [loan chunk]
        void* userPayload;
        const uint32_t ALIGNMENT = 8;
        enum iox_AllocationResult res = iox_pub_loan_aligned_chunk_with_user_header(
            publisher, &userPayload, sizeof(Data), ALIGNMENT, sizeof(Header), ALIGNMENT);
        //! [loan chunk]
        if (res == AllocationResult_SUCCESS)
        {
            //! [loan was successful]
            iox_chunk_header_t* chunkHeader = iox_chunk_header_from_user_payload(userPayload);
            Header* header = (Header*)iox_chunk_header_to_user_header(chunkHeader);
            header->publisherTimestamp = timestamp;

            Data* data = (Data*)userPayload;
            data->fibonacci = fibonacciCurrent;

            iox_pub_publish_chunk(publisher, userPayload);

            // explicit cast to unsigned long since on macOS an uint64_t is a different built-in type than on Linux
            printf("%s sent data: %lu with timestamp %ldms\n",
                   APP_NAME,
                   (unsigned long)fibonacciCurrent,
                   (unsigned long)timestamp);
            fflush(stdout);
            //! [loan was successful]
        }
        else
        {
            //! [loan failed]
            printf("Failed to allocate chunk! Error code: %d\n", res);
            fflush(stdout);
            //! [loan failed]
        }

        const uint32_t MILLISECONDS_SLEEP = 1000;
        sleep_for(MILLISECONDS_SLEEP);
        timestamp += MILLISECONDS_SLEEP;
    }
    //! [send samples in a loop]

    return 0;
}
