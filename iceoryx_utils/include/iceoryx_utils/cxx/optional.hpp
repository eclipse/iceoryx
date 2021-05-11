// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_UTILS_CXX_OPTIONAL_HPP
#define IOX_UTILS_CXX_OPTIONAL_HPP

#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/types.hpp"

#include <new> // needed for placement new in the construct_value member function
#include <utility>

namespace iox
{
namespace cxx
{
/// @brief Helper struct which is used to signal an empty optional.
///         It is equivalent to no value.
struct nullopt_t
{
};
constexpr nullopt_t nullopt = nullopt_t();

/// @brief Optional implementation from the C++17 standard with C++11. The
///         interface is analog to the C++17 standard and it can be used in
///         factory functions which can fail.
///
/// @code
///     #include "iceoryx_utils/cxx/optional.hpp"
///
///     cxx::optional<void*> SomeFactory() {
///         void *memory = malloc(1234);
///         if ( memory == nullptr )
///             return cxx::nullopt_t();
///         else
///             return cxx::make_optional<void*>(memory);
///     }
///
///     int main() {
///         auto var = SomeFactory();
///         // never forget the has_value call before working with an optional
///         if ( var.has_value() ) {
///             // do stuff with var
///         }
///     }
/// @endcode
template <typename T>
class optional
{
  public:
    using type = T;

    /// @brief Creates an optional which has no value. If you access such an
    ///         optional via .value() or the arrow operator the behavior is
    ///         undefined.
    optional() noexcept;

    /// @brief Creates an optional which has no value. If you access such an
    ///         optional via .value() or the arrow operator the behavior is
    ///         defined in the cxx::Expects handling.
    optional(const nullopt_t&) noexcept;

    /// @brief Creates an optional by forwarding value to the constructor of
    ///         T. This optional has a value.
    /// @param[in] value rvalue of type T which will be moved into the optional
    optional(T&& value) noexcept;

    /// @brief Creates an optional by using the copy constructor of T.
    /// @param[in] value lvalue of type T which will be copy constructed into the optional
    optional(const T& value) noexcept;

    /// @brief The destructor will call the destructor of T if a value is set.
    ~optional() noexcept;

    /// @brief Constructs a value with the copy constructor if rhs has a value.
    ///         Otherwise it contains no value.
    /// @param[in] rhs source of the copy
    optional(const optional& rhs) noexcept;

    /// @brief Constructs a value with the move constructor if rhs has a value.
    ///         Otherwise it contains no value.
    /// @param[in] rhs source of the move
    optional(optional&& rhs) noexcept;

    /// @brief Copies an optional. If the optional has a value then the copy
    ///         assignment of that value is called. If the optional has no
    ///         value a new value is constructed with the copy constructor.
    /// @param[in] rhs source of the copy
    /// @return reference to the current optional
    optional& operator=(const optional& rhs) noexcept;

    /// @brief Moves an optional. If the optional has a value then the move
    ///         assignment of that value is called. If the optional has no
    ///         value a new value is constructed with the move constructor.
    /// @param[in] rhs source of the move
    /// @return reference to the current optional
    optional& operator=(optional&& rhs) noexcept;

    /// @brief If the optionals have values it compares these values by using
    ///         their comparision operator.
    /// @param[in] rhs value to which this optional should be compared to
    /// @return true if the contained values are equal, otherwise false
    constexpr bool operator==(const optional<T>& rhs) const noexcept;

    /// @brief Comparision with nullopt_t for easier unset optional comparision
    /// @return true if the optional is unset, otherwise false
    constexpr bool operator==(const nullopt_t&) const noexcept;

    /// @brief If the optionals have values it compares these values by using
    ///         their comparision operator.
    /// @param[in] rhs value to which this optional should be compared to
    /// @return true if the contained values are not equal, otherwise false
    constexpr bool operator!=(const optional<T>& rhs) const noexcept;

    /// @brief Comparision with nullopt_t for easier unset optional comparision
    /// @return true if the optional is set, otherwise false
    constexpr bool operator!=(const nullopt_t&) const noexcept;

    /// @brief Direct assignment of the underlying value. If the optional has no
    ///         value then a new T is constructed by forwarding the assignment to
    ///         T's constructor.
    ///        If the optional has a value the assignment operator of T is called.
    /// @param[in] value value to assign to the underlying optional value
    /// @return reference to the current optional
    template <typename U = T>
    typename std::enable_if<!std::is_same<U, optional<T>&>::value, optional>::type& operator=(U&& value) noexcept;

