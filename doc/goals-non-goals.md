
# Goals and non-goals

## Contents

- [Goals](#goals)
- [Non-Goals](#non-goals)
- [User personas](#user-personas)

## Goals

* High-performance inter-process-communication for various operating systems
* Service discovery functionality with dynamic connections
* Data agnostic, there are no restrictions on payload data
* Compatibility with AUTOSAR Adaptive and ROS 2 communication patterns
* Providing the building blocks for being able to build gateways to network protocols 
* Automotive-grade SW quality
* Modern C++

## Non-Goals

* Providing a data model and things like IDL or code generators
* Shrinking it down for being able to run on µControllers (e.g. with < 1MB of memory)
* Full compliance with the DDS standard

## User personas

**Andrew, the HAD developer**
Andrew is a software developer at a startup working on autonomous cars. Currently their project is using ROS, because
it's easy to get the car driving. After some months, he's realizing that sending gigabytes around, leads to high runtime
demands with ROS. A college mentions iceoryx during lunch, which might be interesting because it has a zero-copy
mechanism and offers a ROS RMW implementation. Soon after giving iceoryx a try, Andrew is thrilled about it. He cannot only feel
the runtime performance boost, but also still keep using his beloved ROS visualization tools!

**Martha, the indie game developer**
Martha always had troubles with those silly game engines. Some are slow but free, others are fast but too expensive.
It's a hard life if you're independent. When a friend who works in the automotive industry mentions he has just started
using iceoryx, which offers fast shared memory communication she listens up. Iceoryx is solely passing pointers around
and does avoid copies to the utmost? "I'll definitely try iceoryx in my new project and see if I can speed up the
performance with my low cost engine" she thinks while wandering home at night after the meetup with her friend.

**Robby, the lonely robot**
Robby is autonomous robot built during a research project at a university. He has a great set of features and can
astonish the crowds by creating a detailed map of the university building in under an hour. However, they made him use
that slow self-made IPC to communicate with his sensors because his parents wanted to get started fast. Though that
makes it hard for him to react in real-time to dangerous incidents like flying coffee cups. When strolling through
the interwebs on a lonely evening, he finds out about iceoryx: Free-to-use, high-performance data transfer with low
runtime overhead, real-time support! Brilliant! Maybe even Robby's biggest wish for a network binding will come true,
so he can stream his favorite videos even faster!
