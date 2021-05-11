# Listener (or how to use callbacks with iceoryx)

## Thread Safety

The Listener is thread-safe and can be used without restrictions.
But be aware that all provided callbacks are executed concurrently
in the background thread of the Listener. If you access structures
inside this callback you have to either ensure that you are the only
one accessing it or that it is accessed with a guard like a `std::mutex`.

## Introduction

For an introduction into the terminology please read the Glossary in the
[WaitSet C++ example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/waitset).

The Listener is a completely thread-safe construct that reacts to events by
executing registered callbacks in a background thread. Events can be emitted by
_EventOrigins_ like a subscriber or a user trigger. Some of the _EventOrigins_
like the subscriber can hereby emit more than one event type.

The interface of a listener consists of two methods: `attachEvent` to attach a
new event with a callback and `detachEvent`. These two methods can be called
concurrently, even from inside a callback that was triggered by an event!

## Expected Output

[![asciicast](https://asciinema.org/a/407365.svg)](https://asciinema.org/a/407365)

## Code Walkthrough

!!! attention
    Please be aware of the thread-safety restrictions of the _Listener_ and
    read the [Thread Safety](#thread-safety) chapter carefully.

Let's say we have an application that offers us two distinct services:
`Radar.FrontLeft.Counter` and `Rader.FrontRight.Counter`. Every time we have
received a sample from left and right we would like to calculate the sum with
the newest values and print it out. If we have received only one of the samples,
we store it until we received the other side.

### ice_callbacks_publisher.cpp

The publisher of this example does not contain any new features but if you have
some questions take a look at the
[icedelivery example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery).

### ice_callbacks_subscriber.cpp

#### int main()

The subscriber main function starts as usual and after registering the runtime
we create the listener that starts a background thread.

```cpp
iox::popo::Listener listener;
```

Because it is fun, we also create a heartbeat trigger that will be triggered
every 4 seconds so that `heartbeat received` can be printed to the console.
Furthermore, we have to create two subscribers to receive samples for the two
services.

```cpp
iox::popo::UserTrigger heartbeat;
iox::popo::Subscriber<CounterTopic> subscriberLeft({"Radar", "FrontLeft", "Counter"});
iox::popo::Subscriber<CounterTopic> subscriberRight({"Radar", "FrontRight", "Counter"});
```

Next thing is a `heartbeatThread` which will trigger our heartbeat trigger every
4 seconds.

```cpp
std::thread heartbeatThread([&] {
    while (keepRunning)
    {
        heartbeat.trigger();
        std::this_thread::sleep_for(std::chrono::seconds(4));
    }
});
```

Now that everything is setup we can attach the subscribers to the listener so that
every time a new sample (`iox::popo::SubscriberEvent::DATA_RECEIVED`) is received our callback
(`onSampleReceivedCallback`) will be called. We also attach
our `heartbeat` user trigger to print the heartbeat message to the console via another
callback (`heartbeatCallback`).

```cpp
listener.attachEvent(heartbeat, iox::popo::createNotificationCallback(heartbeatCallback))
        .or_else([](auto) {
            std::cerr << "unable to attach heartbeat event" << std::endl;
            std::exit(EXIT_FAILURE);
});

listener.attachEvent(subscriberLeft,
                     iox::popo::SubscriberEvent::DATA_RECEIVED, 
                     iox::popo::createNotificationCallback(onSampleReceivedCallback))
    .or_else([](auto) {
        std::cerr << "unable to attach subscriberLeft" << std::endl;
        std::exit(EXIT_FAILURE);
    });

// It is possible to attach any c function here with a signature of void(iox::popo::Subscriber<CounterTopic> *).
// But please be aware that the listener does not take ownership of the callback, therefore it has to exist as
// long as the event is attached. Furthermore, it excludes lambdas which are capturing data since they are not
// convertable to a c function pointer.
// to simplify the example we attach the same callback onSampleReceivedCallback again
listener.attachEvent(subscriberRight,
                     iox::popo::SubscriberEvent::DATA_RECEIVED,
                     iox::popo::createNotificationCallback(onSampleReceivedCallback))
    .or_else([](auto) {
        std::cerr << "unable to attach subscriberRight" << std::endl;
        std::exit(EXIT_FAILURE);
    });
```

Since a user trigger has only one event, we do not have to specify an event when we attach
it to the listener. `attachEvent` returns a `cxx::expected` to inform us if the attachment
succeeded. When this is not the case the error handling is performed in the `.or_else([](auto){` part
after each `attachEvent` call.
In this example, we choose to attach the same callback twice to make things easier
but you are free to attach any callback with the signature `void(iox::popo::Subscriber<CounterTopic> *)`.

The setup is complete, but it would terminate right away since we have no blocker which
waits until SIGINT or SIGTERM was send. In the other examples, we had not that problem
since we pulled all the events in a while true loop but working only with callbacks
requires something like our `shutdownSemaphore`, a semaphore on which we wait until
the signal callback increments it.

```cpp
shutdownSemaphore.wait();
```

When the `shutdownSemaphore` unblocks we clean up all resources and terminate the process
gracefully.

```cpp
listener.detachEvent(heartbeat);
listener.detachEvent(subscriberLeft, iox::popo::SubscriberEvent::DATA_RECEIVED);
listener.detachEvent(subscriberRight, iox::popo::SubscriberEvent::DATA_RECEIVED);

heartbeatThread.join();
```

Hint: You do not have to detach an _EventOrigin_ like a subscriber or user trigger
before it goes out of scope. This also goes for the _Listener_, the implemented
RAII-based design takes care of the resource cleanup.

#### The Callbacks

The callbacks must have a signature like `void(PointerToEventOrigin*)`.
Our `heartbeatCallback` for instance, just prints the message `heartbeat received`.

```cpp
void heartbeatCallback(iox::popo::UserTrigger*)
{
    std::cout << "heartbeat received " << std::endl;
}
```

The `onSampleReceivedCallback` is more complex. We first acquire the received
sample and check which subscriber signaled the event by acquiring the subscriber's
service description. If the instance is equal to `FrontLeft` we store the sample
in the `leftCache` otherwise in the `rightCache`.

```cpp
void onSampleReceivedCallback(iox::popo::Subscriber<CounterTopic>* subscriber)
{
    subscriber->take().and_then([subscriber](auto& sample) {
        auto instanceString = subscriber->getServiceDescription().getInstanceIDString();

        // store the sample in the corresponding cache
        if (instanceString == iox::capro::IdString_t("FrontLeft"))
        {
            leftCache.emplace(*sample);
        }
        else if (instanceString == iox::capro::IdString_t("FrontRight"))
        {
            rightCache.emplace(*sample);
        }
```

In the next step, we check if both caches are filled. If this is the case, we print
an extra message which states the result of the sum of both received values.
Afterward, we reset both caches to start fresh again.

```cpp
    if (leftCache && rightCache)
    {
        std::cout << "Received samples from FrontLeft and FrontRight. Sum of " << leftCache->counter << " + "
                  << rightCache->counter << " = " << leftCache->counter + rightCache->counter << std::endl;
        leftCache.reset();
        rightCache.reset();
    }
```

### Additional context data for callbacks (ice_callbacks_listener_as_class_member.cpp)

Here we demonstrate how you can provide virtually everything as an additional argument to the callbacks.
You just have to provide a reference to a value as additional argument in the `attachEvent` method 
which is then provided as argument in your callback. One of the use cases is to get access 
to members and methods of an object inside a static method which we demonstrate here.

This example is identical to the [ice_callbacks_subscriber.cpp](#ice_callbacks_subscriber.cpp) 
one, except that we left out the cyclic heartbeat trigger. The key difference is that 
the listener is now a class member and in every callback we would like to change 
some member variables. To do this we require an additional pointer to the object 
since the listener requires c function references which do not allow the usage
of lambdas with capturing. Here we can use the userType feature which allows us 
to provide the this pointer as additional argument to the callback.

The main function is now pretty short, we instantiate our object of type `CounterService`
and call `waitForShutdown` which uses the `shutdownSemaphore` like in the 
previous example to wait for the control c event from the user.
```cpp
auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

iox::runtime::PoshRuntime::initRuntime(APP_NAME);

CounterService counterService;

counterService.waitForShutdown();
```

Our `CounterService` has the following members:
```cpp
class CounterService {
    //...
    iox::popo::Subscriber<CounterTopic> m_subscriberLeft;
    iox::popo::Subscriber<CounterTopic> m_subscriberRight;
    iox::cxx::optional<CounterTopic> m_leftCache;
    iox::cxx::optional<CounterTopic> m_rightCache;
    iox::popo::Listener m_listener;
};
```

And their purposes are the same as in the previous example. In the constructor
we initialize the two subscribers and attach them to our listener. But now we 
add an additional parameter in the `iox::popo::createNotificationCallback`, the 
dereferenced `this` pointer. It has to be dereferenced since we require a reference 
as argument.

!!! attention 
    The user has to ensure that the contextData (`*this`) in `attachEvent` 
    lives as long as the attachment, with its callback, is attached otherwise 
    the callback context data pointer is dangling.

```cpp
CounterService()
    : m_subscriberLeft({"Radar", "FrontLeft", "Counter"})
    , m_subscriberRight({"Radar", "FrontRight", "Counter"})
{
    /// Attach the static method onSampleReceivedCallback and provide "*this" as additional argument
    /// to the callback to gain access to the object whenever the callback is called.
    /// It is not possible to use a lambda with capturing here since they are not convertable to
    /// a C function pointer.
    m_listener
        .attachEvent(m_subscriberLeft,
                     iox::popo::SubscriberEvent::DATA_RECEIVED,
                     iox::popo::createNotificationCallback(onSampleReceivedCallback, *this))
        .or_else([](auto) {
            std::cerr << "unable to attach subscriberLeft" << std::endl;
            std::exit(EXIT_FAILURE);
        });
    m_listener
        .attachEvent(m_subscriberRight,
                     iox::popo::SubscriberEvent::DATA_RECEIVED,
                     iox::popo::createNotificationCallback(onSampleReceivedCallback, *this))
        .or_else([](auto) {
            std::cerr << "unable to attach subscriberRight" << std::endl;
            std::exit(EXIT_FAILURE);
        });
}
```

The `onSampleReceivedCallback` is now a static method instead of a free function. It 
has to be static since we require a C function reference as callback argument and a 
static method can be converted into such a type. But in a static method we do not 
have access to the members of an object, therefore we have to add an additional 
argument, the pointer to the object itself, called `self`. 
```cpp
static void onSampleReceivedCallback(iox::popo::Subscriber<CounterTopic>* subscriber, 
                                     CounterService* self)
{
    subscriber->take().and_then([subscriber, self](auto& sample) {
        auto instanceString = subscriber->getServiceDescription().getInstanceIDString();

        // store the sample in the corresponding cache
        if (instanceString == iox::capro::IdString_t("FrontLeft"))
        {
            self->m_leftCache.emplace(*sample);
        }
        else if (instanceString == iox::capro::IdString_t("FrontRight"))
        {
            self->m_rightCache.emplace(*sample);
        }

        std::cout << "received: " << sample->counter << std::endl;
    });

    // if both caches are filled we can process them
    if (self->m_leftCache && self->m_rightCache)
    {
        std::cout << "Received samples from FrontLeft and FrontRight. Sum of " << self->m_leftCache->counter
                  << " + " << self->m_rightCache->counter << " = "
                  << self->m_leftCache->counter + self->m_rightCache->counter << std::endl;
        self->m_leftCache.reset();
        self->m_rightCache.reset();
    }
}
```


<center>
[Check out callbacks on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/callbacks){ .md-button }
</center>
