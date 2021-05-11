# user-header

## Introduction

This example shows how to leverage the user-header to send custom meta-information with each sample.
Specifically, we want to send a timestamp alongside the sample.
The example contains code for the typed and untyped C++ API as well as for the C language binding.

## Expected Output

[![asciicast](https://asciinema.org/a/410691.svg)](https://asciinema.org/a/410691)

## Code walkthrough

The examples uses the user-header and user-payload which is defined in `user_header_and_payload_types.hpp`
for the C++ API and in `user_header_and_payload_types.h` for the C API. The user-header consists of a simple `uint64_t`
which is used to transmit a timestamp and the user-payload is a Fibonacci number.

This are the definitions for the C++ API:
<!-- [geoffrey] [iceoryx_examples/user_header/user_header_and_payload_types.hpp] [user-header] -->
```cpp
struct Header
{
    uint64_t publisherTimestamp{0};
};
```

<!-- [geoffrey] [iceoryx_examples/user_header/user_header_and_payload_types.hpp] [user-payload] -->
```cpp
struct Data
{
    uint64_t fibonacci{0};
};
```

This are the definitions for the C API:
<!-- [geoffrey] [iceoryx_examples/user_header/user_header_and_payload_types.h] [user-header] -->
```c
typedef struct
{
    uint64_t publisherTimestamp;
} Header;
```

<!-- [geoffrey] [iceoryx_examples/user_header/user_header_and_payload_types.h] [user-payload] -->
```c
typedef struct
{
    uint64_t fibonacci;
} Data;
```

### Publisher Typed C++ API

At first, there are includes for the user-header and user-payload types
and the iceoryx includes for publisher, runtime and signal handling.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [iceoryx includes] -->
```cpp
#include "user_header_and_payload_types.hpp"

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"
```

The `main` function contains a loop which periodically sends a Fibonacci number.
In order to break this loop, a signal handler is used which sets an atomic boolean.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [signal handling] -->
```cpp
std::atomic<bool> killswitch{false};

static void sigHandler(int sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    killswitch = true;
}
```

The signal handler is registered in the `main` function.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [register sigHandler] -->
```cpp
auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);
```

Next, the iceoryx runtime is initialized. With this call, the application will be registered at `RouDi`,
the routing and discovery daemon.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [initialize runtime] -->
```cpp
constexpr char APP_NAME[] = "iox-cpp-user-header-publisher";
iox::runtime::PoshRuntime::initRuntime(APP_NAME);
```

Now, we create the publisher. Unlike the other examples, this uses the second template parameter to define the user-header.
This is the only change compared to the other examples with the typed C++ API.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [create publisher] -->
```cpp
// for the 1.0 release, the Publisher alias for the PublisherImpl does not have the second parameter for the Header,
// therefore the PublisherImpl must be used directly
iox::popo::Publisher<Data, Header> publisher({"Example", "User-Header", "Timestamp"});
```

In the main loop, a Fibonacci sequence is created and every second a number is published.
The Fibonacci number is passed to the `loan` method which construct the `Data` object if the loaning was successful.
This example uses the functional approach with `and_then` and `or_else`. Please have a look at the `icehello` example
if you prefer a more traditional approach.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [[send samples in a loop] [loan sample]] -->
```cpp
uint64_t timestamp = 42;
uint64_t fibonacciLast = 0;
uint64_t fibonacciCurrent = 1;
while (!killswitch)
{
    auto fibonacciNext = fibonacciCurrent + fibonacciLast;
    fibonacciLast = fibonacciCurrent;
    fibonacciCurrent = fibonacciNext;

    publisher.loan(Data{fibonacciCurrent})
        .and_then([&](auto& sample) {
            // ...
        })
        .or_else([&](auto& error) {
            // ...
        });

    constexpr uint64_t MILLISECONDS_SLEEP{1000U};
    std::this_thread::sleep_for(std::chrono::milliseconds(MILLISECONDS_SLEEP));
    timestamp += MILLISECONDS_SLEEP;
}
```

When the loaning was successful, we get a sample and can access it in the `and_then` branch.
The sample has a `getUserHeader` method which returns a reference to the user-header we specified with the template parameter.
In this case it's the `Header` struct and we set the `publisherTimestamp`.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [loan was successful] -->
```cpp
sample.getUserHeader().publisherTimestamp = timestamp;
sample.publish();

std::cout << APP_NAME << " sent data: " << fibonacciCurrent << " with timestamp " << timestamp << "ms"
          << std::endl;
```

If the loaning fails, the `or_else` branch is executed, which prints an error message
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_cxx_api.cpp] [loan failed] -->
```cpp
std::cout << APP_NAME << " could not loan sample! Error code: " << static_cast<uint64_t>(error)
          << std::endl;
```

### Publisher Untyped C++ API

The example with the untyped C++ publisher is quite similar to the one with the typed publisher
so only the differences will be discussed in this section.

At first there is a different include
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_untyped_cxx_api.cpp] [include differs from typed C++ API] -->
```cpp
#include "iceoryx_posh/popo/untyped_publisher.hpp"
```

When the publisher is created, there is also no notion of a user-header and it looks exactly the same like already shown in other examples.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_untyped_cxx_api.cpp] [create publisher] -->
```cpp
iox::popo::UntypedPublisher publisher({"Example", "User-Header", "Timestamp"});
```

