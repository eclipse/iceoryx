# What is Eclipse iceoryx?

The technology behind Eclipse iceoryx originated in the automotive domain. With the introduction of video-based driver
assistance, the amount of data to be exchanged between different threads of execution increased to GBytes/sec. The
resources on these embedded systems were constrained and a solution was needed to use precious runtime for functional
computations, not for shifting around bytes in memory.

The simple answer was to avoid copying of messages inside the middleware that manages the data communication between
the different software nodes. This can be done by using shared memory that can be accessed by the producers and
consumers of messages. On its own, this is not a new innovation as the approach has been used since the 1970s.
However, iceoryx takes the approach further, ending up in an inter-process-communication technology with a
publish/subscribe architecture that is fast, flexible and dependable.


## Fast

With the iceoryx API, a publisher can write the message directly into a chunk of memory that was previously requested
from the middleware. When the message is delivered, subscribers receive reference counted pointers to these memory
chunks, which are stored in queues with configurable capacities. With this iceoryx achieves what we refer to as true
zero-copy — an end-to-end approach from publishers to subscribers without creating a single copy.

Avoiding the copies on API level is crucial when GBytes of sensor data have to be processed per second on robotics and
autonomous driving systems. Therefore the iceoryx team contributed to the standardization of true zero-copy capable
APIs in [ROS 2](https://www.ros.org/) and [AUTOSAR Adaptive](https://www.autosar.org/standards/adaptive-platform/). 
  
On modern processors iceoryx has a latency of less than 1 µs for transferring a message. And the best message is that
this latency is constant as size doesn't matter. Want to give it a try? Then have a look at our
[iceperf example](../examples/iceperf) after having made the first steps. 

## Flexible

iceoryx already supports Linux, QNX and MacOS as operating systems as well as C and C++ as user APIs. Windows and Rust
are the next ones on the list. The typed C++ API is the most comfortable when you want to directly use the iceoryx API
on the user side. The untyped C++ API and the C API provide a data agnostic interface that is often preferred when
integrating iceoryx as shared memory backbone into a bigger framework.

The APIs support polling access and event-driven interactions with the [Waitset](../overview/#waitset) and
[Listener](../overview/#listener). Applications can be started and stopped flexibly as there is a service discovery
behind the scenes that dynamically connects matching communication entities. 

That iceoryx has the right set of features can be seen from the already existing integrations in middleware and
frameworks such as [Eclipse Cyclone DDS](https://github.com/eclipse-cyclonedds/cyclonedds),
 [eCAL from Continental](https://continental.github.io/ecal/),
 [RTA-VRTE from ETAS](https://www.etas.com/en/products/rta-vrte.php) and
 [Apex.OS from Apex.AI](https://www.apex.ai/apex-os).  

## Dependable

The predecessor of iceoryx is running in millions of vehicles worldwide. All iceoryx maintainers hail from the
safety critical automotive domain. Hence, they know the necessary requirements and have these in mind for the
design and implementation of features. The usage of heap, exceptions and any undefined behavior are to be avoided
to increase the predictability. Instead a custom memory allocation is being used, based on static memory pools.
Additionally, the handling of return values and error cases was inspired by upcoming C++ features and other
languages like Rust (details can be found
[here](../../advanced/how-optional-and-error-values-are-returned-in-iceoryx/)).

As different processes are operating on shared data structures, avoiding deadlocks is becoming all the more important.
iceoryx uses lock-free data structures like the multi-producer multi-consumer (MPMC) queue that was written portably
thanks to modern C++.  

The tools available for automotive-compliant software development are always one or two releases behind the latest C++
standard. This fact, combined with our already mentioned constraints, led to a bunch of STL like C++ classes that have
the goal to combine modern C++ with the reliability needed for the domains iceoryx is used in. They can be found in
the iceoryx utils which are introduced [here](../../advanced/iceoryx_utils/)
