# icecrystal

## Introduction

This example teaches you how to make use of the introspection for debugging purposes. With the introspection you can
look into the machine room of RouDi. It shows live information about the memory usage and all
registered processes. Additionally, it shows the publisher and subscriber ports that are created inside the shared
memory.

## Expected Output

We re-use the binaries from
[icedelivery](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery).

[![asciicast](https://asciinema.org/a/407442.svg)](https://asciinema.org/a/407442)

## Feature walkthrough

This example does not contain any additional code. The code of the `iceoryx_introspection_client` can be found under
[tools/introspection/](../../tools/introspection/).

The introspection can be started with several command line arguments.

    --mempool         Subscribe to mempool introspection data.

The memory pool view will show all available shared memory segments and their respective owner. Additionally, the
maximum number of available chunks, the number of currently used chunks as well as the minimal value of free chunks
are visible. This can be handy for stress tests to find out if your memory configuration is valid.

    --process         Subscribe to process introspection data.

The process view will show you the processes (incl. PID), which are currently registered with RouDi.

    --port            Subscribe to port introspection data.

The port view shows both publisher and subscriber ports that are created by RouDi in the shared memory. Their respective service
description (service, instance, event) is shown to identify them uniquely. The columns `Process` and `Node` display to which
process and node the ports belong. The service discovery protocol allows you to define the `Propagation scope` of the data. This
can enable data forwarding to other machines e.g. over network or just consume them internally.

    --all             Subscribe to all available introspection data.

`--all` will enable all three views at once.

    -v, --version     Display latest official iceoryx release version and exit.

Make sure that the version number of the introspection exactly matches the version number of RouDi. Currently,
we don't guarantee binary compatibility between different versions. With different version numbers things might break.
