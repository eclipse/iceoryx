# icedelivery in C

You can find a more detailed description of the C API in the
[iceoryx_binding_c README.md](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_binding_c/README.md).

## Introduction

The behavior and structure is identical to the
[icedelivery C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery)
so that we explain here only the C API differences and not the underlying mechanisms.

## Expected Output

[![asciicast](https://asciinema.org/a/407361.svg)](https://asciinema.org/a/407361)

## Code walkthrough

### Subscriber

Like in the
[icedelivery C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery)
we again follow the steps like:

 1. Create runtime instance.
 2. Create subscriber port and subscribe to the offered service.
 3. Receive data.
 4. **C API: Additionally, we have to remove the previously allocated Subscriber
        port!**

Let's take a look at the `receiving` function that comes with the
`ice_c_subscriber.c` example.

 1. We register our process at roudi with the name `iox-c-subscriber`

    ```c
    const char APP_NAME[] = "iox-c-subscriber";
    iox_runtime_init("APP_NAME");
    ```
  
 2. We create a subscriber port and subscribe to the service
    {"Radar", "FrontLeft", "Counter" }. Hereby the `historyRequest`
    tells the subscriber how many previously sent samples it should receive
    right after the connection is established and the `queueCapacity` how many
    samples the subscriber can hold. These are samples which the publisher has
    sent before the subscriber was connected. The `nodeName` is the name of the
    node, where the subscriber belongs.
    The `subscriberStorage` is the place where the subscriber is stored in
    memory and `subscriber` is actually a pointer to that location.

    ```c
    iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.historyRequest = 10U;
    options.queueCapacity = 5U;
    options.nodeName = "iox-c-subscriber-node";

    iox_sub_storage_t subscriberStorage;
    iox_sub_t subscriber = iox_sub_init(&subscriberStorage, "Radar", "FrontLeft", "Object", &options);
    ```

 3. In this loop we receive samples as long the `killswitch` is not
    set to `true` by an external signal and then print the counter
    value to the console.

    ```c
    while (!killswitch)
    {
        if (SubscribeState_SUBSCRIBED == iox_sub_get_subscription_state(subscriber))
        {
            const void* userPayload = NULL;
            while (ChunkReceiveResult_SUCCESS == iox_sub_take_chunk(subscriber, &userPayload))
            {
                const struct RadarObject* sample = (const struct RadarObject*)(userPayload);
                printf("%s got value: %.0f\n", APP_NAME, sample->x);
                iox_sub_release_chunk(subscriber, userPayload);
            }
        }
        else
        {
            printf("Not subscribed!\n");
        }

        sleep_for(1000);
    }
    ```

 4. When using the C API we have to clean up the subscriber after
    its usage.

    ```c
    iox_sub_deinit(subscriber);
    ```

### Publisher

The publisher is implemented in a similar way like in the
[icedelivery C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery):

 1. Create runtime instance.
 2. Create publisher port and offer the service.
 3. Send data.
 4. **C API: Additionally, we have to remove the previously allocated Publisher
        port!**

Let's take a look at the `sending` function that comes with the
`ice_c_publisher.c` example.

 1. We register our process at roudi with the name `iox-c-subscriber`

    ```c
    const char APP_NAME[] = "iox-c-publisher";
    iox_runtime_init("APP_NAME");
    ```

 2. We create a publisher with the service
    {"Radar", "FrontLeft", "Counter"}

    ```c
    iox_pub_options_t options;
    iox_pub_options_init(&options);
    options.historyCapacity = 10U;
    options.nodeName = "iox-c-publisher-node";
    iox_pub_storage_t publisherStorage;
    iox_pub_t publisher = iox_pub_init(&publisherStorage, "Radar", "FrontLeft", "Object", &options);
    ```

 3. Till an external signal sets `killswitch` to `true` we will send an
    incrementing number to all subscribers in every iteration and print the
    value of this number to the console.

    ```c
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

            iox_pub_publish_chunk(publisher, userPayload);

            ++ct;

            sleep_for(400);
        }
        else
        {
            printf("Failed to allocate chunk!");
        }
    }
    ```

 4. And we cleanup our publisher port.

    ```c
    iox_pub_destroy(publisher);
    ```

<center>
[Check out icedelivery_in_c on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery_in_c){ .md-button }
</center>
