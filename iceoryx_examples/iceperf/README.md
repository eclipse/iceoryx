# iceperf

## Introduction

This example measures the latency of IPC transmissions between two applications.
We compare the latency of iceoryx with message queues and unix domain sockets.

The measurement is carried out with several payload sizes. Round trips are performed
for each payload size, using either the default setting or the provided command line parameter
for the number of round trips to do.
The measured time is just allocating/releasing memory and the time to send the data.
The construction and writing of the payload is not part of the measurement.

At the end of the benchmark, the average latency for each payload size is printed.

## Run iceperf

Create three terminals and run one command in each of them.
In this setup the leader is doing the ping pong measurements with the follower.
You can set the number of measurement iterations (number of round trips) with a command line parameter
of iceperf-bench-leader (e.g. `./iceperf-bench-leader -n 100000`).
There are further options which can be printed by calling `./iceperf-bench-leader -h`.
```sh
    # If installed and available in PATH environment variable
    iox-roudi
    # If build from scratch with script in tools
    $ICEORYX_ROOT/build/install/prefix/bin/iox-roudi

    build/iceoryx_examples/iceperf/iceperf-bench-follower

    build/iceoryx_examples/iceperf/iceperf-bench-leader
```

If you would like to test only the C++ API or the C API you can start `iceperf-bench-leader`
with the parameter `-t iceoryx-cpp-api` or `-t iceoryx-c-api`.
```sh
    build/iceoryx_examples/iceperf/iceperf-bench-follower

    build/iceoryx_examples/iceperf/iceperf-bench-leader -n 100000 -t iceoryx-cpp-api
```

## Expected Output

The numbers will differ depending on parameters and the performance of the hardware.
Which technologies are measured depends on the operating system (e.g. no message queue on MacOS).
Here an example output with Ubuntu 18.04 on Intel(R) Xeon(R) CPU E3-1505M v5 @ 2.80GHz.

<!-- @todo Replace this with asciinema recording before v1.0 -->

### iceperf-bench-leader Application

    ******   MESSAGE QUEUE    ********
    Waiting for: subscription, subscriber [ success ]
    Measurement for: 1 kB, 2 kB, 4 kB, 8 kB, 16 kB, 32 kB, 64 kB, 128 kB, 256 kB,
    512 kB, 1024 kB, 2048 kB, 4096 kB
    Waiting for: unsubscribe  [ finished ]

    #### Measurement Result ####
    100000 round trips for each payload.

    | Payload Size [kB] | Average Latency [µs] |
    |------------------:|---------------------:|
    |                 1 |                  3.1 |
    |                 2 |                  3.2 |
    |                 4 |                  3.8 |
    |                 8 |                  5.2 |
    |                16 |                  7.7 |
    |                32 |                   13 |
    |                64 |                   23 |
    |               128 |                   43 |
    |               256 |                   81 |
    |               512 |              1.6e+02 |
    |              1024 |                3e+02 |
    |              2048 |              5.9e+02 |
    |              4096 |              1.2e+03 |

    Finished!

    ****** UNIX DOMAIN SOCKET ********
    Waiting for: subscription, subscriber [ success ]
    Measurement for: 1 kB, 2 kB, 4 kB, 8 kB, 16 kB, 32 kB, 64 kB, 128 kB, 256 kB,
    512 kB, 1024 kB, 2048 kB, 4096 kB
    Waiting for: unsubscribe  [ finished ]

    #### Measurement Result ####
    100000 round trips for each payload.

    | Payload Size [kB] | Average Latency [µs] |
    |------------------:|---------------------:|
    |                 1 |                  4.3 |
    |                 2 |                  4.3 |
    |                 4 |                  4.6 |
    |                 8 |                    6 |
    |                16 |                  8.7 |
    |                32 |                   14 |
    |                64 |                   27 |
    |               128 |                   53 |
    |               256 |              1.1e+02 |
    |               512 |              2.1e+02 |
    |              1024 |              4.2e+02 |
    |              2048 |              8.4e+02 |
    |              4096 |              1.7e+03 |

    Finished!

    ******      ICEORYX       ********
    Waiting for: subscription, subscriber [ success ]
    Measurement for: 1 kB, 2 kB, 4 kB, 8 kB, 16 kB, 32 kB, 64 kB, 128 kB, 256 kB,
    512 kB, 1024 kB, 2048 kB, 4096 kB
    Waiting for: unsubscribe  [ finished ]

    #### Measurement Result ####
    100000 round trips for each payload.

    | Payload Size [kB] | Average Latency [µs] |
    |------------------:|---------------------:|
    |                 1 |                 0.73 |
    |                 2 |                 0.58 |
    |                 4 |                 0.61 |
    |                 8 |                 0.61 |
    |                16 |                 0.59 |
    |                32 |                 0.62 |
    |                64 |                  0.6 |
    |               128 |                 0.58 |
    |               256 |                 0.61 |
    |               512 |                 0.61 |
    |              1024 |                 0.58 |
    |              2048 |                 0.61 |
    |              4096 |                 0.61 |

    Finished!

    ******   ICEORYX C API    ********
    Waiting for: subscription, subscriber [ success ]
    Measurement for: 1 kB, 2 kB, 4 kB, 8 kB, 16 kB, 32 kB, 64 kB, 128 kB, 256 kB,
    512 kB, 1024 kB, 2048 kB, 4096 kB
    Waiting for: unsubscribe  [ finished ]

    #### Measurement Result ####
    100000 round trips for each payload.

    | Payload Size [kB] | Average Latency [µs] |
    |------------------:|---------------------:|
    |                 1 |                 0.73 |
    |                 2 |                 0.58 |
    |                 4 |                 0.61 |
    |                 8 |                 0.61 |
    |                16 |                 0.59 |
    |                32 |                 0.62 |
    |                64 |                  0.6 |
    |               128 |                 0.58 |
    |               256 |                 0.61 |
    |               512 |                 0.61 |
    |              1024 |                 0.58 |
    |              2048 |                 0.61 |
    |              4096 |                 0.61 |

    Finished!

