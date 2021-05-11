
# Eclipse iceoryx utils overview

The iceoryx utils are our basic building blocks - the foundation of
iceoryx. There are a wide variety of building blocks grouped together in categories
or namespace, depending on where or how they are used.

## Categories

| Namespace       | Short Description |
|:---------------:|:------------------|
| [cxx](#cxx) | Since we are not allowed to use C++17 as well as the heap or exceptions we implemented constructs like `optional`, `expected` or `variant` so that we can be as modern as possible. Furthermore, you can find here constructs which are mentioned in the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines) as well as constructs like `smart_c` which enables us to work with good old C function more easily since it does all the error handling for us like taking care of errno or catching EINTR. |
| [concurrent](#concurrent) | You should never use concurrent constructs like `mutex`, `semaphores`, `atomic` etc directly in our codebase. At the moment we still have some exceptions to this guideline but the idea is that all classes which are using them are stored under concurrent and have to undergo more tests then the usual non concurrent class. For instance we try to provide stress tests for them. In here you can find `fifo`, `smart_lock`, `sofi`, `trigger_queue` and much more. |
| [design_pattern](#design-pattern) | Certain code patterns which are repeating themselves all over the code are abstracted and stored in here. At the moment we only have the creation pattern but maybe there will be more. |
| [error-handling](#error-handling) | The Error-Handler to in iceoryx for handling fault cases. |
| [log](#log) | The logger used by iceoryx. |
| [posix_wrapper](#posix-wrapper) | Posix constructs like shared memory, threads or semaphores are not used directly in our code base. We abstracted them so that they are following the [RAII (Resource Acquisition Is Initialization)](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) idiom and other good practices from the C++ community. |
| [units](#units) | |
| other | There are even more namespaces inside the iceoryx utils but they will either become obsolete in the future, will be integrated into existing names or are established as another namespace and documented here. We are unsure where they will end up in the future. |

## Structure

 The following sections have a column called `internal` to indicate that the API
 is not stable and can change anytime. You should never rely on it and you get no
 support if you do and your code does not compile after a pull request.

 The column `maybe obsolete` marks classes which can be removed anytime soon.

### CXX

STL constructs which are not in the C++11 standard included are contained here as
well as constructs which are helping us in our daily life. We differ in
here from our coding guidelines and follow the C++ STL coding guidelines
to help the user to have a painless transition from the official STL
types to ours. The API should also be identical to the corresponding
STL types but we have to make exceptions here. For instance, we do not
throw exceptions, try to avoid undefined behavior and we do not use
dynamic memory. In these cases we adjusted the API to our use case.

Most of the headers are providing some example code on how the
class should be used.

| class/file                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`algorithm`          |   |   | Implements `min` and `max` for an arbitrary number of values of the same type. For instance `min(1,2,3,4,5);` |
|`attributes`          |   |   | C++17 and C++20 attributes are sometimes available through compiler extensions. The attribute macros defined in here (like `IOX_FALLTHROUGH`, `IOX_MAYBE_UNUSED` ... ) make sure that we are able to use them if the compiler supports it. |
|`convert`            |   |   | Converting a number into a string is easy, converting it back can be hard. You can use functions like `strtoll` but you still have to handle errors like under- and overflow, or converting invalid strings into number. Here we abstract all the error handling so that you can convert strings into numbers safely. |
|`expected`           |   |   | Our base class used in error handling. Every function which can fail should return an expected. With this the user knows that this function can fail and that they have to do some kind of error handling. We got inspired by the [C++ expected proposal]( http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0323r7.html) and by the [rust error handling concept](https://doc.rust-lang.org/std/result/enum.Result.html). |
|`forward_list`       |   |   | Heap and exception free, relocatable implementation of `std::forward_list` |
|`function_ref`       |   |   | C++11 implementation of the next-gen C++ feature `std::function_ref` see [function_ref proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0792r2.html). It behaves like `std::function` but does not own the callable. |
|`GenericRAII`        |   |   | This is an abstraction of the C++ RAII idiom. Sometimes you have constructs where you would like to perform a certain task on creation and then again when they are getting out of scope, this is where `GenericRAII` comes in. It is like a `std::lock_guard` or a `std::shared_ptr` but more generic. |
|`helplets`           |   |   | Implementations of [C++ Core Guideline](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) concepts like `not_null` `Ensures` `Expects` are contained here. Additionally, we are providing some types to verify preconditions at compile time. Think of an int which has to be always greater 5, here we provide types like `greater_or_equal<int, 6>`.|
|`list`               |   |   | Heap and exception free, relocatable implementation of `std::list` |
|`MethodCallback  `  |   |   | Constructs a MethodCallback from a pointer to a specific object and a pointer to a method of that object, also as `ConstMethodCallback` available |
|`NewType<T, Policies>`|   |   | C++11 implementation of [Haskells NewType-pattern](https://wiki.haskell.org/Newtype). |
|`optional`           |   |   | C++11 implementation of the C++17 feature `std::optional` |
|`pair`               | i | X | Simplistic re-implementation of an `std::pair` It maybe becomes obsolete. |
|`poor_mans_heap`     |   |   | Acquires memory on the stack for placement new instantiations. All classes must inherit from a base class which has to be known at compile time but the class itself does not have to be known - only the size. |
|`ReferenceCounter`   | i |   | Basic building block for classes which are needing some kind of reference counting like a `std::shared_ptr` |
|`serialization`      |   | X | Implements a simple serialization concept for classes based on the idea presented here [ISOCPP serialization](https://isocpp.org/wiki/faq/serialization#serialize-text-format). |
|`set`                | i | X | Templated helper functions to create a fake `std::set` from a vector. |
|`smart_c`            |   |   | Wrapper around C and POSIX function calls which performs a full error handling. Additionally, this wrapper makes sure that `EINTR` handling is performed correctly by repeating the system call. |
|`string`             |   |   | Heap and exception free implementation of `std::string`. Attention, since the string is stack based, std::string or char array which are assigned to this string will be truncated and zero-terminated if they exceed the string capacity. |
|`type_traits`        |   |   | Extended support for evaluating types on compile-time. |
|`types`              |   |   | Declares essential building block types like `byte_t`. |
|`unique_ptr`         |   |   | Provides a heap-less unique ptr implementation, unlike the STL |
|`variant`            |   |   | C++11 implementation of the C++17 feature `std::variant` |
|`variant_queue`      |   |   | A queue which wraps multiple variants of Queues (FiFo, SoFi, ResizeableLockFreeQueue) |
|`vector`             |   |   | Heap and exception free implementation of `std::vector` |

### Concurrent

If you have to write concurrent code, never use concurrency constructs like `mutex`, `atomic`, `thread`, `semaphore` etc. directly. Most of the use cases can be solved by using an `ActiveObject` which uses as building block our `FiFo` or a
queue which is thread-safe when combined with `smart_lock`. To learn more about active objects see [Prefer Using Active Objects Instead Of Naked Threads](https://www.drdobbs.com/parallel/prefer-using-active-objects-instead-of-n/225700095).

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`ActiveObject`       | i | X | Active object base skeleton implementation inspired by [Prefer Using Active Objects Instead Of Naked Threads](https://www.drdobbs.com/parallel/prefer-using-active-objects-instead-of-n/225700095)  |
|`FiFo`               | i |   | Single Producer, Single Consumer Lock Free FiFo |
|`LoFFLi`             | i |   | Lock-free LIFO based index manager (Lock Free Free List). One building block of our memory manager. After construction it contains the indices {0 ... n} which you can acquire and release. |
|`PeriodicTask`       | i |   | Periodically executes a callable specified by the template parameter in a configurable time interval. |
|`smart_lock`         | i |   | Creates arbitrary thread safe constructs which then can be used like smart pointers. If some STL type should be thread safe use the smart_lock to create the thread safe version in one line. Based on some ideas presented in [Wrapping C++ Member Function Calls](https://stroustrup.com/wrapper.pdf) |
|`SoFi`               | i |   | Single Producer, Single Consumer Lock Free Safely overflowing FiFo (SoFi). |
|`TACO`               | i |   | Thread Aware exChange Ownership (TACO). Solution if you would like to use `std::atomic` with data types larger than 64 bit. Wait free data synchronization mechanism between threads.|
|`TriggerQueue`       | i | X | Queue with a `push` - `pop` interface where `pop` is blocking as long as the queue is empty. Can be used as a building block for active objects. |

attribute overview of the available Queues:

| Data Structure | Shared Memory usable  | Thread-Safe | Lock-Free | Concurrent Producers : Consumers | Bounded Capacity | Data Type Restriction |Use Case |
|----------------|-----------------------|-------------|-----------|----------------------------------|------------------|---|-----------------------|
|`FiFo`          | Yes | Yes | Yes | 1:1 | Yes | Copyable            | FIFO Data transfer |
|`LockfreeQueue` | Yes | Yes | Yes | n:m | Yes | Copyable or Movable | lock-free transfer of arbitrary data between multiple contexts in FIFO order with overflow handling (ringbuffer) |
|`LoFFLi`        | Yes | Yes | Yes | n:m | Yes | int32               | manage memory access, LIFO order |
|`smart_lock`    | Yes | Yes | No  | n/a | n/a | None                | Wrapper to make classes thread-safe (by using a lock)|
|`SoFi`          | Yes | Yes | Yes | 1:1 | Yes | Trivially Copyable  | lock-free transfer of small data (e.g. pointers) between two contexts in FIFO order with overflow handling (ringbuffer) |
|`TACO`          | Yes | Yes | Yes | n:m | Yes | Copyable or Movable | fast lock-free exchange data between threads|
|`TriggerQueue`  | No  | Yes | No  | n:m | Yes | Copyable            | Process events in a blocking way|

### Design pattern

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`Creation`           |  | | When implementing resource handling classes which follow the RAII idiom we may have to throw exceptions inside the constructor. As an alternative to exceptions we have the creation pattern, a specialized factory which returns the object inside of an `expected`. |
|`Command`           | i | | Base class for void(Arg) calls - known as commands. |

### Error handling

The Error-Handler is a central instance for collecting al errors and react to them. In the file `error-handling.hpp` are all error enums collected. The Error-Handler has different error-levels, for more information see [error-handling.md](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/design/error-handling.md)

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`errorHandler`            |   | | Free function to call the Error-Handler with a defined error and an error-level, see header file for practical example.|
|`ErrorHandler`            | i | | ErrorHandler class only for testing purposes, should not be used directly |

### Log

For information about how to use the logger API see [error-handling.md](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/design/error-handling.md)

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`logger`             |   | | |

### POSIX wrapper

We abstract POSIX resources following the RAII idiom and by using our [Creation](#design-pattern) pattern. Try to exclusively use these
abstractions or add a new one when using POSIX resources like semaphores, shared memory, etc.

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`AccessController`   | i | | Interface for Access Control Lists (ACL). |
|`IpcChannel`         | i | | Helper types used by the `MessageQueue`and the `UnixDomainSocket`. |
|`MessageQueue`       | i | | Interface for Message Queues, see [ManPage mq_overview](https://www.man7.org/linux/man-pages/man7/mq_overview.7.html). |
|`mutex`              | i | | Mutex interface, see [ManPage pthread_mutex_lock](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html). |
|`posix_access_rights`   | | | Rights and user management interface. |
|`Semaphore`          |   | | Semaphore interface, see [ManPage sem_overview](https://man7.org/linux/man-pages/man7/sem_overview.7.html) |
|`shared_memory_object/Allocator` | i | | Helper class for the `SharedMemoryObject`. |
|`shared_memory_object/MemoryMap` | i | | Abstraction of `mmap`, `munmap` and helper class for the `SharedMemoryObject`.|
|`shared_memory_object/SharedMemory` | i | | Abstraction of shared memory, see [ManPage shm_overview](https://www.man7.org/linux/man-pages/man7/shm_overview.7.html) and helper class for the `SharedMemoryObject`. |
|`SharedMemoryObject` | i | | Creates and maps existing shared memory into the application. |
|`system_configuration` | i | | Collection of free functions which acquire system information like the page-size. |
|`thread`              |   | | Wrapper for pthread functions like `pthread_setname_np`. |
|`Timer`              |   | | Interface for the posix timer, see [ManPage timer_create](https://www.man7.org/linux/man-pages/man2/timer_create.2.html). |
|`timespec`           | i | X | Free functions which modify `timespec`. |
|`UnixDomainSocket`   | i | | Interface for unix domain sockets. |

### Units

Never use physical properties like speed or time directly as integer or float in your code.
Otherwise you encounter problems like this function `void setTimeout(int timeout)`. What is the unit of the argument, seconds? minutes? If you use our `Duration` you see it directly in the code.

```
void setTimeout(const Duration & timeout);

setTimeout(11_s); // 11 seconds
setTimeout(5_ms); // 5 milliseconds
```

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`Duration`           | i | | Represents the unit time, is convertable to `timespec`, `timeval` and `std::chrono`. Uses user defined literals for readability.  |

### objectpool

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`ObjectPool`           | i | | Container which stores raw objects without calling the ctor of the objects.  |

### graphs

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`DirectedGraph`          | i | | Creates and manages a [directed graph](https://en.wikipedia.org/wiki/Directed_graph).  |
|`DirectedAcyclicGraph`   | i | | Like the DirectedGraph but additional checks prohibit to create edges which have a cyclic dependency.  |

### file-reader

| class                   | internal | maybe obsolete | description |
|:-----------------------:|:--------:|:--------------:|:------------|
|`FileReader`          | i | x | Wrapper for opening files and reading them. Maybe not needed. |

<center>
[Check out iceoryx_utils on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_utils/){ .md-button }
</center>
