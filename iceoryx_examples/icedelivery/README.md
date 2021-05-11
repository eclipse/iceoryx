# icedelivery

## Introduction

This example showcases a data transmission setup with zero-copy inter-process communication (IPC) on iceoryx.
It provides publisher and subscriber applications. They come in two C++ API flavours (untyped and typed).

## Expected Output

Create three terminals and start RouDi, a publisher and a subscriber. You can also mix the typed and untyped versions.

[![asciicast](https://asciinema.org/a/407359.svg)](https://asciinema.org/a/407359)

## Code walkthrough

This example makes use of two kinds of API flavors. With the untyped API, you have the most flexibility. It enables you
to put APIs on a higher level with a different look and feel on top of iceoryx. E.g. the ara::com API of AUTOSAR Adaptive or
the ROS 2 API. It is not meant to be used by developers in daily life, the assumption is that there will always be a higher
abstraction. A simple example how such an abstraction could look like is given in the second step with the typed
example. The typed API provides type safety combined with [RAII](https://en.cppreference.com/w/cpp/language/raii).

### Publisher application (untyped)

First off, let's include the publisher, the runtime and the signal handler:

```cpp
#include "iceoryx_posh/popo/untyped_publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
```

You might be wondering what the publisher application is sending? It's this struct.

```cpp
struct RadarObject
{
    RadarObject() noexcept
    {
    }
    RadarObject(double x, double y, double z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
```

It is included by:

```cpp
#include "topic_data.hpp"
```

For the communication with RouDi a runtime object is created. The parameter of the method `initRuntime()` contains a
unique string identifier for this publisher.

```cpp
constexpr char APP_NAME[] = "iox-cpp-publisher-untyped";
iox::runtime::PoshRuntime::initRuntime(APP_NAME);
```

Now that RouDi knows our publisher application is existing, let's create a publisher instance for sending our charming
struct to everyone:

```cpp
iox::popo::UntypedPublisher publisher({"Radar", "FrontLeft", "Object"});
```

Now comes the work mode. Data needs to be created. But hang on.. we need memory first! Let's reserve a memory chunk
which fits our RadarObject struct.

```cpp
publisher.loan(sizeof(RadarObject))
    .and_then([&](auto& userPayload)
    {
        // ...
    })
    .or_else([&](iox::popo::AllocationError error)
    {
        // ...
    });
```

The call to `loan()` returns a `cxx::expected`. By concatenating `and_then` and `or_else` branches are implicitly taken
and your code becomes less error-prone compared to using `if() { .. } else { .. }`. Well, it's a bit of a
[lambda](https://en.wikipedia.org/wiki/Anonymous_function#C++_(since_C++11)) jungle. Read it like a story in a book:
"Loan memory and then if it succeeds, fill it with some data or else if it fails, handle the error".

Remember, the untyped API will always be bare-metal!

Hence, the `RadarObject` needs to be constructed with a placement new:

```cpp
RadarObject* data = new (userPayload) RadarObject(ct, ct, ct);
```

Then, we can write some values:

```cpp
data->x = 1.0;
data->y = 2.0;
data->z = 3.0;
```

And finally, in both ways, the value is made available to other subscribers with

```cpp
publisher.publish(userPayload);
```

The incrementation and sending of the data is done in a loop every second till the user pressed `Ctrl-C`. It is
captured with the signal handler and stops the loop.

### Subscriber application (untyped)

How can the subscriber application receive the data the publisher application just transmitted?

Similar to the publisher, we include the topic data, the subscriber, the runtime as well as the signal handler header:

```cpp
#include "topic_data.hpp"

#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
```

To make RouDi aware of the subscriber a runtime object is created, once again with a unique identifier string:

```cpp
constexpr char APP_NAME[] = "iox-cpp-subscriber-untyped";
iox::runtime::PoshRuntime::initRuntime(APP_NAME);
```

In the next step a subscriber object is created, matching exactly the `capro::ServiceDescription` that the publisher
offered:

```cpp
iox::popo::UntypedSubscriber subscriber({"Radar", "FrontLeft", "Object"});
```

When using the default n:m communication philosophy, the `SubscriptionState` is immediately `SUBSCRIBED`.
However, when restricting iceoryx to the 1:n communication philosophy before being in the state `SUBSCRIBED`, the state is changed to `SUBSCRIBE_REQUESTED`.

Again in a while-loop we do the following:

```cpp
while (!killswitch)
{
    subscriber.take()
        .and_then([](const void* userPayload)
        {
            // ...
        })
        .or_else([](auto& result)
        {
            if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
            {
                std::cout << "Error receiving chunk." << std::endl;
            }
        });
```

The `killswitch` will be used to stop the program execution.

Let's translate it into a story again: "Take the data and then if this succeeds, work with the sample, or else if an
error other than `NO_CHUNK_AVAILABLE` occurred, print the string 'Error receiving chunk.'" Of course, you don't need to
take care of all cases, but we advise doing so.

In the `and_then` case the content of the sample is printed to the command line:

```cpp
auto object = static_cast<const RadarObject*>(userPayload);
std::cout << APP_NAME << " got value: " << object->x << std::endl;
```

Please note the `static_cast` before reading out the data. It is necessary because the untyped subscriber is unaware
of the type of the transmitted data.

After accessing the value, the chunk of memory needs to be explicitly released by calling:

```cpp
subscriber.release(userPayload);
```

The subscriber runs 10x times faster than the publisher, to make sure that all data samples are received.

### Publisher application (typed)

The typed publisher application is an example for a high-level user API and does the same thing as the untyped
publisher described before. In this summary, just the differences to the prior publisher application are described.

Starting again with the includes, there is now a different one:

```cpp
#include "iceoryx_posh/popo/publisher.hpp"
```

When it comes to the runtime, things are the same as in the untyped publisher. However, a typed publisher object is
created and the transmitted data type is provided as template parameter:

```cpp
iox::popo::Publisher<RadarObject> publisher({"Radar", "FrontLeft", "Object"});
```

A similar while-loop is used to send the data to the subscriber. In contrast to the untyped publisher the typed one
offers three additional possibilities.

#### #1 Loan and publish

Usage #1 default constructs the data type in-place:

```cpp
publisher.loan()
        .and_then([&](auto& sample) {
            sample->x = ct;
            sample->y = ct;
            sample->z = ct;
            sample.publish();
        })
        .or_else([](auto& error) {
            // Do something with error
            std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error)
                    << std::endl;
        });
```

#### #2 Loan, construct in-place and publish

Usage #2 constructs the data type with the values provided in loan:

```cpp
publisher.loan(ct, ct, ct).and_then([](auto& sample) { sample.publish(); }).or_else([](auto& error) {
    // Do something with error
    std::cerr << "Unable to loan sample, error code: " << static_cast<uint64_t>(error) << std::endl;
});
```

One might wonder what the type of the variable `sample` is? It is `iox::popo::Sample<RadarObject>`. This class behaves
similar to a [`std::unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr) and makes sure that the ownership
handling is done automatically and memory is freed when going out of scope on subscriber side.

#### #3 Publish by copy

Usage #3 does a copy-and-publish in one call. This should only be used for small data types, as otherwise copies can
lead to a larger runtime.

```cpp
auto object = RadarObject(ct, ct, ct);
publisher.publishCopyOf(object).or_else([](auto& error) {
    // Do something with error.
    std::cerr << "Unable to publishCopyOf, error code: " << static_cast<uint64_t>(error) << std::endl;
});
```

#### #4 Publish the result of a computation

Usage #4 can be useful if you have a callable e.g. a function or
[functor](https://en.wikipedia.org/wiki/Function_object#In_C_and_C++) should be always called. The callable needs
to have the signature `void(SampleType*)`.  What then happens, is the following: The publisher loans a sample from
shared memory and if loaning was successful the callable is called with a pointer to the `SampleType` as first
argument. If loaning was unsuccessful, the callable is not called, but instead the `or_else` branch is taken.

```cpp
publisher.publishResultOf(getRadarObject, ct).or_else([](auto& error) {
    // Do something with error.
    std::cerr << "Unable to publishResultOf, error code: " << static_cast<uint64_t>(error) << std::endl;
});
publisher.publishResultOf([&ct](RadarObject* object) { *object = RadarObject(ct, ct, ct); })
    .or_else([](auto& error) {
        // Do something with error.
        std::cerr << "Unable to publishResultOf, error code: " << static_cast<uint64_t>(error) << std::endl;
    });
```

### Subscriber application (typed)

As with the typed publisher application there is a different include compared to the untyped subscriber:

```cpp
#include "iceoryx_posh/popo/subscriber.hpp"
```

An instance of `Subscriber` is created:

```cpp
iox::popo::Subscriber<RadarObject> subscriber({"Radar", "FrontLeft", "Object"});
```

Everything else is nearly the same. However, there is one crucial difference which makes the `Subscriber` typed.

Compare this line from the `UntypedSubscriber`

```cpp
.and_then([](const void* userPayload)
{
    // ...
})
```

with

```cpp
.and_then([](auto& sample)
{
    // ...
})
```

In case of the typed `Subscriber`, `auto` is deduced to `iox::popo::Sample<const RadarObject>`. With the `UntypedSubscriber` the parameter is `const void*` as no type information is available.

<center>
[Check out icedelivery on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery){ .md-button }
</center>
