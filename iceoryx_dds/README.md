
# Gateway to DDS Networks
A gateway for bridging between iceoryx systems and DDS networks.
The gateway enables iceoryx systems running on separate hosts to communicate with each other.

i.e. Data published by a publisher on `Host A` can be received by a matching subscriber on `Host B`.

# Organization
This module exports the following executables:
* `iox-gw-iceoryx2dds`
* `iox-gw-dds2iceoryx`

Each executable manages a POSH runtime that runs the gateway logic for a single direction of communication.

The common building blocks logic for these binaries are consolidated in the exported library, `libiceoryx_dds`.

Applications may instead directly embed the gateway by using the exported lib.

# Building
The DDS stack used by the gateway is abstracted and needs to made explicit at compile time. 

## Pre-requisites
* Java is installed
* Maven is installed
* CMake is installed

```bash
sudo apt install cmake maven openjdk-14-jdk-headless
```

## CMake Build 
You can use the standard iceoryx cmake build approach with an activated `-DDDS_GATEWAY=ON`
switch.
```sh
cmake -Bbuild -Hiceoryx_meta -DDDS_GATEWAY=ON
cd build
make
```

# Usage
## Configuration
In `/etc/iceoryx/gateway_config.toml` you find the dds gateway configuration.
Every service which should be offered or to which you would like to 
subscribe has to be listed in here.
```toml
[[services]]
service     = "Radar"
instance    = "FrontLeft"
event       = "Object"

[[services]]
service     = "larry_robotics"
instance    = "SystemMonitor"
event       = "larry_info"
```
In this example we would like to offer or subscribe to the two services
`Radar.FrontLeft.Object` from our [icedelivery example](../iceoryx_examples/icedelivery)
and to one service `larry_robotics.SystemMonitor.larry_info` from our 
[larry demonstrator](https://gitlab.com/larry.robotics/larry.robotics).

## Running icedelivery via CycloneDDS
We can use CycloneDDS to run our [icedelivery example](../iceoryx_examples/icedelivery) 
via a local area network. First we have to adjust the gateway configuration file 
in `/etc/iceoryx/gateway_config.toml` and have to add the publisher service description
from our example.
```toml
[[services]]
service     = "Radar"
instance    = "FrontLeft"
event       = "Object"
```

Now two connected machines `A` and `B` can communicate over a local area network 
via iceoryx.

Open three terminals on machine `A` and execute the following commands:

- Terminal 1: `./build/iox-roudi`
- Terminal 2: `./build/iceoryx_dds/iox-gw-iceoryx2dds` to send all samples from the publisher to DDS
- Terminal 3: `./build/iceoryx_examples/icedelivery/iox-cpp-publisher`

Open another three terminals on machine `B` and execute the commands:

- Terminal 1: `./build/iox-roudi`
- Terminal 2: `./build/iceoryx_dds/iox-gw-dds2iceoryx` to receive all samples from the publisher via DDS
- Terminal 3: `./build/iceoryx_examples/icedelivery/iox-cpp-subscriber`

If you would like to have a bidirectional communication just run `iox-gw-dds2iceoryx` and 
`iox-gw-iceoryx2dds` on the same machine.

## Running with shared libraries
Before running, you may need to add the install directory to the library load path if it is not standard (so that the runtime dependencies can be found).
i.e.
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_DIR/lib
```

Then, simply run the gateway executables as desired.

e.g.
```bash
$INSTALL_DIR/bin/iox-gw-iceoryx2dds
$INSTALL_DIR/bin/iox-gw-dds2iceoryx
```


