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
#ifndef IOX_UTILS_CXX_ALGORITHM_HPP
#define IOX_UTILS_CXX_ALGORITHM_HPP

#include <cstdint>
#include <type_traits>

namespace iox
{
namespace algorithm
{
/// @brief Returns the maximum gained with operator<() of an arbitrary amount
///          of variables of the same type. Helper function which is required as generic
///          recursive template endpoint.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @return returns the given argument left
template <typename T>
constexpr T max(const T& left) noexcept;

/// @brief Returns the maximum gained with operator<() of an arbitrary amount
///          of variables of the same type. Helper function which takes two arguments and returns the
///          greater one.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @param[in] right value which should be compared
/// @return returns the maximum value of the set {left, right}
template <typename T>
constexpr T max(const T& left, const T& right) noexcept;

/// @brief Returns the maximum gained with operator<() of an arbitrary amount
///          of variables of the same type.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @param[in] right value which should be compared
/// @param[in] args... an arbitrary amount of values
/// @return returns the maximum value of the set {left, right, args...}
template <typename T, typename... Targs>
constexpr T max(const T& left, const T& right, const Targs&... args) noexcept;

/// @brief Returns the minimum gained with operator<() of an arbitrary amount
///          of variables of the same type. Helper function which is required as generic
///          recursive template endpoint.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @return returns the given argument left
template <typename T>
constexpr T min(const T& left) noexcept;

/// @brief Returns the minimum gained with operator<() of an arbitrary amount
///          of variables of the same type. Helper function which takes two arguments and returns the
///          smaller one.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @param[in] right value which should be compared
/// @return returns the minimum of the set {left, right}
template <typename T>
constexpr T min(const T& left, const T& right) noexcept;

/// @brief Returns the minimum gained with operator<() of an arbitrary amount
///          of variables of the same type.
/// @param T type which implements operator<()
/// @param[in] left value which should be compared
/// @param[in] right value which should be compared
/// @param[in] args... an arbitrary amount of values
/// @return returns the minimum of the set {left, right, args...}
template <typename T, typename... Targs>
constexpr T min(const T& left, const T& right, const Targs&... args) noexcept;

/// @brief Returns true if T is equal to CompareType, otherwise false
/// @param T type to compare to
/// @param CompareType the type to which T is compared
/// @return true if the types T and CompareType are equal, otherwise false
template <typename T, typename CompareType>
constexpr bool doesContainType() noexcept;

/// @brief Returns true if T is contained the provided type list
/// @param T type to compare to
/// @param CompareType, Next, Remainder the type list in which T should be contained
/// @return true if the T is contained in the type list, otherwise false
template <typename T, typename CompareType, typename Next, typename... Remainder>
constexpr bool doesContainType() noexcept;

/// @brief Merging two sorted containers so that the result is a sorted container
///        where every element is contained only once
/// @tparam Container container type which has to support emplace_back() and size()
/// @param[in] v1 the first sorted input container
/// @param[in] v2 the second sorted input container
/// @return sorted container which contains the elements of v1 and v2 and where every element is unique
template <typename Container>
Container uniqueMergeSortedContainers(const Container& v1, const Container& v2) noexcept;
} // namespace algorithm
} // namespace iox

#include "iceoryx_utils/internal/cxx/algorithm.inl"

#endif // IOX_UTILS_CXX_ALGORITHM_HPP