### iceperf-bench-follower Application

    ******   MESSAGE QUEUE    ********
    registering with the leader

    ****** UNIX DOMAIN SOCKET ********
    registering with the leader

    ******      ICEORYX       ********
    Waiting for: subscription, subscriber [ success ]
    Waiting for: unsubscribe  [ finished ]

    ******   ICEORYX C API    ********
    Waiting for: subscription, subscriber [ success ]
    Waiting for: unsubscribe  [ finished ]

## Code Walkthrough

Here we roughly describe the setup for performing the measurements in `iceperf_bench_leader.hpp/cpp` and `iceperf_bench_follower.hpp/cpp`. Things like initialization, sending and receiving of data are technology specific and can be found in the respective files (e.g. uds.cpp for
unix domain socket). Our focus here is on the abstraction layer on top which allows us or you to add new IPC technologies to extend and compare them.

### iceperf-bench-leader Application

Besides includes for the different IPC technologies, the `topic_data.hpp` file is included which contains the `PerSettings` and `PerTopic` structs, which are used to transfer some information between the applications. Independent of the real payload size, the `PerTopic` struct is used as some kind of header in each transferred sample.

<!-- [geoffrey] [iceoryx_examples/iceperf/topic_data.hpp] [topic data definitions] -->
```cpp
struct PerfSettings
{
    Benchmark benchmark{Benchmark::ALL};
    Technology technology{Technology::ALL};
    uint64_t numberOfSamples{10000U};
};

struct PerfTopic
{
    uint32_t payloadSize{0};
    uint32_t subPackets{0};
    RunFlag runFlag{RunFlag::RUN};
};
```

The `PerfSettings` struct is used to synchronize the settings between the leader and the follower application.

The `PerfTopic` struct is used to share some information during the measurement.
With `payloadSize` as the payload size used for the current measurement. In case it is not possible to transfer the `payloadSize` with a single data transfer (e.g. OS limit for the payload of a single socket send), the payload is divided into several sub-packets. This is indicated with `subPackets`. The `runFlag` is used to shutdown the iceperf-bench follower at the end of the benchmark.

