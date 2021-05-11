# complexdata

## Introduction

To implement zero-copy data transfer we use a shared memory approach. This requires that every data structure needs to be entirely
contained in the shared memory and must not internally use pointers or references. The complete list of restrictions can be found
[here](https://iceoryx.io/latest/getting-started/overview/#restrictions). Therefore, most of the STL types cannot be used, but we
reimplemented some [constructs](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_utils#cxx). This example shows how
to send/receive a iox::cxx::vector and how to send/receive a complex data structure containing some of our STL container surrogates.

## Expected Output

[![asciicast](https://asciinema.org/a/410662.svg)](https://asciinema.org/a/410662)

## Code Walkthrough

The following examples demonstrate how to send/receive the STL containers that were reimplemented in iceoryx so that they meet
our requirements.

### Publisher application sending a `iox::cxx::vector`

In this example we want our publisher to send a vector containing double. Since we cannot use dynamic memory, we use the 
`iox::cxx::vector` with a capacity of 5.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_vector.cpp][create publisher]-->
```cpp
iox::popo::Publisher<iox::cxx::vector<double, 5>> publisher({"Radar", "FrontRight", "VectorData"});
```

We use a while-loop similar to the one described in the 
[icedelivery example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery) to send the
vector to the subscriber. After successfully loaning memory we append elements to the vector until it's full.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_vector.cpp][vector emplace_back]-->
```cpp
for (uint64_t i = 0U; i < sample->capacity(); ++i)
{
    // we can omit the check of the return value since the loop doesn't exceed the capacity of the
    // vector
    sample->emplace_back(static_cast<double>(ct + i));
}
```

The only difference here to the `std::vector` is that `emplace_back` returns a bool - true if the appending was successful,
false otherwise. `emplace_back` fails when the vector is already full. In our case, we can omit the check of the return value
since the for-loop doesn't exceed the capacity of the vector.

### Subscriber application receiving a `iox::cxx::vector`

Our subscriber application iterates over the received vector to print its entries to the console. Note that the `separator` is only
used for a easy to read output.

<!--[geoffrey][iceoryx_examples/complexdata/iox_subscriber_vector.cpp][vector output]-->
```cpp
for (const auto& entry : *sample)
{
    s << separator << entry;
    separator = ", ";
}
```

### Publisher application sending a complex data structure

In this example our publisher will send a more complex data structure. It contains some of the STL containers that are reimplemented
in iceoryx. A list of all reimplemented containers can be found
[here](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_utils#cxx).

<!--[geoffrey][iceoryx_examples/complexdata/topic_data.hpp][complexdata type]-->
```cpp
struct ComplexDataType
{
    forward_list<string<10>, 5> stringForwardList;
    list<uint64_t, 10> integerList;
    list<optional<int32_t>, 15> optionalList;
    stack<float, 5> floatStack;
    string<20> someString;
    vector<double, 5> doubleVector;
    vector<variant<string<10>, double>, 10> variantVector;
};
```

Contrary to the STL containers, the iceoryx containers have a static size, i.e. you have to provide the capacity (= max. size).

We use again a while-loop to loan memory, add data to our containers and send it to the subscriber. Since we must not throw exceptions
all used insertion methods return a bool that indicates whether the insertion was successful. It will fail when a container is already
full. To handle the return value we introduce a helper function.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_complexdata.cpp][handle return val]-->
```cpp
void handleInsertionReturnVal(const bool success)
{
    if (!success)
    {
        std::cerr << "Failed to insert element." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
```

Now let's add some data to our containers. For the lists we use the `push_front` methods which can be used similar to the 
corresponding STL methods.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_complexdata.cpp][fill lists]-->
```cpp
// forward_list<string<10>, 5>
handleInsertionReturnVal(sample->stringForwardList.push_front("world"));
handleInsertionReturnVal(sample->stringForwardList.push_front("hello"));
// list<uint64_t, 10>;
handleInsertionReturnVal(sample->integerList.push_front(ct));
handleInsertionReturnVal(sample->integerList.push_front(ct * 2));
handleInsertionReturnVal(sample->integerList.push_front(ct + 4));
// list<optional<int32_t>, 15>
handleInsertionReturnVal(sample->optionalList.push_front(42));
handleInsertionReturnVal(sample->optionalList.push_front(nullopt));
```

!!! note
    If you're not familiar with `optional`, please have a look at
    [How optional and error values are returned in iceoryx](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/website/advanced/how-optional-and-error-values-are-returned-in-iceoryx.md#optional).

Now we fill the stack

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_complexdata.cpp][fill stack]-->
```cpp
for (uint64_t i = 0U; i < sample->floatStack.capacity(); ++i)
{
    handleInsertionReturnVal(sample->floatStack.push(static_cast<float>(ct * i)));
}
```

and assign a greeting to the string.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_complexdata.cpp][assign string]-->
```cpp
sample->someString = "hello iceoryx";
```

For the vectors we use the `emplace_back` method, which can be used similar to corresponding `std::vector` method.

<!--[geoffrey][iceoryx_examples/complexdata/iox_publisher_complexdata.cpp][fill vectors]-->
```cpp
for (uint64_t i = 0U; i < sample->doubleVector.capacity(); ++i)
{
    handleInsertionReturnVal(sample->doubleVector.emplace_back(static_cast<double>(ct + i)));
}
// vector<variant<string<10>, double>, 10>;
handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<0>(), "seven"));
handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<1>(), 8.0));
handleInsertionReturnVal(sample->variantVector.emplace_back(in_place_index<0>(), "nine"));
```

With `in_place_index` the passed object is constructed in-place at the given index.

### Subscriber application receiving a complex data structure

The subscriber application just prints the received data to the console. For the `optionalList` we have to check whether the
`optional` contains a value. As in the first example, the `separator` is used for a clear output.

<!--[geoffrey][iceoryx_examples/complexdata/iox_subscriber_complexdata.cpp][read optional list]-->
```cpp
for (const auto& entry : sample->optionalList)
{
    (entry.has_value()) ? s << separator << entry.value() : s << separator << "optional is empty";
    separator = ", ";
}
```

To print the elements of the `floatStack`, we pop elements until the stack is empty.

<!--[geoffrey][iceoryx_examples/complexdata/iox_subscriber_complexdata.cpp][read stack]-->
```cpp
auto stackCopy = sample->floatStack;
while (stackCopy.size() > 0U)
{
    auto result = stackCopy.pop();
    s << separator << result.value();
    separator = ", ";
}
```

Please note that `pop` returns a `iox::cxx::optional` which contains the last pushed element or a `nullopt` if the stack is
empty. Here, we don't have to check whether the `optional` contains a value since the loop ensures that we only pop elements
when the stack contains some.

To print the elements of the `variantVector` we iterate over the vector entries and access the alternative that is held by the
variant via its index. We use the not STL compliant `get_at_index` method which returns a pointer to the type stored at the
index. If the variant does not contain any type, `index()` will return an `INVALID_VARIANT_INDEX`.

<!--[geoffrey][iceoryx_examples/complexdata/iox_subscriber_complexdata.cpp][read variant vector]-->
```cpp
for (const auto& i : sample->variantVector)
{
    switch (i.index())
    {
    case 0:
        s << separator << *i.template get_at_index<0>();
        break;
    case 1:
        s << separator << *i.template get_at_index<1>();
        break;
    case INVALID_VARIANT_INDEX:
        s << separator << "variant does not contain a type";
        break;
    default:
        s << separator << "this is a new type";
    }
    separator = ", ";
}
```

<center>
[Check out complexdata on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/complexdata){ .md-button }
</center>
