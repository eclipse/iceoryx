// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_UTILS_CXX_EXPECTED_HPP
#define IOX_UTILS_CXX_EXPECTED_HPP

#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/variant.hpp"

#include <utility>

namespace iox
{
namespace cxx
{
namespace internal
{
/// @brief Type trait which verifies whether the passed type T has INVALID_STATE
///        std::true_type overload chosen when INVALID_STATE is present, otherwise std::false_type
template <typename, typename = void>
struct HasInvalidStateMember;

/// @brief Type trait which verifies whether the passed type T is of type cxx::optional
template <typename...>
struct IsOptional;
} // namespace internal

/// @brief Generic adapter to access INVALID_STATE member or value
/// @note Works for enum classes and classes having a member INVALID_STATE
template <typename T>
struct ErrorTypeAdapter
{
    static_assert(internal::HasInvalidStateMember<T>::value,
                  "T must have an INVALID_STATE value/member. Alternatively write an ErrorTypeAdapter specialisation "
                  "for your type");

    static T getInvalidState() noexcept;
};

/// @brief helper struct to create an expected which is signalling success more easily
/// @param T type which the success helper class should contain
/// @code
///     cxx::expected<int, float> callMe() {
///         //...
///         return cxx::success<int>(55);
///     }
/// @endcode
template <typename T = void>
struct success
{
    /// @brief constructor which creates a success helper class by copying
    ///         the value of t
    /// @param[in] t value which should be later stored in an expected
    success(const T& t) noexcept;

    /// @brief constructor which creates a success helper class by moving
    ///         the value of t
    /// @param[in] t value which should be later moved into an expected
    success(T&& t) noexcept;
    template <typename... Targs>

    /// @brief constructor which creates a success helper class by forwarding
    ///         arguments to the constructor of T
    /// @param[in] args... arguments which will be perfectly forwarded to the
    ///                     constructor
    success(Targs&&... args) noexcept;

    T value;
};

/// @brief helper struct to create an error only expected which is signalling success more easily
/// @code
///     cxx::expected<float> callMe() {
///         //...
///         return cxx::success<>();
///     }
/// @endcode
template <>
struct success<void>
{
};

/// @brief helper struct to create an expected which is signalling an error more easily
/// @param T type which the success helper class should contain
/// @code
///     cxx::expected<float> callMe() {
///         //...
///         return cxx::error<float>(12.34f);
///     }
/// @endcode
template <typename T>
struct error
{
    /// @brief constructor which creates a error helper class by copying
    ///         the value of t
    /// @param[in] t value which should be later stored in an expected
    error(const T& t) noexcept;

    /// @brief constructor which creates a error helper class by moving
    ///         the value of t
    /// @param[in] t value which should be later moved into an expected
    error(T&& t) noexcept;

    /// @brief constructor which creates a error helper class by forwarding
    ///         arguments to the constructor of T
    /// @param[in] args... arguments which will be perfectly forwarded to the
    ///                     constructor
    template <typename... Targs>
    error(Targs&&... args) noexcept;

    T value;
};

template <typename... T>
class IOX_NO_DISCARD expected;

/// @brief expected implementation from the C++20 proposal with C++11. The interface
///         is inspired by the proposal but it has changes since we are not allowed to
///         throw an exception.
/// @param ErrorType type of the error which can be stored in the expected
///
/// @code
///     cxx::expected<int, float> callMe() {
///         bool l_errorOccured;
///         // ... do stuff
///         if ( l_errorOccured ) {
///             return cxx::error<float>(55.1f);
///         } else if ( !l_errorOccured ) {
///             return cxx::success<int>(123);
///         }
///     }
///
///     cxx::expected<float> errorOnlyMethod() {
///         return callMe().or_else([]{
///             std::cerr << "Error Occured\n";
///             /// perform some action
///         }).and_then([](cxx::expected<int, float> & result){
///             std::cout << "Success, got " << result.value() << std::endl;
///             /// perform some action
///         });
///     }
///
///     cxx::expected<std::vector<int>, int> allHailHypnotoad(success<std::vector<int>>({6,6,6}));
///     allHailHypnotoad->push_back(7);
/// @endcode
template <typename ErrorType>
class IOX_NO_DISCARD expected<ErrorType>
{
  public:
    /// @brief default ctor is deleted since you have to clearly state if the
    ///         expected contains a success value or an error value
    expected() = delete;

