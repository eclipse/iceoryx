# Single process

## Introduction

This example demonstrates how iceoryx can be used in a single process for
inter-thread communication. This is for instance helpful if you would like
to establish a simple communication API in a 3D engine with a multitude of
threads that are interacting without starting RouDi every time separately.

## Run singleprocess

[![asciicast](https://asciinema.org/a/407439.svg)](https://asciinema.org/a/407439)

The first lines until `RouDi is ready for clients` are coming from the RouDi
startup in which the shared memory management segment and user data segment are
created.

Afterward, the publisher and subscriber thread are started and are beginning to
transmit and receive data.

## Code Walkthrough

### Creating a Single Process RouDi, Publisher and Subscriber

 1. We start by setting the log level to error since we do not want to see all the
    debug messages.

    ```cpp
    int main()
    {
        // set the log level to error to see the essence of the example
        iox::log::LogManager::GetLogManager().SetDefaultLogLevel(iox::log::LogLevel::kError);
    ```

 2. To start RouDi we have to create a configuration for him. We are choosing the
    default config. Additionally, RouDi needs some other components like a memory
    management unit which handles how the memory is created in which the transmission
    data is stored. The `IceOryxRouDiComponents` class is handling them for us

    ```cpp
    iox::RouDiConfig_t defaultRouDiConfig = iox::RouDiConfig_t().setDefaults();
    iox::roudi::IceOryxRouDiComponents roudiComponents(defaultRouDiConfig);
    ```

 3. We are starting RouDi and provide him with the required components. Furthermore, we
    disable monitoring. The last bool parameter `false` states that RouDi does not
    terminate all registered processes when he goes out of scope. If we would set it
    to `true`, our application would self terminate when the destructor is called.

    ```cpp
    iox::roudi::RouDi roudi(roudiComponents.m_rouDiMemoryManager,
                            roudiComponents.m_portManager,
                            iox::roudi::RouDi::RoudiStartupParameters{iox::roudi::MonitoringMode::OFF, false});
    ```

 4. Here comes a key difference to an inter-process application. If you would like
    to communicate within one process, you have to use `PoshRuntimeSingleProcess`.
    You can create only one runtime at a time!

    ```cpp
    iox::runtime::PoshRuntimeSingleProcess runtime("singleProcessDemo");
    ```

 5. Now that everything is up and running, we can start the publisher and subscriber
    thread, wait two seconds and stop the example.

    ```cpp
    std::thread publisherThread(publisher), subscriberThread(subscriber);

    // communicate for 2 seconds and then stop the example
    std::this_thread::sleep_for(std::chrono::seconds(2));
    keepRunning.store(false);

    publisherThread.join();
    subscriberThread.join();

    std::cout << "Finished" << std::endl;
    ```

### Implementation of Publisher and Subscriber

Since there are no differences to inter-process ports you can take a look at the
[icedelivery example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery)
for detailed documentation. We only provide you here with a short overview.

#### Publisher

We create a typed publisher with the following service description
(Service = `Single`, Instance = `Process`, Event = `Demo`)

```cpp
iox::popo::PublisherOptions publisherOptions;
publisherOptions.historyCapacity = 10U;
iox::popo::Publisher<TransmissionData_t> publisher({"Single", "Process", "Demo"}, publisherOptions);
```

After that, we are sending numbers in ascending order with a 100ms interval in a `while` loop till the
variable `keepRunning` is false.

```cpp
uint64_t counter{0};
std::string greenRightArrow("\033[32m->\033[m ");
while (keepRunning.load())
{
    publisher.loan().and_then([&](auto& sample) {
        sample->counter = counter++;
        consoleOutput(std::string("Sending   " + greenRightArrow + std::to_string(sample->counter)));
        sample.publish();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

#### Subscriber

Like with the publisher, we are creating a corresponding subscriber port with the
same service description.

```cpp
    iox::popo::SubscriberOptions options;
    options.queueCapacity = 10U;
    options.historyRequest = 5U;
    iox::popo::Subscriber<TransmissionData_t> subscriber({"Single", "Process", "Demo"}, options);
```

Now we can receive the data in a while loop till `keepRunning` is false. But we
only try to acquire data if our `SubscribeState` is `SUBSCRIBED`.

```cpp
std::string orangeLeftArrow("\033[33m<-\033[m ");
while (keepRunning.load())
{
    if (iox::SubscribeState::SUBSCRIBED == subscriber.getSubscriptionState())
    {
        bool hasMoreSamples{true};

        do
        {
            subscriber.take()
                .and_then([&](iox::popo::Sample<const TransmissionData_t>& sample) {
                    consoleOutput(std::string("Receiving " + orangeLeftArrow + std::to_string(sample->counter)));
                })
                .if_empty([&] { hasMoreSamples = false; })
                .or_else([](auto) { std::cout << "Error receiving sample: " << std::endl; });
        } while (hasMoreSamples);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

<center>
[Check out singleprocess on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/singleprocess){ .md-button }
</center>