Let's use some constants to prevent magic values and set and names for the communication resources that are used.
<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_leader.cpp] [use constants instead of magic values] -->
```c++
constexpr const char APP_NAME[]{"iceperf-bench-leader"};
constexpr const char PUBLISHER[]{"Leader"};
constexpr const char SUBSCRIBER[]{"Follower"};
```

The `IcePerfLeader` c'tor does a cleanup of potentially outdated resources of technologies
which might have left some resources in the file system after an abnormal terminations.
<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_leader.cpp] [cleanup outdated resources] -->
```cpp
#ifndef __APPLE__
MQ::cleanupOutdatedResources(PUBLISHER, SUBSCRIBER);
#endif
UDS::cleanupOutdatedResources(PUBLISHER, SUBSCRIBER);
```

The `doMeasurement()` method executes a measurement for the provided IPC technology the and number of round trips.
For being able to always perform the same steps and avoiding code duplications,
we use a base class with technology independent functionality and the technology has to implement the technology dependent part.

<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_leader.cpp] [do the measurement for a single technology] -->
```cpp
void IcePerfLeader::doMeasurement(IcePerfBase& ipcTechnology) noexcept
{
    ipcTechnology.initLeader();

    std::vector<std::tuple<uint32_t, iox::units::Duration>> latencyMeasurements;
    const std::vector<uint32_t> payloadSizesInKB{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    std::cout << "Measurement for:";
    const char* separator = " ";
    for (const auto payloadSizeInKB : payloadSizesInKB)
    {
        std::cout << separator << payloadSizeInKB << " kB" << std::flush;
        separator = ", ";
        auto payloadSizeInBytes = payloadSizeInKB * IcePerfBase::ONE_KILOBYTE;

        ipcTechnology.preLatencyPerfTestLeader(payloadSizeInBytes);

        auto latency = ipcTechnology.latencyPerfTestLeader(m_settings.numberOfSamples);

        latencyMeasurements.push_back(std::make_tuple(payloadSizeInKB, latency));

        ipcTechnology.postLatencyPerfTestLeader();
    }
    std::cout << std::endl;

    ipcTechnology.releaseFollower();

    ipcTechnology.shutdown();

    std::cout << std::endl;
    std::cout << "#### Measurement Result ####" << std::endl;
    std::cout << m_settings.numberOfSamples << " round trips for each payload." << std::endl;
    std::cout << std::endl;
    std::cout << "| Payload Size [kB] | Average Latency [µs] |" << std::endl;
    std::cout << "|------------------:|---------------------:|" << std::endl;
    for (const auto& latencyMeasuement : latencyMeasurements)
    {
        auto payloadSizeInKB = std::get<0>(latencyMeasuement);
        auto latencyInMicroseconds = static_cast<double>(std::get<1>(latencyMeasuement).toNanoseconds()) / 1000.0;
        std::cout << "| " << std::setw(17) << payloadSizeInKB << " | " << std::setw(20) << std::setprecision(2)
                  << latencyInMicroseconds << " |" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Finished!" << std::endl;
}
```

Initialization is different for each IPC technology. Here we have to create sockets, message queues or iceoryx publisher and subscriber.
With `ipcTechnology.initLeader()` we are setting up these resources on the leader side.
After the definition of the different payload sizes to use, we execute a single round trip measurement for each individual payload size.
The leader has to orchestrate the whole process and has a pre- and post-step for each ping pong round trip measurement.
`ipcTechnology.preLatencyPerfTestLeader(...)` sets the payload size for the upcoming measurement.
`ipcTechnology.latencyPerfTestLeader(m_settings.numberOfSamples)` performs the ping pong between leader and follower and returns
the time it took to transmit the number of samples in a ping pong round trip.  After the measurements were done for all the different payload sizes,
`ipcTechnology.releaseFollower()` releases the follower since it is not aware of things like how many payload sizes are considered.
After cleaning up the communication resources with `ipcTechnology.shutdown()` the results are printed.

In the `run()` method we create instances for the different IPC technologies we want to compare. Each technology is implemented in an own class and implements the pure virtual functions provided with the `IcePerfBase` class. But before this is done, we send the `PerfSettings` to the follower application.