    /// @brief the copy constructor calls the copy constructor of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected(const expected&) noexcept = default;

    /// @brief the move constructor calls the move constructor of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected(expected&& rhs) noexcept;

#if defined(_WIN32)
    /// @brief copy conversion constructor to convert an expected which contains value and
    ///        error type to an expected which contains only an error
    template <typename ValueType>
    expected(const expected<ValueType, ErrorType>& rhs) noexcept;

    /// @brief move conversion constructor to convert an expected which contains value and
    ///        error type to an expected which contains only an error
    template <typename ValueType>
    expected(expected<ValueType, ErrorType>&& rhs) noexcept;
#endif
    /// @brief calls the destructor of the success value or error value - depending on what
    ///         is stored in the expected
    ~expected() = default;

    /// @brief  calls the copy assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected& operator=(const expected&);

    /// @brief  calls the move assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected& operator=(expected&& rhs) noexcept;

#if defined(_WIN32)
    /// @brief  calls the copy assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    template <typename ValueType>
    expected& operator=(const expected<ValueType, ErrorType>& rhs) noexcept;

    /// @brief  calls the move assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    template <typename ValueType>
    expected& operator=(expected<ValueType, ErrorType>&& rhs) noexcept;
#endif

    /// @brief  constructs an expected which is signaling success
    /// @param[in] successValue value which will be stored in the expected
    expected(const success<void>& successValue) noexcept;

    /// @brief  constructs an expected which is signaling an error and stores the
    ///         error value provided by errorValue
    /// @param[in] errorValue error value which will be stored in the expected
    expected(const error<ErrorType>& errorValue) noexcept;

    /// @brief  constructs an expected which is signaling an error and stores the
    ///         error value provided by value
    /// @param[in] errorValue error value which will be moved into the expected
    expected(error<ErrorType>&& errorValue) noexcept;

    /// @brief  creates an expected which is signaling success
    /// @return expected signalling success
    static expected create_value() noexcept;

    /// @brief  creates an expected which is signaling an error and perfectly forwards
    ///         the args to the constructor of lErrorType
    /// @param[in] args... arguments which will be forwarded to the ErrorType constructor
    /// @return expected signalling error
    template <typename... Targs>
    static expected create_error(Targs&&... args) noexcept;

    /// @brief  returns true if the expected contains an error otherwise false
    /// @return bool which contains true if the expected contains an error
    explicit operator bool() const noexcept;

    /// @brief  returns true if the expected contains an error otherwise false
    /// @return bool which contains true if the expected contains an error
    bool has_error() const noexcept;

    /// @brief  returns a reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return reference to the internally contained error
    ErrorType& get_error() & noexcept;

    /// @brief  returns a const reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return const reference to the internally contained error
    const ErrorType& get_error() const& noexcept;

    /// @brief  returns a rvalue reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return rvalue reference to the internally contained error
    ErrorType&& get_error() && noexcept;

    /// @brief  if the expected does contain an error the given callable is called and
    ///         a reference to the ErrorType is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains an error
    /// @return const reference to the expected itself
    /// @code
    ///     someExpected.or_else([](float& error){
    ///         std::cout << "error occured : " << error << std::endl;
    ///     })
    /// @endcode
    const expected& or_else(const cxx::function_ref<void(ErrorType&)>& callable) const noexcept;

    /// @brief  if the expected does contain an error the given callable is called and
    ///         a reference to the ErrorType is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains an error
    /// @return const reference to the expected itself
    /// @code
    ///     someExpected.or_else([](float& error){
    ///         std::cout << "error occured : " << error << std::endl;
    ///     })
    /// @endcode
    expected& or_else(const cxx::function_ref<void(ErrorType&)>& callable) noexcept;

    /// @brief  if the expected does contain a success value the given callable is called and
    ///         a reference to the expected is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains a success value
    /// @return const reference to the expected itself
    /// @code
    ///     someExpected.and_then([]{
    ///         std::cout << "we are successful!" << std::endl;
    ///     })
    /// @endcode
    const expected& and_then(const cxx::function_ref<void()>& callable) const noexcept;