    /// @brief Returns a pointer to the underlying value. If the optional has no
    ///         value the behavior is undefined. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return pointer of type const T to the underlying type
    const T* operator->() const noexcept;

    /// @brief Returns a reference to the underlying value. If the optional has no
    ///         value the behavior is undefined. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return reference of type const T to the underlying type
    const T& operator*() const noexcept;

    /// @brief Returns a pointer to the underlying value. If the optional has no
    ///         value the behavior is undefined. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return pointer of type T to the underlying type
    T* operator->() noexcept;

    /// @brief Returns a reference to the underlying value. If the optional has no
    ///         value the behavior is undefined. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return reference of type T to the underlying type
    T& operator*() noexcept;

    /// @brief Will return true if the optional contains a value, otherwise false.
    /// @return true if optional contains a value, otherwise false
    constexpr explicit operator bool() const noexcept;

    /// @brief Will return true if the optional contains a value, otherwise false.
    /// @return true if optional contains a value, otherwise false
    constexpr bool has_value() const noexcept;

    /// @brief A new element is constructed by forwarding the arguments to the
    ///         constructor of T.
    ///         If the optional has a value then the destructor of T is called.
    /// @param[in] perfectly forwards args to the constructor of T to perform a placement
    ///             new
    /// @return reference to the underlying type
    template <typename... Targs>
    T& emplace(Targs&&... args) noexcept;

    /// @brief Calls the destructor of T if the optional has a value.
    ///         If the optional has no value, nothing happens. After that call
    ///         the optional has no more value.
    void reset() noexcept;

    /// @brief Returns a reference to the underlying value. If the optional has no
    ///         value the application terminates. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return reference to the underlying type
    T& value() & noexcept;

    /// @brief Returns a const reference to the underlying value. If the optional has no
    ///         value the application terminates. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return const reference to the underlying type
    const T& value() const& noexcept;

    /// @brief Returns a rvalue reference to the underlying value. If the optional has no
    ///         value the application terminates. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return rvalue reference to the underlying type
    T&& value() && noexcept;

    /// @brief Returns a const rvalue reference to the underlying value. If the optional has no
    ///         value the application terminates. You need to verify that the
    ///         optional has a value by calling has_value() before using it.
    /// @return const rvalue reference to the underlying type
    const T&& value() const&& noexcept;

    /// @brief If the optional contains a value a copy of that value is returned,
    ///         otherwise the default_value is returned.
    /// @return copy of the underlying type if the optional has a value otherwise
    ///         a copy of default_value
    template <typename U>
    constexpr T value_or(U&& default_value) const noexcept;

    /// @brief calls the provided callable with the optional value as arguments
    ///         if the optional contains a value
    /// @param[in] callable which has T as argument
    /// @return reference to this
    optional& and_then(const cxx::function_ref<void(T&)>& callable) noexcept;

    /// @brief calls the provided callable with the optional value as arguments
    ///         if the optional contains a value
    /// @param[in] callable which has T as argument
    /// @return reference to this
    const optional& and_then(const cxx::function_ref<void(const T&)>& callable) const noexcept;

    /// @brief calls the provided callable if the optional does not contain a value
    /// @param[in] callable
    /// @return reference to this
    optional& or_else(const cxx::function_ref<void()>& callable) noexcept;

    /// @brief calls the provided callable if the optional does not contain a value
    /// @param[in] callable
    /// @return reference to this
    const optional& or_else(const cxx::function_ref<void()>& callable) const noexcept;

  private:
    alignas(T) byte_t m_data[sizeof(T)];
    bool m_hasValue{false};

  private:
    template <typename... Targs>
    void construct_value(Targs&&... args) noexcept;
    void destruct_value() noexcept;
};

/// @brief Creates an optional which contains a value by forwarding the arguments
///         to the constructor of T.
/// @param[in] args arguments which will be perfectly forwarded to the constructor
///             of T
/// @return optional which contains T constructed with args
template <typename OptionalBaseType, typename... Targs>
optional<OptionalBaseType> make_optional(Targs&&... args) noexcept;
} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/optional.inl"

#endif // IOX_UTILS_CXX_OPTIONAL_HPP
