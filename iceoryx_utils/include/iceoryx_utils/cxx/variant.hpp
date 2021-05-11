// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_CXX_VARIANT_HPP
#define IOX_UTILS_CXX_VARIANT_HPP

#include "iceoryx_utils/cxx/algorithm.hpp"
#include "iceoryx_utils/internal/cxx/variant_internal.hpp"

#include <cstdint>
#include <iostream>
#include <limits>

#include "iceoryx_utils/platform/platform_correction.hpp"

namespace iox
{
namespace cxx
{
/// @brief helper struct to perform an emplacement at a predefined index
///        in the constructor of a variant
/// @param[in] N index where to perform the placement new
/// @code
///     cxx::variant<int, float, int> someVariant(cxx::in_place_index<2>(), 42);
/// @endcode
template <uint64_t N>
struct in_place_index
{
    static constexpr uint64_t value = N;
};

/// @brief helper struct to perform an emplacement of a predefined type in
///        in the constructor of a variant
/// @param[in] T type which should be created
/// @code
///     cxx::variant<int, float, double> someVariant(cxx::in_place_type<float>(), 123.456f);
/// @endcode
template <typename T>
struct in_place_type
{
    using type = T;
};

/// @brief value which an invalid variant index occupies
/// @code
///     cxx::variant<int, float> someVariant;
///
///     // every unset variant has an invalid variant in the beginning
///     if ( someVariant.index() == INVALID_VARIANT_INDEX ) ...
///
///     cxx::variant<int, float> someVariant2(cxx::in_place_type<int>, 12);
///
///     // variant with setted value therefore the index is not invalid
///     if ( someVariant.index() != INVALID_VARIANT_INDEX ) ...
/// @endcode
static constexpr uint64_t INVALID_VARIANT_INDEX = std::numeric_limits<uint64_t>::max();

/// @brief Variant implementation from the C++17 standard with C++11. The
///         interface is inspired by the C++17 standard but it has changes in
///         get and emplace since we are not allowed to throw exceptions.
/// @param Types... variadic list of types which the variant should be able to store
///
/// @code
///     #include "iceoryx_utils/cxx/variant.hpp"
///     #include <iostream>
///
///     cxx::variant<int, float, double> someVariant;
///
///     // ... do stuff
///
///     if ( someVariant.index() == INVALID_VARIANT_INDEX )
///     {
///         someVariant.emplace<float>(123.456f);
///     }
///     else if ( someVariant.index() == 1)
///     {
///         auto blubb = someVariant.template get_at_index<1>();
///         std::cout << *blubb << std::endl;
///
///         auto sameAsBlubb = someVariant.get<float>();
///         std::cout << *sameAsBlubb << std::endl;
///     }
///
///     // .. do stuff
///
///     int defaultValue = 123;
///     int * fuu = someVariant.get_if<int>(&defaultValue);
///     std::cout << *fuu << std::endl;
///
/// @endcode
template <typename... Types>
class variant
{
  private:
    /// @brief contains the size of the largest element
    static constexpr uint64_t TYPE_SIZE = algorithm::max(sizeof(Types)...);

  public:
    /// @brief the default constructor constructs a variant which does not contain
    ///     an element and returns INVALID_VARIANT_INDEX when .index() is called
    variant() = default;

    /// @brief creates a variant and perform an in place construction of the type
    ///         stored at index N. If the index N is out of bounds you get a compiler
    ///         error.
    /// @param[in] index index of the type which should be constructed
    /// @param[in] args variadic list of arguments which will be forwarded to the constructor to
    ///                 the type at index
    template <uint64_t N, typename... CTorArguments>
    variant(const in_place_index<N>& index, CTorArguments&&... args) noexcept;

    /// @brief creates a variant and perform an in place construction of the type T.
    ///         If T is not part of the variant you get a compiler error.
    /// @param[in] type type which should be created inside the variant
    /// @param[in] args variadic list of arguments which will be forwarded to the constructor to
    ///                 the type
    template <typename T, typename... CTorArguments>
    variant(const in_place_type<T>& type, CTorArguments&&... args) noexcept;

    /// @brief if the variant contains an element the elements copy constructor is called
    ///     otherwise an empty variant is copied
    /// @param[in] rhs source of the copy
    variant(const variant& rhs) noexcept;

    /// @brief if the variant contains an element the elements copy assignment operator is called
    ///     otherwise an empty variant is copied
    /// @param[in] rhs source of the copy assignment
    /// @return reference to the variant itself
    variant& operator=(const variant& rhs) noexcept;

    /// @brief if the variant contains an element the elements move constructor is called
    ///     otherwise an empty variant is moved
    /// @param[in] rhs source of the move
    variant(variant&& rhs) noexcept;

