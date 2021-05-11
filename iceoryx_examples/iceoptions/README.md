# iceoptions

## Introduction

This example demonstrates what kind of quality of service options can be configured on the publisher and subscriber
side. The options can be used for the typed and untyped C++ API flavors as well as the C API.

## Expected Output

[![asciicast](https://asciinema.org/a/407362.svg)](https://asciinema.org/a/407362)

## Code walkthrough

### Publisher

In order to configure a publisher, we have to supply a struct of the type `iox::popo::PublisherOptions` as a second parameter.

`historyCapacity` will enable subscribers to read the last n samples e.g. in case they are started later than the publisher:

```cpp
publisherOptions.historyCapacity = 10U;
```

Topics are automatically offered on creation of a publisher, if you want to disable that feature and control the offering yourself, do:

```cpp
publisherOptions.offerOnCreate = false;
```

Due to the disabled `offerOnCreate` feature, don't forget to offer our topic:

```cpp
publisher.offer();
```

To organize publishers inside an application, they can be associated and grouped by providing a node name. Some frameworks call nodes _runnables_.

```cpp
publisherOptions.nodeName = "Pub_Node_With_Options";
```

To ensure that samples are never lost, you have the possibility to busy-wait for the subscriber when publishing.
Both publisher and subscriber have to request compatible policies (`SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER` and
`QueueFullPolicy::BLOCK_PUBLISHER`).

```cpp
publisherOptions.subscriberTooSlowPolicy = iox::popo::SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER;
```

With this option set, it is possible that a slow subscriber blocks a publisher indefinitely due to the busy waiting loop.
In order to be able to gracefully shutdown the application with `Ctrl+C`, the publisher needs to be unblocked.
This is done by placing the following code in the signal handler.
```
iox::runtime::PoshRuntime::getInstance().shutdown();
```

### Subscriber

To configure a subscriber, we have to supply a struct of the type `iox::popo::SubscriberOptions` as a second parameter.

The `queueCapacity` parameter specifies how many samples the queue of the subscriber object can hold. If the queue
would encounter an overflow, the oldest sample is released to create space for the newest one, which is then stored. The queue behaves like a circular buffer.

```cpp
subscriberOptions.queueCapacity = 10U;
```

`historyRequest` will enable a subscriber to receive the last n samples on subscription e.g. in case it was started later than the publisher. The publisher needs to have its `historyCapacity` enabled, too.

```cpp
subscriberOptions.historyRequest = 5U;
```

Topics are automatically subscribed on creation, if you want to disable that feature and control the subscription
yourself, set `subscribeOnCreate` appropriately:

```cpp
subscriberOptions.subscribeOnCreate = false;
```

Due to the disabled `subscribeOnCreate` feature, don't forget to subscribe to our topic:

```cpp
subscriber.subscribe();
```

Again, for organising subscribers inside an application, a `nodeName` can be applied:

```cpp
subscriberOptions.nodeName = "Sub_Node_With_Options";
```

Again, to ensure that samples are never lost, we request the publisher to busy-wait, in case of a full queue:

```cpp
subscriberOptions.queueFullPolicy = iox::popo::QueueFullPolicy::BLOCK_PUBLISHER;
```

<center>
[Check out iceoptions on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/iceoptions){ .md-button }
</center>