    /// @brief  if the expected does contain a success value the given callable is called and
    ///         a reference to the expected is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains a success value
    /// @return const reference to the expected itself
    /// @code
    ///     someExpected.and_then([]{
    ///         std::cout << "we are successful!" << std::endl;
    ///     })
    /// @endcode
    expected& and_then(const cxx::function_ref<void()>& callable) noexcept;

  private:
    expected(variant<ErrorType>&& store, const bool hasError) noexcept;
    variant<ErrorType> m_store;
    bool m_hasError;
    static constexpr uint64_t ERROR_INDEX = 0U;
};

/// @brief specialization of the expected class which can contain an error as well as a success value
/// @param ValueType type of the value which can be stored in the expected
/// @param ErrorType type of the error which can be stored in the expected
template <typename ValueType, typename ErrorType>
class IOX_NO_DISCARD expected<ValueType, ErrorType>
{
  public:
    /// @brief default ctor is deleted since you have to clearly state if the
    ///         expected contains a success value or an error value
    expected() = delete;

    /// @brief the copy constructor calls the copy constructor of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected(const expected&) = default;

    /// @brief the move constructor calls the move constructor of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected(expected&& rhs) noexcept;

    /// @brief calls the destructor of the success value or error value - depending on what
    ///         is stored in the expected
    ~expected() noexcept = default;

    /// @brief  calls the copy assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected& operator=(const expected&) noexcept;

    /// @brief  calls the move assignment operator of the contained success value
    ///         or the error value - depending on what is stored in the expected
    expected& operator=(expected&& rhs) noexcept;

    /// @brief  constructs an expected which is signaling success and uses the value
    ///         provided by successValue to copy construct its success value
    /// @param[in] successValue value which will be stored in the expected
    expected(const success<ValueType>& successValue) noexcept;

    /// @brief  constructs an expected which is signaling success and uses the value
    ///         provided by successValue to move construct its success value
    /// @param[in] successValue value which will be moved into the expected
    expected(success<ValueType>&& successValue) noexcept;

    /// @brief  constructs an expected which is signaling an error and stores the
    ///         error value provided by errorValue
    /// @param[in] errorValue error value which will be stored in the expected
    expected(const error<ErrorType>& errorValue) noexcept;

    /// @brief  constructs an expected which is signaling an error and stores the
    ///         error value provided by errorValue
    /// @param[in] errorValue error value which will be moved into the expected
    expected(error<ErrorType>&& errorValue) noexcept;

    /// @brief  creates an expected which is signaling success and perfectly forwards
    ///         the args to the constructor of ValueType
    /// @param[in] args... arguments which will be forwarded to the ValueType constructor
    /// @return expected signalling success
    template <typename... Targs>
    static expected create_value(Targs&&... args) noexcept;

    /// @brief  creates an expected which is signaling an error and perfectly forwards
    ///         the args to the constructor of ErrorType
    /// @param[in] args... arguments which will be forwarded to the ErrorType constructor
    /// @return expected signalling error
    template <typename... Targs>
    static expected create_error(Targs&&... args) noexcept;

    /// @brief  returns true if the expected contains an error otherwise false
    /// @return bool which contains true if the expected contains an error
    explicit operator bool() const noexcept;

    /// @brief  returns true if the expected contains an error otherwise false
    /// @return bool which contains true if the expected contains an error
    bool has_error() const noexcept;

    /// @brief  returns a reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return reference to the internally contained error
    ErrorType& get_error() & noexcept;

    /// @brief  returns a const reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return const reference to the internally contained error
    const ErrorType& get_error() const& noexcept;

    /// @brief  returns a rvalue reference to the contained error value, if the expected
    ///         does not contain an error this is undefined behavior
    /// @return rvalue reference to the internally contained error
    ErrorType&& get_error() && noexcept;

    /// @brief  returns a reference to the contained success value, if the expected
    ///         does not contain a success value this is undefined behavior
    /// @return reference to the internally contained value
    ValueType& value() & noexcept;

    /// @brief  returns a const reference to the contained success value, if the expected
    ///         does not contain a success value this is undefined behavior
    /// @return const reference to the internally contained value
    const ValueType& value() const& noexcept;

