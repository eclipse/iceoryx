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
#ifndef IOX_UTILS_CXX_HELPLETS_HPP
#define IOX_UTILS_CXX_HELPLETS_HPP

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#include "iceoryx_utils/platform/platform_correction.hpp"

namespace iox
{
namespace cxx
{
namespace internal
{
inline void
Require(const bool condition, const char* file, const int line, const char* function, const char* conditionString)
{
    if (!condition)
    {
        std::cerr << "Condition: " << conditionString << " in " << function << " is violated. (" << file << ":" << line
                  << ")" << std::endl;
        std::terminate();
    }
}

/// @brief struct to find the best fitting unsigned integer type
template <bool GreaterUint8, bool GreaterUint16, bool GreaterUint32>
struct BestFittingTypeImpl
{
    using Type_t = uint64_t;
};

template <>
struct BestFittingTypeImpl<false, false, false>
{
    using Type_t = uint8_t;
};

template <>
struct BestFittingTypeImpl<true, false, false>
{
    using Type_t = uint16_t;
};

template <>
struct BestFittingTypeImpl<true, true, false>
{
    using Type_t = uint32_t;
};
} // namespace internal

// implementing C++ Core Guideline, I.6. Prefer Expects
// see:
// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-expects
#define Expects(condition) internal::Require(condition, __FILE__, __LINE__, __PRETTY_FUNCTION__, #condition)

// implementing C++ Core Guideline, I.8. Prefer Ensures
// see:
// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-ensures
#define Ensures(condition) internal::Require(condition, __FILE__, __LINE__, __PRETTY_FUNCTION__, #condition)

template <typename T, typename = typename std::enable_if<std::is_pointer<T>::value, void>::type>
struct not_null
{
  public:
    not_null(T t)
        : value(t)
    {
        Expects(t != nullptr);
    }

    constexpr operator T() const
    {
        return value;
    }

  private:
    T value;
};

template <typename T, T Minimum>
struct greater_or_equal
{
  public:
    greater_or_equal(T t)
        : value(t)
    {
        Expects(t >= Minimum);
    }

    constexpr operator T() const
    {
        return value;
    }

  private:
    T value;
};

template <typename T, T Minimum, T Maximum>
struct range
{
  public:
    range(T t)
        : value(t)
    {
        Expects(t >= Minimum && t <= Maximum);
    }

    constexpr operator T() const
    {
        return value;
    }

  private:
    T value;
};

template <typename T>
T align(const T value, const T alignment)
{
    T remainder = value % alignment;
    return value + ((remainder == 0u) ? 0u : alignment - remainder);
}

/// @brief allocates aligned memory which can only be free'd by alignedFree
/// @param[in] alignment, alignment of the memory
/// @param[in] size, memory size
/// @return void pointer to the aligned memory
void* alignedAlloc(const uint64_t alignment, const uint64_t size) noexcept;

/// @brief frees aligned memory allocated with alignedAlloc
/// @param[in] memory, pointer to the aligned memory
void alignedFree(void* const memory);

/// template recursion stopper for maximum alignment calculation
template <size_t s = 0>
constexpr size_t maxAlignment()
{
    return s;
}

/// calculate maximum alignment of supplied types
template <typename T, typename... Args>
constexpr size_t maxAlignment()
{
    return alignof(T) > maxAlignment<Args...>() ? alignof(T) : maxAlignment<Args...>();
}

/// template recursion stopper for maximum size calculation
template <size_t s = 0>
constexpr size_t maxSize()
{
    return s;
}

/// calculate maximum size of supplied types
template <typename T, typename... Args>
constexpr size_t maxSize()
{
    return sizeof(T) > maxSize<Args...>() ? sizeof(T) : maxSize<Args...>();
}

/// Convert Enum class type to string
template <typename T, typename Enumeration>
const char* convertEnumToString(T port, const Enumeration source)
{
    return port[static_cast<size_t>(source)];
}

/// cast an enum to its underlying type
template <typename enum_type>
auto enumTypeAsUnderlyingType(enum_type const value) -> typename std::underlying_type<enum_type>::type
{
    return static_cast<typename std::underlying_type<enum_type>::type>(value);
}

/// calls a given functor for every element in a given container
/// @tparam[in] Container type which must be iteratable
/// @tparam[in] Functor which has one argument, the element type of the container
/// @param[in] c container which should be iterated
/// @param[in] f functor which should be applied to every element
template <typename Container, typename Functor>
void forEach(Container& c, const Functor& f) noexcept
{
    for (auto& element : c)
    {
        f(element);
    }
}

/// @brief Get the size of a string represented by a char array at compile time.
/// @tparam The size of the char array filled out by the compiler.
/// @param[in] The actual content of the char array is not of interest. Its just the size of the array that matters.
/// @return Returns the size of a char array at compile time.
template <uint64_t SizeValue>
static constexpr uint64_t strlen2(char const (&/*notInterested*/)[SizeValue])
{
    return SizeValue - 1;
}

/// @brief get the best fitting unsigned integer type for a given value at compile time
template <uint64_t Value>
struct BestFittingType
{
/// ignore the warnings because we need the comparisons to find the best fitting type
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    using Type_t = typename internal::BestFittingTypeImpl<(Value > std::numeric_limits<uint8_t>::max()),
                                                          (Value > std::numeric_limits<uint16_t>::max()),
                                                          (Value > std::numeric_limits<uint32_t>::max())>::Type_t;
#pragma GCC diagnostic pop
};

template <uint64_t Value>
using BestFittingType_t = typename BestFittingType<Value>::Type_t;

/// @brief Returns info whether called on a 32-bit system
/// @return True if called on 32-bit, false if not 32-bit system
constexpr bool isCompiledOn32BitSystem()
{
    return INTPTR_MAX == INT32_MAX;
}

/// @brief Checks if an unsigned integer is a power of two
/// @return true if power of two, otherwise false
template <typename T>
constexpr bool isPowerOfTwo(const T n)
{
    static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Only unsigned integer are allowed!");
    return n && ((n & (n - 1U)) == 0U);
}

} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_HELPLETS_HPP