    /// @brief if the variant contains an element the elements move assignment operator is called
    ///     otherwise an empty variant is moved
    /// @param[in] rhs source of the move assignment
    /// @return reference to the variant itself
    variant& operator=(variant&& rhs) noexcept;

    /// @brief if the variant contains an element the elements destructor is called otherwise
    ///         nothing happens
    ~variant() noexcept;

    /// @brief if the variant contains an element the elements assignment operator is called otherwise
    ///         we have undefined behavior. It is important that you make sure that the variant really
    ///         contains that type T.
    /// @param[in] rhs source object for the underlying move assignment
    /// @return reference to the variant itself
    template <typename T>
    typename std::enable_if<!std::is_same<T, variant<Types...>&>::value, variant<Types...>>::type&
    operator=(T&& rhs) noexcept;

    /// @brief calls the constructor of the type at index TypeIndex and perfectly forwards the arguments
    ///         to this constructor. (not stl compliant)
    /// @param TypeIndex index of the type which will be created
    /// @param[in] args arguments which will be forwarded to the constructor to the type at TypeIndex
    /// @return if the variant already contains a different type it returns false, if the construction
    ///         was successful it returns true
    template <uint64_t TypeIndex, typename... CTorArguments>
    bool emplace_at_index(CTorArguments&&... args) noexcept;

    /// @brief calls the constructor of the type T and perfectly forwards the arguments
    ///         to the constructor of T.
    /// @return if the variant already contains a different type it returns false, if the construction
    ///         was successful it returns true
    template <typename T, typename... CTorArguments>
    bool emplace(CTorArguments&&... args) noexcept;

    /// @brief returns a pointer to the type stored at index TypeIndex. (not stl compliant)
    /// @return if the variant does contain the type at index TypeIndex it returns a valid
    ///             pointer, if it does contain no type at all or a different type it returns
    ///             nullptr.
    /// @code
    ///     cxx::variant<int, float> someVariant(cxx::in_place_type<int>(), 12);
    ///     int * someNumber = someVariant.template get_at_index<0>();
    /// @endcode
    template <uint64_t TypeIndex>
    typename internal::get_type_at_index<0, TypeIndex, Types...>::type* get_at_index() noexcept;

    /// @brief returns a pointer to the type stored at index TypeIndex. (not stl compliant)
    /// @return if the variant does contain the type at index TypeIndex it returns a valid
    ///             pointer, if it does contain no type at all or a different type it returns
    ///             nullptr.
    /// @code
    ///     cxx::variant<int, float> someVariant(cxx::in_place_type<int>(), 12);
    ///     int * someNumber = someVariant.template get_at_index<0>();
    /// @endcode
    template <uint64_t TypeIndex>
    const typename internal::get_type_at_index<0, TypeIndex, Types...>::type* get_at_index() const noexcept;

    /// @brief returns a pointer to the type T stored in the variant. (not stl compliant)
    /// @return if the variant does contain the type T it returns a valid pointer otherwise
    ///         if the variant does contain no type at all or a different type it returns
    ///         nullptr
    template <typename T>
    const T* get() const noexcept;

    /// @brief returns a pointer to the type T stored in the variant. (not stl compliant)
    /// @return if the variant does contain the type T it returns a valid pointer otherwise
    ///         if the variant does contain no type at all or a different type it returns
    ///         nullptr
    template <typename T>
    T* get() noexcept;

    /// @brief returns a pointer to the type T if its stored in the variant otherwise
    ///         it returns the provided defaultValue
    /// @return pointer to the stored value if it is of type T, otherwise defaultValue
    template <typename T>
    T* get_if(T* defaultValue) noexcept;

    /// @brief returns a pointer to the type T if its stored in the variant otherwise
    ///         it returns the provided defaultValue
    /// @return pointer to the stored value if it is of type T, otherwise defaultValue
    template <typename T>
    const T* get_if(const T* defaultValue) const noexcept;

    /// @brief returns the index of the stored type in the variant. if the variant does
    ///         not contain any type it returns INVALID_VARIANT_INDEX
    /// @return index of the stored type
    constexpr uint64_t index() const noexcept;

  private:
    alignas(algorithm::max(alignof(Types)...)) internal::byte_t m_storage[TYPE_SIZE]{0u};
    uint64_t m_type_index = INVALID_VARIANT_INDEX;

  private:
    template <typename T>
    bool has_bad_variant_element_access() const noexcept;
    static void error_message(const char* f_source, const char* f_msg) noexcept;

    void call_element_destructor() noexcept;
};

/// @brief returns true if the variant holds a given type T, otherwise false
template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>& f_variant) noexcept;

} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/variant.inl"

#endif // IOX_UTILS_CXX_VARIANT_HPP
