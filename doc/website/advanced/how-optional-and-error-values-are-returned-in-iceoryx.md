# How optional and error values are returned in iceoryx

Many parts of the iceoryx C++ API follow a functional programming approach and allow the user to specify functions
which handle the possible cases, e.g. what should happen when data is received.

This is very flexible but requires using the monadic types `cxx::expected` and `cxx::optional`, which we
introduce in the following sections.

## Optional

The type `iox::cxx::optional<T>` is used to indicate that there may or may not be a value of a specific type `T`
available. This is essentially the 'maybe [monad](https://en.wikipedia.org/wiki/Monad_(functional_programming))' in
functional programming. Assuming we have some optional (usually the result of some computation)

```cpp
optional<int> result = someComputation();
```

we can check for its value using

```cpp
if(result.has_value())
{
    auto value = result.value();
    // do something with the value
}
else
{
    // handle the case that there is no value
}
```

A shorthand to get the value is

```cpp
auto value = *result;
```

!!! attention
    Accessing the value if there is no value terminates the application, so it must be checked beforehand.

We can achieve the same with the functional approach by providing a function for both cases.

```cpp
result.and_then([](int& value) { /*do something with the value*/ })
    .or_else([]() { /*handle the case that there is no value*/ });
```

Notice that we get the value by reference, so if a copy is desired it has to be created explicitly in the
[lambda](https://en.wikipedia.org/wiki/Anonymous_function#C++_(since_C++11)) or function we pass.

The optional can be initialized from a value directly

```cpp
optional<int> result = 73;
result = 37;
```

If the optional is default initialized, it is automatically set to its null value of type `iox::cxx::nullopt_t`.
This can be also done directly by using the constant `iox::cxx::nullopt`

```cpp
result = iox::cxx::nullopt;
```

For a complete list of available functions see
[`optional.hpp`](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_utils/include/iceoryx_utils/cxx/optional.hpp).
The `iox::cxx::optional` behaves like the [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional)
except that it does not throw exceptions and has no undefined behavior.

## Expected

`iox::cxx::expected<T, E>` generalizes `iox::cxx::optional` by admitting a value of another type `E` instead of
no value at all, i.e. it contains either a value of type `T` or `E`. In this way, `expected` is a special case of
the 'either monad'. It is usually used to pass a value of type `T` or an error that may have occurred, i.e. `E` is the
error type. `E` must contain a static member or an enum value called `INVALID_STATE`. Alternatively an
[`ErrorTypeAdapter`](https://github.com/eclipse-iceoryx/iceoryx/blob/5b1a0514e72514c2eae8a9d071d82a3905fedf8b/iceoryx_utils/include/iceoryx_utils/cxx/expected.hpp#L46)
can be implemented.

For more information on how it is used for error handling see
[error-handling.md](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/design/error-handling.md).

Assume we have `E` as an error type, then we can create a value

```cpp
iox::cxx::expected<int, E> result(iox::cxx::success<int>(73));
```

and use the value or handle a potential error

```cpp
if (!result.has_error())
{
    auto value = result.value();
    // do something with the value
}
else
{
    auto error = result.get_error();
    // handle the error
}
```

If we need an error value, we set

```cpp
result = iox::cxx::error<E>(errorCode);
```

which assumes that `E` can be constructed from an `errorCode`.

We again can employ a functional approach like this:

```cpp
auto handleValue = [](int& value) { /*do something with the value*/ };
auto handleError = [](E& value) { /*handle the error*/ };
result.and_then(handleValue).or_else(handleError);
```

There are more convenience functions such as `value_or` which provides the value or an alternative specified by the
user. These can be found in
[`expected.hpp`](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_utils/include/iceoryx_utils/cxx/expected.hpp).

Note that when we move an `expected`, the origin is set to the error value `E::INVALID_STATE` and `has_error()` will
always return true:

```cpp
cxx::expected<int, E> result(iox::cxx::success<int>(1421));
cxx::expected<int, E> anotherResult = std::move(result);

if (result.has_error()) // is now true since it was moved
{
   result.get_error(); // returns E::INVALID_STATE
}
```
