# iceensemble

## Introduction

A common use case is that multiple sensors are sending data of the same type, e.g. LIDAR data, and a subscriber is interested in the data of all those sensors.

This example demonstrates

1. how you can run multiple publisher applications publishing on the same topic (n:m communication)
2. how to communicate between C and C++

!!! info
    The default communication policy is many to many. If you have built iceoryx with the CMake flag `-DONE_TO_MANY_ONLY` this example will not run.

## Run iceensemble

The easiest way is to build all examples via `./tools/iceoryx_build_test.sh`. Then, create eight terminals and run one command in each of them.

```sh
./build/iox-roudi

./build/iceoryx_examples/icehello/iox-cpp-publisher-helloworld
./build/iceoryx_examples/icedelivery/iox-cpp-publisher
./build/iceoryx_examples/icedelivery/iox-cpp-publisher-untyped
./build/iceoryx_examples/iceoptions/iox-cpp-publisher-with-options
./build/iceoryx_examples/icedelivery_in_c/iox-c-publisher

./build/iceoryx_examples/icedelivery/iox-cpp-subscriber
./build/iceoryx_examples/iceoptions/iox-subscriber-with-options
```

Alternatively, you can use the provided [tmux](https://en.wikipedia.org/wiki/Tmux) script.

```sh
./iceoryx_examples/iceensemble/run_iceensemble.sh
```

!!! info
    As `iox-subscriber-with-options` requests the blocking publisher feature, although having the same `capro::ServiceDescription` `{"Radar", "FrontLeft", "Object"}` it is only connected to `iox-publisher-with-options`

## Expected Output

[![asciicast](https://asciinema.org/a/407432.svg)](https://asciinema.org/a/407432)

<center>
[Check out iceensemble on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/iceensemble){ .md-button }
</center>