Variations come again into play when the chunk is loaned. Since the API is untyped,
the parameter for the user-header have to be provided with the `loan` call. These are optional parameter and
are set to values indicating no user-header by default.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_untyped_cxx_api.cpp] [[loan chunk]] -->
```cpp
publisher.loan(sizeof(Data), alignof(Data), sizeof(Header), alignof(Header))
    .and_then([&](auto& userPayload) {
        // ...
    })
    .or_else([&](auto& error) {
        // ...
    });
```

Lastly, since the untyped C++ API returns a `void` pointer to the user-payload, there is an intermediate step
to access the user-header by obtaining a `ChunkHeader` first, which provides a method to get the user-header.
This also return a `void` pointer, which makes a cast to the actual type necessary.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_untyped_cxx_api.cpp] [loan was successful] -->
```cpp
auto header = static_cast<Header*>(iox::mepoo::ChunkHeader::fromUserPayload(userPayload)->userHeader());
header->publisherTimestamp = timestamp;

auto data = static_cast<Data*>(userPayload);
data->fibonacci = fibonacciCurrent;

publisher.publish(userPayload);

std::cout << APP_NAME << " sent data: " << fibonacciCurrent << " with timestamp " << timestamp << "ms"
          << std::endl;
```

### Publisher C API

The example for the C API is similar to the one in `icedelivery_in_c` and therefore only the user-header related parts are looked into.
The overall structure is the same like in the typed and untyped C++ API counterparts.

The functions to access the user-header are located in the following include
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_c_api.c] [additional include for user-header] -->
```cpp
#include "iceoryx_binding_c/chunk.h"
```

Similar to the untyped C++ API, the user-header parameter are specified with the loan function.
Since C does not have overloading, this is done by a different function
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_c_api.c] [[loan chunk]] -->
```cpp
void* userPayload;
const uint32_t ALIGNMENT = 8;
enum iox_AllocationResult res = iox_pub_loan_aligned_chunk_with_user_header(
    publisher, &userPayload, sizeof(Data), ALIGNMENT, sizeof(Header), ALIGNMENT);
```

Like with the untyped C++ API, the path to the user-header needs an intermediate step with the `iox_chunk_header_t` and
explicit casting to since the functions return `void` pointer.
<!-- [geoffrey] [iceoryx_examples/user_header/publisher_c_api.c] [[loan was successful]] -->
```cpp
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
```

### Subscriber Typed C++ API

The boilerplate code for the subscriber is the same like for the publisher, therefore only the specific subscriber code is discussed in this section.

To use the subscriber, `subscriber.hpp` needs to be included.
Similar to the publisher, the subscriber has the same additional template parameter compared to use case without the user-header.
<!-- [geoffrey] [iceoryx_examples/user_header/subscriber_cxx_api.cpp] [create subscriber] -->
```cpp
// for the 1.0 release, the Subscriber alias for the SubscriberImpl does not have the second parameter for the
// Header, therefore the SubscriberImpl must be used directly
iox::popo::Subscriber<Data, Header> subscriber({"Example", "User-Header", "Timestamp"});
```

The main loop is quite simple. The publisher is periodically polled and the data of the received sample is printed.
Again, the user-header is accessed by the dedicated method of the sample.
<!-- [geoffrey] [iceoryx_examples/user_header/subscriber_cxx_api.cpp] [poll subscriber for samples in a loop] -->
```cpp
while (!killswitch)
{
    subscriber.take().and_then([&](auto& sample) {
        std::cout << APP_NAME << " got value: " << sample->fibonacci << " with timestamp "
                  << sample.getUserHeader().publisherTimestamp << "ms" << std::endl;
    });

    constexpr std::chrono::milliseconds SLEEP_TIME{100U};
    std::this_thread::sleep_for(SLEEP_TIME);
}
```

### Subscriber Untyped C++ API

On the subscriber side, the typed and untyped examples are even closer in similarity than the publisher example.
The notable difference is the `take` method, which returns a `void` pointer to the user-payload.
Like with the untyped publisher, in order to get the user-header the `ChunkHeader` must be obtained.
Contrary to the untyped publisher, we must cast to a `const T*` type, like `const Header*`
since we are not allowed to mutate the header from a subscriber.
At the end, the chunk must be released to prevent chunk leaks. This is done by calling the `release` method with user-payload pointer.
<!-- [geoffrey] [iceoryx_examples/user_header/subscriber_untyped_cxx_api.cpp] [take chunk] -->
```cpp
subscriber.take().and_then([&](auto& userPayload) {
    auto header =
        static_cast<const Header*>(iox::mepoo::ChunkHeader::fromUserPayload(userPayload)->userHeader());

    auto data = static_cast<const Data*>(userPayload);

    std::cout << APP_NAME << " got value: " << data->fibonacci << " with timestamp "
              << header->publisherTimestamp << "ms" << std::endl;

    subscriber.release(userPayload);
});
```

### Subscriber C API

Finally there is the C API example. Like with the publisher example for the C API we just take a look at the user-header specific parts.
The overall structure is the same like in the typed and untyped C++ API counterparts.

As we already have seen with the untyped C++ example, the call to take the sample is independent of the usage of a user-header.
Likewise, the access to the user-header is done by the intermediate step of getting an `iox_chunk_header_t` first.
Since we are not allowed to mutate the user-header from a subscriber, the respective functions with a `_const` suffix must be used.
At last, the chunk is released in order to prevent chunk leaks.
<!-- [geoffrey] [iceoryx_examples/user_header/subscriber_c_api.c] [take chunk] -->
```cpp
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
```