<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_leader.cpp] [[run all technologies] [send setting to follower application]] -->
```cpp
int IcePerfLeader::run() noexcept
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    iox::capro::ServiceDescription serviceDescription{"IcePerf", "Settings", "Generic"};
    iox::popo::PublisherOptions options;
    options.historyCapacity = 1U;
    iox::popo::Publisher<PerfSettings> settingsPublisher{serviceDescription, options};
    if (!settingsPublisher.publishCopyOf(m_settings))
    {
        std::cerr << "Could not send settings to follower!" << std::endl;
        return EXIT_FAILURE;
    }
    // ...
    return EXIT_SUCCESS;
}
```

Now we can create an object for each IPC technology that we want to evaluate and call the `doMeasurement()` method.

<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_leader.cpp] [[run all technologies] [create an run technologies]] -->
```cpp
int IcePerfLeader::run() noexcept
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    // ...
    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::POSIX_MESSAGE_QUEUE)
    {
#ifndef __APPLE__
        std::cout << std::endl << "******   MESSAGE QUEUE    ********" << std::endl;
        MQ mq(PUBLISHER, SUBSCRIBER);
        doMeasurement(mq);
#else
        if (m_settings.technology == Technology::POSIX_MESSAGE_QUEUE)
        {
            std::cout << "The message queue is not supported on macOS and will be skipped!" << std::endl;
        }
#endif
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::UNIX_DOMAIN_SOCKET)
    {
        std::cout << std::endl << "****** UNIX DOMAIN SOCKET ********" << std::endl;
        UDS uds(PUBLISHER, SUBSCRIBER);
        doMeasurement(uds);
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::ICEORYX_CPP_API)
    {
        std::cout << std::endl << "******      ICEORYX       ********" << std::endl;
        Iceoryx iceoryx(PUBLISHER, SUBSCRIBER);
        doMeasurement(iceoryx);
    }

    if (m_settings.technology == Technology::ALL || m_settings.technology == Technology::ICEORYX_C_API)
    {
        std::cout << std::endl << "******   ICEORYX C API    ********" << std::endl;
        IceoryxC iceoryxc(PUBLISHER, SUBSCRIBER);
        doMeasurement(iceoryxc);
    }

    return EXIT_SUCCESS;
}
```

### iceperf_bench_follower Application

The `iceperf-bench-follower` application is similar to `iceperf-bench-leader`. The first change is the `SUBSCRIBER` and `PUBLISHER` switched their names.
<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_follower.cpp] [use constants instead of magic values] -->
```c++
constexpr const char APP_NAME[]{"iceperf-bench-follower"};
constexpr const char PUBLISHER[]{"Follower"};
constexpr const char SUBSCRIBER[]{"Leader"};
```

While the `run()` method of the leader publishes the `PerfSettings`, the follower is subscribed to those settings
and waits for them before the technologies are created, which is again equal to the leader.
<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_follower.cpp] [[run all technologies] [get settings from leader]] -->
```cpp
int IcePerfFollower::run() noexcept
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    iox::capro::ServiceDescription serviceDescription{"IcePerf", "Settings", "Generic"};
    iox::popo::SubscriberOptions options;
    options.historyRequest = 1U;
    iox::popo::Subscriber<PerfSettings> settingsSubscriber{serviceDescription, options};

    m_settings = getSettings(settingsSubscriber);
    // ...
    return EXIT_SUCCESS;
}
```

The `doMeasurement()` method is much simpler than the one from the leader, it reacts only and does not have the control.
Besides `ipcTechnology.initFollower()` and `ipcTechnology.shutdown()` all the functionality to do the ping pong for different payload sizes is done in `ipcTechnology.latencyPerfTestFollower()`

<!-- [geoffrey] [iceoryx_examples/iceperf/iceperf_follower.cpp] [do the measurement for a single technology] -->
```cpp
void IcePerfFollower::doMeasurement(IcePerfBase& ipcTechnology) noexcept
{
    ipcTechnology.initFollower();

    ipcTechnology.latencyPerfTestFollower();

    ipcTechnology.shutdown();
}
```
<center>
[Check out iceperf on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/iceperf){ .md-button }
</center>
