# WaitSet in C

## Thread Safety

The WaitSet is **not** thread-safe!

- It is **not** allowed to attach or detach _Triggerable_
   classes with `iox_ws_attach_**` or `iox_ws_detach_**` when another thread is currently
   waiting for notifications with `iox_ws_wait` or `iox_ws_timed_wait`.
- Do **not** call any of the `iox_ws_` functions concurrently.

The _TriggerHandle_ on the other hand, is thread-safe! Therefore you are allowed to
attach/detach a _TriggerHandle_ to a _Triggerable_ while another thread may
trigger the _TriggerHandle_.

## Introduction

A detailed introduction into the WaitSet nomenclature and topic can be found in the
[waitset C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/waitset).
Here we will only introduce the C API and not the WaitSet in general. For this, we will
take a look at the same use case as the
[waitset C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/waitset).
The examples are structured in the same way as the C++ ones.

## Expected Output

[![asciicast](https://asciinema.org/a/407431.svg)](https://asciinema.org/a/407431)

## Code Walkthrough

!!! attention
    Please be aware of the thread-safety restrictions of the _WaitSet_ and
    read the [Thread Safety](#thread-safety) chapter carefully.

To run an example you need a running `iox-roudi` and the waitset publisher
`iox-c-waitset-publisher`. They are identical to the ones introduced in the
[icedelivery C example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery_in_c).

### Gateway

Let's say we would like to write a gateway and would like to forward every
incoming message from a subscriber with the same callback. For instance we could perform
a memcopy of the received data into a specific struct. Additionally, we would
like to count all processed samples therefor we provide an extra void pointer
argument called `contextData` which is a pointer to an `uint64_t`.

This could be performed by a function that we attach to an event as a
callback. In our case, we have the function `subscriberCallback` that
prints out the subscriber pointer and the content of the received sample.

```c
void subscriberCallback(iox_sub_t const subscriber, void * const contextData)
{
    if (contextData == NULL)
    {
        fprintf(stderr, "aborting subscriberCallback since contextData is a null pointer\n");
        return;
    }

    uint64_t* sumOfAllSamples = (uint64_t*)contextData;
    const void* userPayload;
    while (iox_sub_take_chunk(subscriber, &userPayload) == ChunkReceiveResult_SUCCESS)
    {
        printf("subscriber: %p received %u\n", (void*)subscriber, ((struct CounterTopic*)userPayload)->counter);
        fflush(stdout);

        iox_sub_release_chunk(subscriber, userPayload);
        ++(*sumOfAllSamples);
    }
}
```

The `shutdownTrigger` gets a simplified callback where it just states that the
program will be terminated. For this we do not need any context data.
```c
void shutdownCallback(iox_user_trigger_t userTrigger)
{
    (void)userTrigger;
    printf("CTRL+c pressed - exiting now\n");
    fflush(stdout);
}
```

Since we attach the `SubscriberEvent_DATA_RECEIVED` event to the _WaitSet_ that
notifies us just once when data was received we have to gather and process all chunks.
One will never miss chunks since the event notification is reset after a call to
`iox_ws_wait` or `iox_ws_timed_wait` which we introduce below.

After we registered our runtime we create some stack storage for our WaitSet,
initialize it and attach a `shutdownTrigger` to handle `CTRL-c`.

```c
iox_runtime_init("iox-c-waitset-gateway");

iox_ws_storage_t waitSetStorage;
iox_ws_t waitSet = iox_ws_init(&waitSetStorage);
shutdownTrigger = iox_user_trigger_init(&shutdownTriggerStorage);

iox_ws_attach_user_trigger_event(waitSet, shutdownTrigger, 0U, shutdownCallback);
```

In the next steps, we define `sumOfAllSamples`, create two subscribers with `iox_sub_init`,
subscribe them to our topic
and attach the event `SubscriberEvent_DATA_RECEIVED` to the WaitSet with
the `subscriberCallback`, an event id `1U` and a pointer to our user defined
context data `sumOfAllSamples` which is then provided as argument for the callback.

!!! attention 
    The user has to ensure that the contextData (`sumOfAllSamples`) in
    `iox_ws_attach_subscriber_event_with_context_data` lives as long as the
    attachment, with its callback, is attached otherwise the callback context
    data pointer is dangling.

```c
uint64_t sumOfAllSamples = 0U;

iox_sub_storage_t subscriberStorage[NUMBER_OF_SUBSCRIBERS];

iox_sub_options_t options;
iox_sub_options_init(&options);
options.historyRequest = 1U;
options.queueCapacity = 256U;
options.nodeName = "iox-c-waitSet-gateway-node";
for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
{
    iox_sub_t subscriber = iox_sub_init(&(subscriberStorage[i]), "Radar", "FrontLeft", "Counter", &options);

    iox_ws_attach_subscriber_event_with_context_data(
        waitSet, subscriber, SubscriberEvent_DATA_RECEIVED, 1U, subscriberCallback, &sumOfAllSamples);
}
```

Now that everything is set up we enter the event loop. It always starts with
a call to `iox_ws_wait`, a blocking call which returns us the number
of occurred notifications.

```c
uint64_t missedElements = 0U;
uint64_t numberOfNotifications = 0U;

iox_notification_info_t notificationArray[NUMBER_OF_NOTIFICATIONS];

bool keepRunning = true;
while (keepRunning)
{
    numberOfNotifications =
        iox_ws_wait(waitSet, notificationArray, NUMBER_OF_NOTIFICATIONS, &missedElements);
```

The events which have occurred are stored in the `notificationArray`. We iterate through
it, if the `shutdownTrigger` was triggered we terminate the program otherwise
we call the callback with `iox_notification_info_call(notification)`.

```c
for (uint64_t i = 0U; i < numberOfNotifications; ++i)
{
    iox_notification_info_t notification = notificationArray[i];

    if (iox_notification_info_does_originate_from_user_trigger(notification, shutdownTrigger))
    {
        keepRunning = false;
    }
    else
    {
        iox_notification_info_call(notification);
    }

    printf("sum of all samples: %lu\n", sumOfAllSamples);
    fflush(stdout);
}
```

Before we can close the program, we cleanup all resources.

```c
for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
{
    iox_ws_detach_subscriber_event(waitSet, (iox_sub_t) & (subscriberStorage[i]), SubscriberEvent_DATA_RECEIVED);
    iox_sub_deinit((iox_sub_t) & (subscriberStorage[i]));
}

iox_ws_deinit(waitSet);
iox_user_trigger_deinit(shutdownTrigger);
```

### Grouping

In this scenario, we have two groups of subscribers. We are interested in the
data of the first group and would like to print them onto the console and the
data of the second group should be discarded.

We start like in every example with creating the WaitSet and attaching the
`shutdownTrigger`.

```c
iox_runtime_init("iox-c-waitset-grouping");

iox_ws_storage_t waitSetStorage;
iox_ws_t waitSet = iox_ws_init(&waitSetStorage);
shutdownTrigger = iox_user_trigger_init(&shutdownTriggerStorage);

iox_ws_attach_user_trigger_event(waitSet, shutdownTrigger, 0U, NULL);
```

After that we can create a list of subscribers and subscribe them to our topic.

```c
iox_sub_storage_t subscriberStorage[NUMBER_OF_SUBSCRIBERS];
iox_sub_t subscriber[NUMBER_OF_SUBSCRIBERS];

iox_sub_options_t options;
iox_sub_options_init(&options);
options.historyRequest = 1U;
options.queueCapacity = 256U;
options.nodeName = "iox-c-waitset-grouping-node";
for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
{
    subscriber[i] = iox_sub_init(&(subscriberStorage[i]), "Radar", "FrontLeft", "Counter", &options);

}
```

To distinct our two groups we set the eventId of the first group to
`123` and of the second group to `456`. The first two subscribers are attached with
the `SubscriberState_HAS_DATA` state and the event id of the first group to our waitset.
The third and forth subscriber are attached to the same
waitset under the second group id.

```c
const uint64_t FIRST_GROUP_ID = 123;
const uint64_t SECOND_GROUP_ID = 456;

for (uint64_t i = 0U; i < 2U; ++i)
{
    iox_ws_attach_subscriber_state(waitSet, subscriber[i], SubscriberState_HAS_DATA, FIRST_GROUP_ID, NULL);
}

for (uint64_t i = 2U; i < 4U; ++i)
{
    iox_ws_attach_subscriber_state(waitSet, subscriber[i], SubscriberState_HAS_DATA, SECOND_GROUP_ID, NULL);
}
```

We are again ready for our event loop. We start as usual by setting the array
of notifications by calling `iox_ws_wait`.

```c
bool keepRunning = true;
while (keepRunning)
{
    numberOfNotifications =
        iox_ws_wait(waitSet, notificationArray, NUMBER_OF_NOTIFICATIONS, &missedElements);
```

When we iterate through the array we handle the `shutdownTrigger` first.
We check if an event is from the first group by calling 
`iox_notification_info_get_event_id` and compare the result with `FIRST_GROUP_ID`.
If that is the case we acquire the subscriber handle with
`iox_notification_info_get_subscriber_origin`. This allows us to receive the new
sample and to print the result to the console.
The second group is handled in the same way. But we do not print the new samples
to screen, we just discard them.

```c
for (uint64_t i = 0U; i < numberOfNotifications; ++i)
{
    iox_notification_info_t event = notificationArray[i];

    if (iox_notification_info_does_originate_from_user_trigger(event, shutdownTrigger))
    {
        keepRunning = false;
    }
    else if (iox_notification_info_get_event_id(event) == FIRST_GROUP_ID)
    {
        iox_sub_t subscriber = iox_notification_info_get_subscriber_origin(event);
        const void* userPayload;
        if (iox_sub_take_chunk(subscriber, &userPayload))
        {
            printf("received: %u\n", ((struct CounterTopic*)userPayload)->counter);

            iox_sub_release_chunk(subscriber, userPayload);
        }
    }
    else if (iox_notification_info_get_event_id(event) == SECOND_GROUP_ID)
    {
        printf("dismiss data\n");
        iox_sub_t subscriber = iox_notification_info_get_subscriber_origin(event);
        iox_sub_release_queued_chunks(subscriber);
    }
}
```

In the case of the `SECOND_GROUP_ID` we have to release all queued chunks otherwise
the _WaitSet_ would notify us right away since the `SubscriberState_HAS_DATA` still
persists.

The last thing we have to do is to cleanup all the acquired resources.

```c
for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
{
    iox_sub_deinit((iox_sub_t) & (subscriberStorage[i]));
}

iox_ws_deinit(waitSet);
iox_user_trigger_deinit(shutdownTrigger);
```

### Individual

We also can handle every event individually, for instance when you would like
to have a different reaction for every subscriber which has received a sample.
One way would be to assign every subscriber a different callback, here we look
at a different approach. We check if the event originated from a specific
subscriber and then perform the calls on that subscriber directly.

We start as usual by creating a WaitSet and attach the `shutdownTrigger` to it.

```c
iox_runtime_init("iox-c-waitset-individual");

iox_ws_storage_t waitSetStorage;
iox_ws_t waitSet = iox_ws_init(&waitSetStorage);
shutdownTrigger = iox_user_trigger_init(&shutdownTriggerStorage);

iox_ws_attach_user_trigger_event(waitSet, shutdownTrigger, 0U, NULL);
```

Now we create two subscribers, subscribe them to our topic and attach them to
the waitset without a callback and with the same trigger id.

```c
iox_sub_options_t options;
iox_sub_options_init(&options);
options.historyRequest = 1U;
options.queueCapacity = 256U;
options.nodeName = "iox-c-waitset-individual-node1";
    
subscriber[0] = iox_sub_init(&(subscriberStorage[0]), "Radar", "FrontLeft", "Counter", &options);
options.nodeName = "iox-c-waitset-individual-node2";
subscriber[1] = iox_sub_init(&(subscriberStorage[1]), "Radar", "FrontLeft", "Counter", &options);

iox_ws_attach_subscriber_state(waitSet, subscriber[0U], SubscriberState_HAS_DATA, 0U, NULL);
iox_ws_attach_subscriber_state(waitSet, subscriber[1U], SubscriberState_HAS_DATA, 0U, NULL);
```

We are ready to start the event loop. We begin by acquiring the array of all
the triggered triggers.

```c
bool keepRunning = true;
while (keepRunning)
{
    numberOfNotifications =
        iox_ws_wait(waitSet, notificationArray, NUMBER_OF_NOTIFICATIONS, &missedElements);
```

The `shutdownTrigger` is handled as usual and
we use `iox_notification_info_does_originate_from_subscriber`
to identify the event that originated from a specific subscriber. If it originated
from the first subscriber we print the received data to the console, if it
originated from the second subscriber we discard the data.

```c
    for (uint64_t i = 0U; i < numberOfNotifications; ++i)
    {
        iox_notification_info_t event = notificationArray[i];

        if (iox_notification_info_does_originate_from_user_trigger(event, shutdownTrigger))
        {
            keepRunning = false;
        }
        else if (iox_notification_info_does_originate_from_subscriber(event, subscriber[0]))
        {
            const void* userPayload;
            if (iox_sub_take_chunk(subscriber[0], &userPayload))
            {
                printf("subscriber 1 received: %u\n", ((struct CounterTopic*)userPayload)->counter);

                iox_sub_release_chunk(subscriber[0], userPayload);
            }
        }
        else if (iox_notification_info_does_originate_from_subscriber(event, subscriber[1]))
        {
            iox_sub_release_queued_chunks(subscriber[1]);
            printf("subscriber 2 received something - dont care\n");
        }
    }
```

We conclude the example as always, by cleaning up the resources.

```c
for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBERS; ++i)
{
    iox_sub_deinit((iox_sub_t) & (subscriberStorage[i]));
}

iox_ws_deinit(waitSet);
iox_user_trigger_deinit(shutdownTrigger);
```

### Sync
In this example, we demonstrate how you can use the WaitSet to trigger a cyclic
call every second. We use a user trigger which will be triggered in a separate
thread every second to signal the WaitSet that it's time for the next run.
Additionally, we attach a callback (`cyclicRun`) to this user trigger
so that the event can directly call the cyclic call.

We begin by creating the waitset and attach the `shutdownTrigger`.

```c
iox_runtime_init("iox-c-waitset-sync");

iox_ws_storage_t waitSetStorage;
iox_ws_t waitSet = iox_ws_init(&waitSetStorage);
shutdownTrigger = iox_user_trigger_init(&shutdownTriggerStorage);

iox_ws_attach_user_trigger_event(waitSet, shutdownTrigger, 0, NULL);
```

Now we create our cyclic trigger and attach it to our waitset with an eventId
of `0` and the callback `cyclicRun`.

```c
cyclicTrigger = iox_user_trigger_init(&cyclicTriggerStorage);
iox_ws_attach_user_trigger_event(waitSet, cyclicTrigger, 0, cyclicRun);
```

The thread which will trigger the `cyclicTrigger` every second is started in
the next lines.

```c
pthread_t cyclicTriggerThread;
if (pthread_create(&cyclicTriggerThread, NULL, cyclicTriggerCallback, NULL))
{
    printf("failed to create thread\n");
    return -1;
}
```

Everything is prepared and we enter the event loop. We start by gathering all
notifications in an array.

```c
while (keepRunning)
{
    numberOfNotifications =
        iox_ws_wait(waitSet, notificationArray, NUMBER_OF_NOTIFICATIONS, &missedElements);
```

The `shutdownTrigger` is handled as usual and the `cyclicTrigger` is handled by
just calling the attached callback with `iox_notification_info_call(notification)`.

```c
    for (uint64_t i = 0U; i < numberOfNotifications; ++i)
    {
        iox_notification_info_t notification = notificationArray[i];

        if (iox_notification_info_does_originate_from_user_trigger(notification, shutdownTrigger))
        {
            // CTRL+c was pressed -> exit
            keepRunning = false;
        }
        else
        {
            // call myCyclicRun
            iox_notification_info_call(notification);
        }
    }
```

The last thing we have to do is to cleanup all the used resources.

```c
    pthread_join(cyclicTriggerThread, NULL);
    iox_ws_deinit(waitSet);
    iox_user_trigger_deinit(shutdownTrigger);
```

<center>
[Check out waitset_in_c on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/waitset_in_c){ .md-button }
</center>
