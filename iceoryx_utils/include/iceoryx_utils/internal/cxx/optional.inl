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
#ifndef IOX_UTILS_CXX_OPTIONAL_INL
#define IOX_UTILS_CXX_OPTIONAL_INL

namespace iox
{
namespace cxx
{
template <typename T>
inline optional<T>::optional(const nullopt_t&) noexcept
{
}

template <typename T>
inline optional<T>::optional() noexcept
    : optional(nullopt_t())
{
}

template <typename T>
inline optional<T>::optional(T&& value) noexcept
{
    construct_value(std::forward<T>(value));
}

template <typename T>
inline optional<T>::optional(const T& value) noexcept
{
    construct_value(value);
}

template <typename T>
inline optional<T>::optional(const optional& rhs) noexcept
{
    if (rhs.m_hasValue)
    {
        construct_value(rhs.value());
    }
}

template <typename T>
inline optional<T>::optional(optional&& rhs) noexcept
{
    if (rhs.m_hasValue)
    {
        construct_value(std::move(rhs.value()));
        rhs.destruct_value();
    }
}

template <typename T>
inline optional<T>& optional<T>::operator=(const optional& rhs) noexcept
{
    if (this != &rhs)
    {
        if (!rhs.m_hasValue && m_hasValue)
        {
            destruct_value();
        }
        else if (rhs.m_hasValue && m_hasValue)
        {
            value() = rhs.value();
        }
        else if (rhs.m_hasValue && !m_hasValue)
        {
            construct_value(rhs.value());
        }
    }
    return *this;
}

template <typename T>
inline optional<T>& optional<T>::operator=(optional&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (!rhs.m_hasValue && m_hasValue)
        {
            destruct_value();
        }
        else if (rhs.m_hasValue && m_hasValue)
        {
            value() = std::move(rhs.value());
        }
        else if (rhs.m_hasValue && !m_hasValue)
        {
            construct_value(std::move(rhs.value()));
        }
        if (rhs.m_hasValue)
        {
            rhs.destruct_value();
        }
    }
    return *this;
}

template <typename T>
inline optional<T>::~optional() noexcept
{
    if (m_hasValue)
    {
        destruct_value();
    }
}

template <typename T>
template <typename U>
inline typename std::enable_if<!std::is_same<U, optional<T>&>::value, optional<T>>::type&
optional<T>::operator=(U&& newValue) noexcept
{
    if (m_hasValue)
    {
/// @todo broken msvc compiler, see:
///  https://developercommunity.visualstudio.com/content/problem/858688/stdforward-none-of-these-2-overloads-could-convert.html
/// remove this as soon as it is fixed;
#ifdef _WIN32
        value() = newValue;
#else
        value() = std::forward<T>(newValue);
#endif
    }
    else
    {
/// @todo again broken msvc compiler
#ifdef _WIN32
        construct_value(newValue);
#else
        construct_value(std::forward<T>(newValue));
#endif
    }
    return *this;
}

template <typename T>
constexpr inline bool optional<T>::operator==(const optional<T>& rhs) const noexcept
{
    return (!m_hasValue && !rhs.m_hasValue) || ((m_hasValue && rhs.m_hasValue) && (value() == rhs.value()));
}

template <typename T>
constexpr inline bool optional<T>::operator==(const nullopt_t&) const noexcept
{
    return !m_hasValue;
}

template <typename T>
constexpr inline bool optional<T>::operator!=(const optional<T>& rhs) const noexcept
{
    return !(*this == rhs);
}

template <typename T>
constexpr inline bool optional<T>::operator!=(const nullopt_t& rhs) const noexcept
{
    return !(*this == rhs);
}

template <typename T>
inline const T* optional<T>::operator->() const noexcept
{
    return &value();
}

template <typename T>
inline const T& optional<T>::operator*() const noexcept
{
    return value();
}

template <typename T>
inline T* optional<T>::operator->() noexcept
{
    return &value();
}

template <typename T>
inline T& optional<T>::operator*() noexcept
{
    return value();
}

template <typename T>
inline constexpr optional<T>::operator bool() const noexcept
{
    return m_hasValue;
}

template <typename T>
template <typename... Targs>
inline T& optional<T>::emplace(Targs&&... args) noexcept
{
    if (m_hasValue)
    {
        destruct_value();
    }

    construct_value(std::forward<Targs>(args)...);
    return value();
}

template <typename T>
inline constexpr bool optional<T>::has_value() const noexcept
{
    return m_hasValue;
}

template <typename T>
inline void optional<T>::reset() noexcept
{
    if (m_hasValue)
    {
        destruct_value();
    }
}

template <typename T>
inline T& optional<T>::value() & noexcept
{
    Expects(has_value());
    return *static_cast<T*>(static_cast<void*>(m_data));
}

template <typename T>
inline const T& optional<T>::value() const& noexcept
{
    // PRQA S 3066 1 # const cast to avoid code duplication
    return const_cast<optional<T>*>(this)->value();
}

template <typename T>
inline T&& optional<T>::value() && noexcept
{
    Expects(has_value());
    return std::move(*static_cast<T*>(static_cast<void*>(m_data)));
}

template <typename T>
inline const T&& optional<T>::value() const&& noexcept
{
    return std::move(*const_cast<optional<T>*>(this)->value());
}


template <typename T>
template <typename U>
inline constexpr T optional<T>::value_or(U&& default_value) const noexcept
{
    return (m_hasValue) ? value() : std::forward<U>(default_value);
}

template <typename T>
template <typename... Targs>
inline void optional<T>::construct_value(Targs&&... args) noexcept
{
    new (static_cast<T*>(static_cast<void*>(m_data))) T(std::forward<Targs>(args)...);
    m_hasValue = true;
}

template <typename T>
inline void optional<T>::destruct_value() noexcept
{
    value().~T();
    m_hasValue = false;
}

template <typename OptionalBaseType, typename... Targs>
inline optional<OptionalBaseType> make_optional(Targs&&... args) noexcept
{
    optional<OptionalBaseType> returnValue = nullopt_t();
    returnValue.emplace(std::forward<Targs>(args)...);
    return returnValue;
}

template <typename T>
inline optional<T>& optional<T>::and_then(const cxx::function_ref<void(T&)>& callable) noexcept
{
    if (m_hasValue && callable)
    {
        callable(value());
    }
    return *this;
}

template <typename T>
inline const optional<T>& optional<T>::and_then(const cxx::function_ref<void(const T&)>& callable) const noexcept
{
    if (m_hasValue && callable)
    {
        callable(value());
    }
    return *this;
}

template <typename T>
inline optional<T>& optional<T>::or_else(const cxx::function_ref<void()>& callable) noexcept
{
    if (!m_hasValue && callable)
    {
        callable();
    }
    return *this;
}

template <typename T>
inline const optional<T>& optional<T>::or_else(const cxx::function_ref<void()>& callable) const noexcept
{
    if (!m_hasValue && callable)
    {
        callable();
    }
    return *this;
}


} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_OPTIONAL_INL
