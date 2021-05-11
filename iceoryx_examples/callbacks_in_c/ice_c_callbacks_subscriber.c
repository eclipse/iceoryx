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
#include "iceoryx_binding_c/user_trigger.h"
#include "sleep_for.h"
#include "topic_data.h"

#if !defined(_WIN32)
#include <pthread.h>
#endif
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool keepRunning = true;

iox_user_trigger_t heartbeat;

static void sigHandler(int signalValue)
{
    (void)signalValue;
    // caught SIGINT or SIGTERM, now exit gracefully
    keepRunning = false;
}

struct cache_t
{
    struct CounterTopic value;
    bool isSet;
};

struct cache_t leftCache = {.isSet = false};
struct cache_t rightCache = {.isSet = false};

void heartbeatCallback(iox_user_trigger_t userTrigger)
{
    (void)userTrigger;
    printf("heartbeat received\n");
    fflush(stdout);
}

void* cyclicHeartbeatTrigger(void* dontCare)
{
    (void)dontCare;
    while (keepRunning)
    {
        iox_user_trigger_trigger(heartbeat);
        sleep_for(4000);
    }
    return NULL;
}

void onSampleReceivedCallback(iox_sub_t subscriber)
{
    const struct CounterTopic* userPayload;
    if (iox_sub_take_chunk(subscriber, (const void**)&userPayload) == ChunkReceiveResult_SUCCESS)
    {
        iox_service_description_t serviceDescription = iox_sub_get_service_description(subscriber);
        if (strcmp(serviceDescription.instanceString, "FrontLeft") == 0)
        {
            leftCache.value = *userPayload;
            leftCache.isSet = true;
        }
        else if (strcmp(serviceDescription.instanceString, "FrontRight") == 0)
        {
            rightCache.value = *userPayload;
            rightCache.isSet = true;
        }
        printf("received: %d\n", userPayload->counter);
        fflush(stdout);
    }

    if (leftCache.isSet && rightCache.isSet)
    {
        printf("Received samples from FrontLeft and FrontRight. Sum of %d + %d = %d\n",
               leftCache.value.counter,
               rightCache.value.counter,
               leftCache.value.counter + rightCache.value.counter);
        fflush(stdout);
        leftCache.isSet = false;
        rightCache.isSet = false;
    }
}

int main()
{
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    iox_runtime_init("iox-c-callback-subscriber");

    // the listener starts a background thread and the callbacks of the attached events
    // will be called in this background thread when they are triggered
    iox_listener_storage_t listenerStorage;
    iox_listener_t listener = iox_listener_init(&listenerStorage);

    iox_user_trigger_storage_t heartbeatStorage;
    heartbeat = iox_user_trigger_init(&heartbeatStorage);

    iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.historyRequest = 10U;
    options.queueCapacity = 5U;
    options.nodeName = "iox-c-callback-subscriber-node";
    iox_sub_storage_t subscriberLeftStorage, subscriberRightStorage;

    iox_sub_t subscriberLeft = iox_sub_init(&subscriberLeftStorage, "Radar", "FrontLeft", "Counter", &options);
    iox_sub_t subscriberRight = iox_sub_init(&subscriberRightStorage, "Radar", "FrontRight", "Counter", &options);

    // send a heartbeat every 4 seconds
#if !defined(_WIN32)
    pthread_t heartbeatTriggerThread;
    if (pthread_create(&heartbeatTriggerThread, NULL, cyclicHeartbeatTrigger, NULL))
    {
        printf("failed to create thread\n");
        return -1;
    }
#endif

    // attach everything to the listener, from here one the callbacks are called when an event occurs
    iox_listener_attach_user_trigger_event(listener, heartbeat, &heartbeatCallback);
    iox_listener_attach_subscriber_event(
        listener, subscriberLeft, SubscriberEvent_DATA_RECEIVED, &onSampleReceivedCallback);
    iox_listener_attach_subscriber_event(
        listener, subscriberRight, SubscriberEvent_DATA_RECEIVED, &onSampleReceivedCallback);

    // wait until someone presses CTRL+c
    while (keepRunning)
    {
        sleep_for(100);
    }

    // optional detachEvent, but not required.
    //   when the listener goes out of scope it will detach all events and when a
    //   subscriber goes out of scope it will detach itself from the listener
    iox_listener_detach_user_trigger_event(listener, heartbeat);
    iox_listener_detach_subscriber_event(listener, subscriberLeft, SubscriberEvent_DATA_RECEIVED);
    iox_listener_detach_subscriber_event(listener, subscriberRight, SubscriberEvent_DATA_RECEIVED);

#if !defined(_WIN32)
    pthread_join(heartbeatTriggerThread, NULL);
#endif

    iox_user_trigger_deinit(heartbeat);
    iox_sub_deinit(subscriberLeft);
    iox_sub_deinit(subscriberRight);
    iox_listener_deinit(listener);

    return 0;
}
