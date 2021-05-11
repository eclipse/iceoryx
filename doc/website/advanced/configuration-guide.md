# Configuration guide

## :material-cog: CMake switches for configuring iceoryx_posh build

When building iceoryx_posh, there are several configuration options set by default.
These options adjust the limits of Publisher and Subscriber Ports for resource management. These limits are used to create management structures in the shared memory segment called `iceoryx_mgmt` when starting up RouDi.

 |  switch  |  description |
 |:---------|:-------------|
 | `IOX_MAX_PUBLISHERS` | Maximum number of publishers which can be managed by one `RouDi` instance |
 | `IOX_MAX_SUBSCRIBERS_PER_PUBLISHER` | Maximum number of connected subscriber ports per publisher port |
 | `IOX_MAX_PUBLISHER_HISTORY` | Maximum number of chunks available for the publisher history |
 | `IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY` | Maximum number of chunks a publisher can allocate at a given time |
 | `IOX_MAX_SUBSCRIBERS` | Maximum number of subscribers which can be managed by one `RouDi` instance |
 | `IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY` | Maximum number of chunks a subscriber can hold at a given time (subscriber history size)|
 | `IOX_MAX_INTERFACE_NUMBER` | Maximum number of interface ports which are used for gateways |

Have a look at [iceoryx_posh_deployment.cmake](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_posh/cmake/iceoryx_posh_deployment.cmake) for the default values of the constants.

!!! hint
    With the default values set, the size of `iceoryx_mgmt` is ~64.5 MByte. You can reduce the size by decreasing the values from the table via the CMake options. The current values are printed in the CMake stage when building iceoryx.

Example:

```bash
cmake -Bbuild -Hiceoryx_meta -DIOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY=64
```

With that change, the footprint of the management segment is reduced to ~52.7 MBytes. For larger use cases you can increase the value to avoid that samples are dropped on the subscriber side (see also [#615](https://github.com/eclipse-iceoryx/iceoryx/issues/615)).

## :material-memory: Configuring Mempools for RouDi

RouDi supports several shared memory segments with different access rights, to limit the read and write access between different applications. Inside of these segments reside mempools where the user payload data for transfer is stored.
Based on the [conceptual guide](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/conceptual-guide.md) the end-user may want to configure the mempools with the number of chunks and their size.

!!! note
    Actually only the chunk-payload size is configured and the size of the `ChunkHeader` will be added to the configured size. If a user-header or a user-payload alignment larger than 8 is used, the available size for the user-payload will be smaller than the configured chunk-payload since some space is needed for the other functionality.
    Please have a look at the `chunk_header.md` design document for a formula how to determine the necessary chunk-payload size with user-header and extended user-payload alignment.

For building RouDi, iceoryx ships a library named `iceoryx_posh_roudi`. This lib gives you an API for compiling your own RouDi application and is part of `iceoryx_posh`.

!!! note
    The chunk size for the mempool needs to follow these restrictions:

    1. Chunksize needs to be greater than the alignment
    2. Chunksize needs to be a multiple of the alignment

The value for the alignment is set to 8.

### :material-file-cog: Dynamic configuration

One way is to read a configuration dynamically at RouDi runtime (startup).
Using TOML Config in RouDi is not mandatory for configuring segments and mempools, but a comfortable alternative.

To enable the TOML config in iceoryx, the CMake option `-DTOML_CONFIG=ON` must be used (enabled by default).
The `iox-roudi` provided by iceoryx is with TOML support and can be used out of the box.

If you create your own RouDi application you need to link against `iceoryx_posh_config`:

```cmake
target_link_libraries(custom-roudi
    PRIVATE
    iceoryx_posh::iceoryx_posh_roudi
    iceoryx_posh::iceoryx_posh_config
)
```

The TOML config file can be passed to RouDi with the `-c` command-line option.

```bash
./iox-roudi -c /absolute/path/to/config/file.toml
```

This is a common config file with format version 1:

```TOML
[general]
version = 1

[[segment]]

[[segment.mempool]]
size = 32
count = 10000

[[segment.mempool]]
size = 128
count = 10000

[[segment.mempool]]
size = 1024
count = 1000
```

With this configuration, one payload segment will be created. The access rights are set to the RouDi group id.
There are three mempools within this segment. One with 10000 chunks of 32 byte payload size, one with 10000 chunks of 128 bytes, and one with 1000 chunks of 1024 bytes.

To restrict the access, a reader and writer group can be set:

```TOML
[general]
version = 1

[[segment]]
reader = "foo"
writer = "bar"

[[segment.mempool]]
size = 32
count = 10000

[[segment.mempool]]
size = 128
count = 10000

[[segment.mempool]]
size = 1024
count = 1000
```

With this configuration, only applications from the `bar` group have write access and can allocate chunks. Applications from the `foo` group have only read access.

This is an example with multiple segments:

```TOML
[general]
version = 1

[[segment]]
reader = "foo"
writer = "bar"

[[segment.mempool]]
size = 32
count = 10000

[[segment]]
reader = "alice"
writer = "eve"

[[segment.mempool]]
size = 1024
count = 100
```

When no config file is specified, a hard-coded version similar to the [default config](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_posh/etc/iceoryx/roudi_config_example.toml) will be used.

### Static configuration

Another way is to have a static config that is compile-time dependent, this means that you have to recompile your RouDi application if you want to change your config (not the iceoryx_posh_roudi lib).
You can have your source file with `main()` method where you can create your custom configuration and pass it to a RouDi instantiation.
In your CMake file for your custom RouDi you need to ensure that it is **not** linking against `iceoryx_posh_config` to have a static config.

A good example of a static config can be found [here](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_examples/iceperf/roudi_main_static_config.cpp).
