// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2021 by Perforce All rights reserved.
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
#ifndef IOX_UTILS_CXX_VARIANT_INL
#define IOX_UTILS_CXX_VARIANT_INL

#include "iceoryx_utils/cxx/variant.hpp"

namespace iox
{
namespace cxx
{
template <typename... Types>
inline variant<Types...>::variant(const variant& f_rhs) noexcept
    : m_type_index(f_rhs.m_type_index)
{
    if (m_type_index != INVALID_VARIANT_INDEX)
    {
        internal::call_at_index<0, Types...>::copyConstructor(
            m_type_index, const_cast<internal::byte_t*>(f_rhs.m_storage), m_storage);
    }
}

template <typename... Types>
template <uint64_t N, typename... CTorArguments>
inline variant<Types...>::variant(const in_place_index<N>&, CTorArguments&&... f_args) noexcept
{
    emplace_at_index<N>(std::forward<CTorArguments>(f_args)...);
}

template <typename... Types>
template <typename T, typename... CTorArguments>
inline variant<Types...>::variant(const in_place_type<T>&, CTorArguments&&... f_args) noexcept
{
    emplace<T>(std::forward<CTorArguments>(f_args)...);
}

template <typename... Types>
variant<Types...>& variant<Types...>::operator=(const variant& f_rhs) noexcept
{
    if (this != &f_rhs)
    {
        if (m_type_index != f_rhs.m_type_index)
        {
            call_element_destructor();
            m_type_index = f_rhs.m_type_index;

            if (m_type_index != INVALID_VARIANT_INDEX)
            {
                internal::call_at_index<0, Types...>::copyConstructor(
                    m_type_index, const_cast<internal::byte_t*>(f_rhs.m_storage), m_storage);
            }
        }
        else
        {
            if (m_type_index != INVALID_VARIANT_INDEX)
            {
                internal::call_at_index<0, Types...>::copy(
                    m_type_index, const_cast<internal::byte_t*>(f_rhs.m_storage), m_storage);
            }
        }
    }
    return *this;
}

template <typename... Types>
variant<Types...>::variant(variant&& f_rhs) noexcept
    : m_type_index{std::move(f_rhs.m_type_index)}
{
    if (m_type_index != INVALID_VARIANT_INDEX)
    {
        internal::call_at_index<0, Types...>::moveConstructor(m_type_index, f_rhs.m_storage, m_storage);
    }
}

template <typename... Types>
variant<Types...>& variant<Types...>::operator=(variant&& f_rhs) noexcept
{
    if (this != &f_rhs)
    {
        if (m_type_index != f_rhs.m_type_index)
        {
            call_element_destructor();
            m_type_index = std::move(f_rhs.m_type_index);
            if (m_type_index != INVALID_VARIANT_INDEX)
            {
                internal::call_at_index<0, Types...>::moveConstructor(m_type_index, f_rhs.m_storage, m_storage);
            }
        }
        else
        {
            if (m_type_index != INVALID_VARIANT_INDEX)
            {
                internal::call_at_index<0, Types...>::move(m_type_index, f_rhs.m_storage, m_storage);
            }
        }
    }
    return *this;
}

template <typename... Types>
inline variant<Types...>::~variant() noexcept
{
    call_element_destructor();
}

template <typename... Types>
void variant<Types...>::call_element_destructor() noexcept
{
    if (m_type_index != INVALID_VARIANT_INDEX)
    {
        internal::call_at_index<0, Types...>::destructor(m_type_index, m_storage);
    }
}

template <typename... Types>
template <typename T>
inline typename std::enable_if<!std::is_same<T, variant<Types...>&>::value, variant<Types...>>::type&
variant<Types...>::operator=(T&& f_rhs) noexcept
{
    if (m_type_index == INVALID_VARIANT_INDEX)
    {
        m_type_index = internal::get_index_of_type<0, T, Types...>::index;
    }

    if (!has_bad_variant_element_access<T>())
    {
        auto storage = static_cast<T*>(static_cast<void*>(m_storage));
        *storage = (std::forward<T>(f_rhs));
    }
    else
    {
        error_message(__PRETTY_FUNCTION__,
                      "wrong variant type assignment, another type is already "
                      "set in variant");
    }

    return *this;
}

template <typename... Types>
template <uint64_t TypeIndex, typename... CTorArguments>
inline bool variant<Types...>::emplace_at_index(CTorArguments&&... f_args) noexcept
{
    static_assert(TypeIndex <= sizeof...(Types), "TypeIndex is out of bounds");

    using T = typename internal::get_type_at_index<0, TypeIndex, Types...>::type;

    call_element_destructor();
    new (m_storage) T(std::forward<CTorArguments>(f_args)...);
    m_type_index = TypeIndex;

    return true;
}

template <typename... Types>
template <typename T, typename... CTorArguments>
inline bool variant<Types...>::emplace(CTorArguments&&... f_args) noexcept
{
    if (m_type_index != INVALID_VARIANT_INDEX && has_bad_variant_element_access<T>())
    {
        error_message(__PRETTY_FUNCTION__,
                      "wrong variant type emplacement, another type is already "
                      "set in variant");
        return false;
    }

    if (m_type_index != INVALID_VARIANT_INDEX)
    {
        call_element_destructor();
    }

    new (m_storage) T(std::forward<CTorArguments>(f_args)...);
    m_type_index = internal::get_index_of_type<0, T, Types...>::index;

    return true;
}

template <typename... Types>
template <uint64_t TypeIndex>
inline typename internal::get_type_at_index<0, TypeIndex, Types...>::type* variant<Types...>::get_at_index() noexcept
{
    if (TypeIndex != m_type_index)
    {
        return nullptr;
    }

    using T = typename internal::get_type_at_index<0, TypeIndex, Types...>::type;

    return static_cast<T*>(static_cast<void*>(m_storage));
}

template <typename... Types>
template <uint64_t TypeIndex>
inline const typename internal::get_type_at_index<0, TypeIndex, Types...>::type*
variant<Types...>::get_at_index() const noexcept
{
    using T = typename internal::get_type_at_index<0, TypeIndex, Types...>::type;
    return const_cast<const T*>(const_cast<variant*>(this)->template get_at_index<TypeIndex>());
}

template <typename... Types>
template <typename T>
inline const T* variant<Types...>::get() const noexcept
{
    if (has_bad_variant_element_access<T>())
    {
        return nullptr;
    }
    else
    {
        return static_cast<const T*>(static_cast<const void*>(m_storage));
    }
}

template <typename... Types>
template <typename T>
inline T* variant<Types...>::get() noexcept
{
    return const_cast<T*>(const_cast<const variant*>(this)->get<T>());
}

template <typename... Types>
template <typename T>
inline T* variant<Types...>::get_if(T* f_default_value) noexcept
{
    return const_cast<T*>(const_cast<const variant*>(this)->get_if<T>(const_cast<const T*>(f_default_value)));
}

template <typename... Types>
template <typename T>
inline const T* variant<Types...>::get_if(const T* f_default_value) const noexcept
{
    if (has_bad_variant_element_access<T>())
    {
        return f_default_value;
    }

    return this->get<T>();
}

template <typename... Types>
constexpr uint64_t variant<Types...>::index() const noexcept
{
    return m_type_index;
}

template <typename... Types>
template <typename T>
inline bool variant<Types...>::has_bad_variant_element_access() const noexcept
{
    static_assert(internal::does_contain_type<T, Types...>::value, "variant does not contain given type");
    return (m_type_index != internal::get_index_of_type<0, T, Types...>::index);
}

template <typename... Types>
inline void variant<Types...>::error_message(const char* f_source, const char* f_msg) noexcept
{
    std::cerr << f_source << " ::: " << f_msg << std::endl;
}

template <typename T, typename... Types>
inline constexpr bool holds_alternative(const variant<Types...>& f_variant) noexcept
{
    return f_variant.template get<T>() != nullptr;
}
} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_VARIANT_INL
