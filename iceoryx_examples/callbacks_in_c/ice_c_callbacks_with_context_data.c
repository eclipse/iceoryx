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


#include "iceoryx_binding_c/listener.h"
#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "sleep_for.h"
#include "topic_data.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool keepRunning = true;

static void sigHandler(int signalValue)
{
    (void)signalValue;
    // caught SIGINT or SIGTERM, now exit gracefully
    keepRunning = false;
}

typedef struct
{
    struct CounterTopic value;
    bool isSet;
} cache_t;

typedef struct
{
    cache_t leftCache;
    cache_t rightCache;
} CounterService;

void onSampleReceivedCallback(iox_sub_t subscriber, void* contextData)
{
    if (contextData == NULL)
    {
        fprintf(stderr, "aborting onSampleReceivedCallback since contextData is a null pointer\n");
        return;
    }

    CounterService* self = (CounterService*)contextData;
    const struct CounterTopic* userPayload;
    if (iox_sub_take_chunk(subscriber, (const void**)&userPayload) == ChunkReceiveResult_SUCCESS)
    {
        iox_service_description_t serviceDescription = iox_sub_get_service_description(subscriber);
        if (strcmp(serviceDescription.instanceString, "FrontLeft") == 0)
        {
            self->leftCache.value = *userPayload;
            self->leftCache.isSet = true;
        }
        else if (strcmp(serviceDescription.instanceString, "FrontRight") == 0)
        {
            self->rightCache.value = *userPayload;
            self->rightCache.isSet = true;
        }
        printf("received: %d\n", userPayload->counter);
        fflush(stdout);
    }

    if (self->leftCache.isSet && self->rightCache.isSet)
    {
        printf("Received samples from FrontLeft and FrontRight. Sum of %d + %d = %d\n",
               self->leftCache.value.counter,
               self->rightCache.value.counter,
               self->leftCache.value.counter + self->rightCache.value.counter);
        fflush(stdout);
        self->leftCache.isSet = false;
        self->rightCache.isSet = false;
    }
}

int main()
{
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    iox_runtime_init("iox-c-callbacks-with-context-data");

    // the listener starts a background thread and the callbacks of the attached events
    // will be called in this background thread when they are triggered
    iox_listener_storage_t listenerStorage;
    iox_listener_t listener = iox_listener_init(&listenerStorage);

    iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.historyRequest = 10U;
    options.queueCapacity = 5U;
    options.nodeName = "iox-c-callback-subscriber-node";
    iox_sub_storage_t subscriberLeftStorage, subscriberRightStorage;

    CounterService counterService;
    counterService.leftCache.isSet = false;
    counterService.rightCache.isSet = false;

    iox_sub_t subscriberLeft = iox_sub_init(&subscriberLeftStorage, "Radar", "FrontLeft", "Counter", &options);
    iox_sub_t subscriberRight = iox_sub_init(&subscriberRightStorage, "Radar", "FrontRight", "Counter", &options);

    // attach everything to the listener, from here on the callbacks are called when an event occurs
    // we attach the pointer to counterService as context data that is then provided as second argument to
    // the callback which allows us to modify counterService from within the callback
    // important: the user has to ensure that the contextData (counterService) lives as long as
    //            the subscriber with its callback is attached to the listener
    iox_listener_attach_subscriber_event_with_context_data(
        listener, subscriberLeft, SubscriberEvent_DATA_RECEIVED, &onSampleReceivedCallback, &counterService);
    iox_listener_attach_subscriber_event_with_context_data(
        listener, subscriberRight, SubscriberEvent_DATA_RECEIVED, &onSampleReceivedCallback, &counterService);

    // wait until someone presses CTRL+c
    while (keepRunning)
    {
        sleep_for(100);
    }

    // optional detachEvent, but not required.
    //   when the listener goes out of scope it will detach all events and when a
    //   subscriber goes out of scope it will detach itself from the listener
    iox_listener_detach_subscriber_event(listener, subscriberLeft, SubscriberEvent_DATA_RECEIVED);
    iox_listener_detach_subscriber_event(listener, subscriberRight, SubscriberEvent_DATA_RECEIVED);

    iox_sub_deinit(subscriberLeft);
    iox_sub_deinit(subscriberRight);
    iox_listener_deinit(listener);

    return 0;
}
