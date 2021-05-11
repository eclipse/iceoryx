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
#ifndef IOX_UTILS_CXX_ALGORITHM_INL
#define IOX_UTILS_CXX_ALGORITHM_INL

namespace iox
{
namespace algorithm
{
template <typename T>
inline constexpr T max(const T& left) noexcept
{
    return left;
}

template <typename T>
inline constexpr T max(const T& left, const T& right) noexcept
{
    return (right < left) ? left : right;
}

template <typename T, typename... Targs>
inline constexpr T max(const T& left, const T& right, const Targs&... args) noexcept
{
    return max(max(left, right), args...);
}

template <typename T>
inline constexpr T min(const T& left) noexcept
{
    return left;
}

template <typename T>
inline constexpr T min(const T& left, const T& right) noexcept
{
    return (left < right) ? left : right;
}

template <typename T, typename... Targs>
inline constexpr T min(const T& left, const T& right, const Targs&... args) noexcept
{
    return min(min(left, right), args...);
}

template <typename T, typename CompareType>
inline constexpr bool doesContainType() noexcept
{
    return std::is_same<T, CompareType>::value;
}

template <typename T, typename CompareType, typename Next, typename... Remainder>
inline constexpr bool doesContainType() noexcept
{
    return doesContainType<T, CompareType>() || doesContainType<T, Next, Remainder...>();
}

template <typename Container>
inline Container uniqueMergeSortedContainers(const Container& v1, const Container& v2) noexcept
{
    Container mergedContainer;
    uint64_t indexV1 = 0U, indexV2 = 0U;
    uint64_t v1Size = v1.size();
    uint64_t v2Size = v2.size();

    while (indexV1 < v1Size && indexV2 < v2Size)
    {
        if (v1[indexV1] == v2[indexV2])
        {
            mergedContainer.emplace_back(v1[indexV1]);
            ++indexV1;
            ++indexV2;
        }
        else if (v1[indexV1] < v2[indexV2])
        {
            mergedContainer.emplace_back(v1[indexV1]);
            ++indexV1;
        }
        else
        {
            mergedContainer.emplace_back(v2[indexV2]);
            ++indexV2;
        }
    }

    while (indexV2 < v2Size)
    {
        mergedContainer.emplace_back(v2[indexV2++]);
    }

    while (indexV1 < v1Size)
    {
        mergedContainer.emplace_back(v1[indexV1++]);
    }

    return mergedContainer;
}


} // namespace algorithm
} // namespace iox

#endif // IOX_UTILS_CXX_ALGORITHM_INL