    /// @brief  returns a reference to the contained success value, if the expected
    ///         does not contain a success value this is undefined behavior
    /// @return rvalue reference to the internally contained value
    ValueType&& value() && noexcept;

    /// @brief  returns a copy of the contained success value if the expected does
    ///         contain a success value, otherwise it returns a copy of value
    /// @return copy of the internally contained value or copy of value
    ValueType value_or(const ValueType& value) const noexcept;

    /// @brief  returns a copy of the contained success value if the expected does
    ///         contain a success value, otherwise it returns a copy of value
    /// @return copy of the internally contained value or copy of value
    ValueType value_or(const ValueType& value) noexcept;


    /// @brief dereferencing operator which returns a reference to the contained
    ///         success value. if the expected contains an error the behavior is
    ///         undefined.
    /// @return reference to the contained value
    /// @code
    ///     cxx::expected<int, float> frodo(success<int>(45));
    ///     *frodo += 12;
    ///     std::cout << *frodo << std::endl; // prints 57
    /// @endcode
    ValueType& operator*() noexcept;

    /// @brief dereferencing operator which returns a reference to the contained
    ///         success value. if the expected contains an error the behavior is
    ///         undefined.
    /// @return const reference to the contained value
    /// @code
    ///     cxx::expected<int, float> frodo(success<int>(45));
    ///     *frodo += 12;
    ///     std::cout << *frodo << std::endl; // prints 57
    /// @endcode
    const ValueType& operator*() const noexcept;

    /// @brief arrow operator which returns the pointer to the contained success value.
    ///         if the expected contains an error the behavior is undefined.
    /// @return pointer of type ValueType to the contained value
    /// @code
    ///     cxx::expected<std::vector<int>, int> holyPiotr(success<std::vector<int>>({1,2,3}));
    ///     holyPiotr->push_back(4);
    /// @endcode
    ValueType* operator->() noexcept;

    /// @brief arrow operator which returns the pointer to the contained success value.
    ///         if the expected contains an error the behavior is undefined.
    /// @return pointer of type const ValueType to the contained value
    /// @code
    ///     cxx::expected<std::vector<int>, int> holyPiotr(success<std::vector<int>>({1,2,3}));
    ///     holyPiotr->push_back(4);
    /// @endcode
    const ValueType* operator->() const noexcept;

    /// @brief conversion operator to an error only expected which can be useful
    ///         if you would like to return only the success of a function
    /// @return converts an expected which can contain a value and an error to an
    ///         expected which contains only an error
    /// @code
    ///     cxx::expected<int, int> someErrorProneFunction(){}
    ///
    ///     cxx::expected<int> isItSuccessful() {
    ///         return someErrorProneFunction();
    ///     }
    /// @endcode
    template <typename T>
    operator expected<T>() noexcept;

    /// @brief conversion operator to an error only expected which can be useful
    ///         if you would like to return only the success of a function
    /// @return converts an expected which can contain a value and an error to an
    ///         expected which contains only an error
    /// @code
    ///     cxx::expected<int, int> someErrorProneFunction(){}
    ///
    ///     cxx::expected<int> isItSuccessful() {
    ///         return someErrorProneFunction();
    ///     }
    /// @endcode
    template <typename T>
    operator expected<T>() const noexcept;

    /// @brief  if the expected does contain an error the given callable is called and
    ///         a reference to the ErrorType is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains an error
    /// @return const reference to the expected itself
    /// @code
    ///     someExpected.or_else([](float& result){
    ///         std::cout << "error occured : " << error << std::endl;
    ///     })
    /// @endcode
    const expected& or_else(const cxx::function_ref<void(ErrorType&)>& callable) const noexcept;

    /// @brief  if the expected does contain an error the given callable is called and
    ///         a reference to the ErrorType is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains an error
    /// @return reference to the expected itself
    /// @code
    ///     someExpected.or_else([](float& error){
    ///         std::cout << "error occured : " << error << std::endl;
    ///     })
    /// @endcode
    expected& or_else(const cxx::function_ref<void(ErrorType&)>& callable) noexcept;

