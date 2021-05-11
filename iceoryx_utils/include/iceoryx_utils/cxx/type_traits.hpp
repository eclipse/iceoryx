// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_UTILS_CXX_TYPE_TRAITS_HPP
#define IOX_UTILS_CXX_TYPE_TRAITS_HPP

#include <type_traits>

namespace iox
{
namespace cxx
{
///
/// @brief Conditionally add const to type T if C has the const qualifier
/// @tparam T is the type to conditionally add the const qualifier
/// @tparam Condition is the type which determines if the const qualifier needs to be added to T
///
template <typename T, typename C>
struct add_const_conditionally
{
    using type = T;
};
template <typename T, typename C>
struct add_const_conditionally<T, const C>
{
    using type = const T;
};
///
/// @brief Helper type for add_const_conditionally which adds const to type T if C has the const qualifier
///
template <typename T, typename C>
using add_const_conditionally_t = typename add_const_conditionally<T, C>::type;

///
/// @brief Verifies whether the passed Callable type is in fact invocable with the given arguments
///
template <typename Callable, typename... ArgTypes>
struct is_invocable
{
    // This variant is chosen when Callable(ArgTypes) successfully resolves to a valid type, i.e. is invocable.
    /// @note result_of is deprecated, switch to invoke_result in C++17
    template <typename C, typename... As>
    static constexpr std::true_type test(typename std::result_of<C(As...)>::type*)
    {
        return {};
    }

    // This is chosen if Callable(ArgTypes) does not resolve to a valid type.
    template <typename C, typename... As>
    static constexpr std::false_type test(...)
    {
        return {};
    }

    // Test with nullptr as this can stand in for a pointer to any type.
    static constexpr bool value = decltype(test<Callable, ArgTypes...>(nullptr))::value;
};

///
/// @brief Verfies the signature ReturnType(ArgTypes...) of the provided Callable type
///
template <typename Callable = void, typename ReturnType = void, typename ArgTypes = void>
struct has_signature : std::false_type
{
};

template <typename Callable, typename ReturnType, typename... ArgTypes>
struct has_signature<Callable,
                     ReturnType(ArgTypes...),
                     typename std::enable_if<
                         std::is_convertible<typename std::result_of<Callable(ArgTypes...)>::type, ReturnType>::value,
                         void>::type> : std::true_type
{
};


/// @brief Maps a sequence of any types to the type void
template <typename...>
using void_t = void;
} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_TYPE_TRAITS_HPP