    /// @brief  if the expected does contain a success value the given callable is called and
    ///         a reference to the result is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains a success value
    /// @return const reference to the expected
    /// @code
    ///     someExpected.and_then([](int& result){
    ///         std::cout << "we have a result : " << result << std::endl;
    ///     })
    /// @endcode
    const expected& and_then(const cxx::function_ref<void(ValueType&)>& callable) const noexcept;

    /// @brief  if the expected does contain a success value the given callable is called and
    ///         a reference to the result is given as an argument to the callable
    /// @param[in] callable callable which will be called if the expected contains a success value
    /// @return reference to the expected
    /// @code
    ///     someExpected.and_then([](int& result){
    ///         std::cout << "we have a result : " << result << std::endl;
    ///     })
    /// @endcode
    expected& and_then(const cxx::function_ref<void(ValueType&)>& callable) noexcept;

    ///
    /// @brief if the expected contains a success value and its type is a non-empty optional, retrieve the value from
    ///         the optional and provide it as the argument to the provided callable
    /// @param[in] callable the callable to be called with the contents of the optional
    /// @return reference to the expected
    /// @code
    ///     anExpectedOptional.and_then([](int& value){
    ///         std::cout << "the optional contains the value: " << result << std::endl;
    ///     })
    /// @endcode
    ///
    template <typename Optional = ValueType,
              typename std::enable_if<internal::IsOptional<Optional>::value, int>::type = 0>
    const expected& and_then(const cxx::function_ref<void(typename Optional::type&)>& callable) const noexcept;

    ///
    /// @brief if the expected contains a success value and its type is a non-empty optional, retrieve the value from
    ///         the optional and provide it as the argument to the provided callable
    /// @param[in] callable the callable to be called with the contents of the optional
    /// @return reference to the expected
    /// @code
    ///     anExpectedOptional.and_then([](int& value){
    ///         std::cout << "the optional contains the value: " << result << std::endl;
    ///     })
    /// @endcode
    ///
    template <typename Optional = ValueType,
              typename std::enable_if<internal::IsOptional<Optional>::value, int>::type = 0>
    expected& and_then(const cxx::function_ref<void(typename Optional::type&)>& callable) noexcept;

    ///
    /// @brief if the expected contains a success value and its type is an empty optional, calls the provided callable
    /// @param[in] callable the callable to be called if the contained optional is empty
    /// @return reference to the expected
    /// @code
    ///     anExpectedOptional.and_then([](SomeType& value){
    ///             std::cout << "we got something in the optional: " << value << std::endl;
    ///         })
    ///         .if_empty([](){
    ///             std::cout << "the optional was empty, but do something anyway!" << result << std::endl;
    ///         })
    /// @endcode
    ///
    template <typename Optional = ValueType,
              typename std::enable_if<internal::IsOptional<Optional>::value, int>::type = 0>
    [[deprecated]] const expected& if_empty(const cxx::function_ref<void()>& callable) const noexcept;

    ///
    /// @brief if the expected contains a success value and its type is an empty optional, calls the provided callable
    /// @param[in] callable the callable to be called if the contained optional is empty
    /// @return reference to the expected
    /// @code
    ///     anExpectedOptional.and_then([](SomeType& value){
    ///             std::cout << "we got something in the optional: " << value << std::endl;
    ///         })
    ///         .if_empty([](){
    ///             std::cout << "the optional was empty, but do something anyway!" << result << std::endl;
    ///         })
    /// @endcode
    ///
    template <typename Optional = ValueType,
              typename std::enable_if<internal::IsOptional<Optional>::value, int>::type = 0>
    [[deprecated]] expected& if_empty(const cxx::function_ref<void()>& callable) noexcept;

    optional<ValueType> to_optional() const noexcept;

  private:
    expected(variant<ValueType, ErrorType>&& f_store, const bool hasError) noexcept;
    variant<ValueType, ErrorType> m_store;
    bool m_hasError;
    static constexpr uint64_t VALUE_INDEX = 0U;
    static constexpr uint64_t ERROR_INDEX = 1U;
};

template <typename ErrorType>
class IOX_NO_DISCARD expected<void, ErrorType> : public expected<ErrorType>
{
  public:
    using expected<ErrorType>::expected;
};


} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/expected.inl"

#endif // IOX_UTILS_CXX_EXPECTED_HPP
