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
#ifndef IOX_UTILS_CXX_EXPECTED_INL
#define IOX_UTILS_CXX_EXPECTED_INL

#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace cxx
{
namespace internal
{
template <typename T, typename>
struct HasInvalidStateMember : std::false_type
{
};
template <typename T>
struct HasInvalidStateMember<T, iox::cxx::void_t<decltype(T::INVALID_STATE)>> : std::true_type
{
};
template <typename... T>
struct IsOptional : std::false_type
{
};
template <typename T>
struct IsOptional<iox::cxx::optional<T>> : std::true_type
{
};
} // namespace internal
template <typename T>
inline T ErrorTypeAdapter<T>::getInvalidState() noexcept
{
    return T::INVALID_STATE;
}

template <typename T>
inline success<T>::success(const T& t) noexcept
    : value(t)
{
}

template <typename T>
inline success<T>::success(T&& t) noexcept
    : value(std::forward<T>(t))
{
}

template <typename T>
template <typename... Targs>
inline success<T>::success(Targs&&... args) noexcept
    : value(std::forward<Targs>(args)...)
{
}

template <typename T>
inline error<T>::error(const T& t) noexcept
    : value(t)
{
}

template <typename T>
inline error<T>::error(T&& t) noexcept
    : value(std::forward<T>(t))
{
}

template <typename T>
template <typename... Targs>
inline error<T>::error(Targs&&... args) noexcept
    : value(std::forward<Targs>(args)...)
{
}


template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(variant<ValueType, ErrorType>&& f_store, const bool hasError) noexcept
    : m_store(std::move(f_store))
    , m_hasError(hasError)
{
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(const success<ValueType>& successValue) noexcept
    : m_store(in_place_index<VALUE_INDEX>(), successValue.value)
    , m_hasError(false)
{
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(success<ValueType>&& successValue) noexcept
    : m_store(in_place_index<VALUE_INDEX>(), std::move(successValue.value))
    , m_hasError(false)
{
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(const error<ErrorType>& errorValue) noexcept
    : m_store(in_place_index<ERROR_INDEX>(), errorValue.value)
    , m_hasError(true)
{
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(error<ErrorType>&& errorValue) noexcept
    : m_store(in_place_index<ERROR_INDEX>(), std::move(errorValue.value))
    , m_hasError(true)
{
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::expected(expected<ValueType, ErrorType>&& rhs) noexcept
{
    *this = std::move(rhs);
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::operator=(expected<ValueType, ErrorType>&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_store = std::move(rhs.m_store);
        m_hasError = std::move(rhs.m_hasError);
        rhs.m_store.template emplace_at_index<ERROR_INDEX>(ErrorTypeAdapter<ErrorType>::getInvalidState());
        rhs.m_hasError = true;
    }
    return *this;
}

template <typename ValueType, typename ErrorType>
template <typename... Targs>
inline expected<ValueType, ErrorType> expected<ValueType, ErrorType>::create_value(Targs&&... args) noexcept
{
    expected<ValueType, ErrorType> returnValue(
        variant<ValueType, ErrorType>(in_place_index<VALUE_INDEX>(), std::forward<Targs>(args)...), false);

    return returnValue;
}

template <typename ValueType, typename ErrorType>
template <typename... Targs>
inline expected<ValueType, ErrorType> expected<ValueType, ErrorType>::create_error(Targs&&... args) noexcept
{
    expected<ValueType, ErrorType> returnValue(
        variant<ValueType, ErrorType>(in_place_index<ERROR_INDEX>(), std::forward<Targs>(args)...), true);

    return returnValue;
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>::operator bool() const noexcept
{
    return !m_hasError;
}

template <typename ValueType, typename ErrorType>
inline bool expected<ValueType, ErrorType>::has_error() const noexcept
{
    return m_hasError;
}

template <typename ValueType, typename ErrorType>
inline ErrorType&& expected<ValueType, ErrorType>::get_error() && noexcept
{
    return std::move(*m_store.template get_at_index<ERROR_INDEX>());
}

template <typename ValueType, typename ErrorType>
inline ErrorType& expected<ValueType, ErrorType>::get_error() & noexcept
{
    return *m_store.template get_at_index<ERROR_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline ValueType&& expected<ValueType, ErrorType>::value() && noexcept
{
    return std::move(*m_store.template get_at_index<VALUE_INDEX>());
}

template <typename ValueType, typename ErrorType>
inline const ValueType& expected<ValueType, ErrorType>::value() const& noexcept
{
    return *m_store.template get_at_index<VALUE_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline ValueType& expected<ValueType, ErrorType>::value() & noexcept
{
    return *m_store.template get_at_index<VALUE_INDEX>();
}

template <typename ErrorType>
inline const ErrorType& expected<ErrorType>::get_error() const& noexcept
{
    return *m_store.template get_at_index<ERROR_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline ValueType expected<ValueType, ErrorType>::value_or(const ValueType& value) noexcept
{
    if (has_error())
    {
        return value;
    }

    return *m_store.template get_at_index<VALUE_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline ValueType expected<ValueType, ErrorType>::value_or(const ValueType& value) const noexcept
{
    return const_cast<expected*>(this)->value_or(value);
}

template <typename ValueType, typename ErrorType>
inline ValueType* expected<ValueType, ErrorType>::operator->() noexcept
{
    return m_store.template get_at_index<VALUE_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline const ValueType* expected<ValueType, ErrorType>::operator->() const noexcept
{
    return const_cast<expected*>(this)->operator->();
}

template <typename ValueType, typename ErrorType>
inline ValueType& expected<ValueType, ErrorType>::operator*() noexcept
{
    return *m_store.template get_at_index<VALUE_INDEX>();
}

template <typename ValueType, typename ErrorType>
inline const ValueType& expected<ValueType, ErrorType>::operator*() const noexcept
{
    return const_cast<expected*>(this)->operator*();
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::or_else(const cxx::function_ref<void(ErrorType&)>& callable) noexcept
{
    if (has_error() && callable)
    {
        callable(get_error());
    }

    return *this;
}

template <typename ValueType, typename ErrorType>
inline const expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::or_else(const cxx::function_ref<void(ErrorType&)>& callable) const noexcept
{
    return const_cast<expected*>(this)->or_else(callable);
}

template <typename ValueType, typename ErrorType>
inline const expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::and_then(const cxx::function_ref<void(ValueType&)>& callable) const noexcept
{
    return const_cast<expected*>(this)->and_then(callable);
}

template <typename ValueType, typename ErrorType>
inline expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::and_then(const cxx::function_ref<void(ValueType&)>& callable) noexcept
{
    if (!has_error() && callable)
    {
        callable(value());
    }

    return *this;
}

template <typename ValueType, typename ErrorType>
template <typename Optional, typename std::enable_if<internal::IsOptional<Optional>::value, int>::type>
inline const expected<ValueType, ErrorType>& expected<ValueType, ErrorType>::and_then(
    const cxx::function_ref<void(typename Optional::type&)>& callable) const noexcept
{
    return const_cast<expected*>(this)->and_then(callable);
}

template <typename ValueType, typename ErrorType>
template <typename Optional, typename std::enable_if<internal::IsOptional<Optional>::value, int>::type>
inline expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::and_then(const cxx::function_ref<void(typename Optional::type&)>& callable) noexcept
{
    if (!has_error() && callable)
    {
        auto& optional = value();
        if (optional.has_value())
        {
            callable(optional.value());
        }
    }

    return *this;
}

template <typename ValueType, typename ErrorType>
template <typename Optional, typename std::enable_if<internal::IsOptional<Optional>::value, int>::type>
inline expected<ValueType, ErrorType>&
expected<ValueType, ErrorType>::if_empty(const cxx::function_ref<void(void)>& callable) noexcept
{
    if (!has_error() && callable)
    {
        auto& optional = value();
        if (!optional.has_value())
        {
            callable();
        }
    }

    return *this;
}

template <typename ValueType, typename ErrorType>
template <typename T>
inline expected<ValueType, ErrorType>::operator expected<T>() noexcept
{
    if (has_error())
    {
        return error<ErrorType>(get_error());
    }
    return success<>();
}

template <typename ValueType, typename ErrorType>
template <typename T>
inline expected<ValueType, ErrorType>::operator expected<T>() const noexcept
{
    return const_cast<expected*>(this)->operator expected<T>();
}

template <typename ValueType, typename ErrorType>
inline optional<ValueType> expected<ValueType, ErrorType>::to_optional() const noexcept
{
    optional<ValueType> returnValue;
    if (!has_error())
    {
        returnValue.emplace(value());
    }
    return returnValue;
}


// expected<ErrorType>

template <typename ErrorType>
inline expected<ErrorType>::expected(variant<ErrorType>&& f_store, const bool hasError) noexcept
    : m_store(std::move(f_store))
    , m_hasError(hasError)
{
}

template <typename ErrorType>
inline expected<ErrorType>::expected(const success<void>&) noexcept
    : m_hasError(false)
{
}

template <typename ErrorType>
inline expected<ErrorType>::expected(expected<ErrorType>&& rhs) noexcept
{
    *this = std::move(rhs);
}

template <typename ErrorType>
inline expected<ErrorType>& expected<ErrorType>::operator=(expected<ErrorType>&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_store = std::move(rhs.m_store);
        m_hasError = rhs.m_hasError;
        rhs.m_store = ErrorTypeAdapter<ErrorType>::getInvalidState();
        rhs.m_hasError = true;
    }
    return *this;
}

template <typename ErrorType>
inline expected<ErrorType>::expected(const error<ErrorType>& errorValue) noexcept
    : m_store(in_place_index<ERROR_INDEX>(), errorValue.value)
    , m_hasError(true)
{
}

template <typename ErrorType>
inline expected<ErrorType>::expected(error<ErrorType>&& errorValue) noexcept
    : m_store(in_place_index<ERROR_INDEX>(), std::move(errorValue.value))
    , m_hasError(true)
{
}

#if defined(_WIN32)
template <typename ErrorType>
template <typename ValueType>
inline expected<ErrorType>::expected(const expected<ValueType, ErrorType>& rhs) noexcept
{
    m_hasError = rhs.has_error();
    if (m_hasError)
    {
        m_store.emplace_at_index<ERROR_INDEX>(rhs.get_error());
    }
}

template <typename ErrorType>
template <typename ValueType>
inline expected<ErrorType>::expected(expected<ValueType, ErrorType>&& rhs) noexcept
{
    m_hasError = rhs.has_error();
    if (m_hasError)
    {
        m_store.emplace_at_index<ERROR_INDEX>(std::move(rhs.get_error()));
    }
}

template <typename ErrorType>
template <typename ValueType>
inline expected<ErrorType>& expected<ErrorType>::operator=(const expected<ValueType, ErrorType>& rhs) noexcept
{
    if (m_hasError && rhs.has_error())
    {
        m_store.get_error() = rhs.get_error();
    }
    else if (rhs.has_error())
    {
        m_store = variant<ErrorType>(in_place_type<ErrorType>(), rhs.get_error());
    }
    m_hasError = rhs.has_error();
}

template <typename ErrorType>
template <typename ValueType>
inline expected<ErrorType>& expected<ErrorType>::operator=(expected<ValueType, ErrorType>&& rhs) noexcept
{
    if (m_hasError && rhs.has_error())
    {
        m_store.get_error() = std::move(rhs.get_error());
    }
    else if (rhs.has_error())
    {
        m_store = variant<ErrorType>(in_place_type<ErrorType>(), std::move(rhs.get_error()));
    }
    m_hasError = rhs.has_error();
}
#endif

template <typename ErrorType>
inline expected<ErrorType> expected<ErrorType>::create_value() noexcept
{
    expected<ErrorType> returnValue(variant<ErrorType>(), false);

    return returnValue;
}

template <typename ErrorType>
template <typename... Targs>
inline expected<ErrorType> expected<ErrorType>::create_error(Targs&&... args) noexcept
{
    expected<ErrorType> returnValue(variant<ErrorType>(in_place_index<ERROR_INDEX>(), std::forward<Targs>(args)...),
                                    true);

    return returnValue;
}

template <typename ErrorType>
inline expected<ErrorType>::operator bool() const noexcept
{
    return !m_hasError;
}

template <typename ErrorType>
inline bool expected<ErrorType>::has_error() const noexcept
{
    return m_hasError;
}

template <typename ErrorType>
inline ErrorType&& expected<ErrorType>::get_error() && noexcept
{
    return std::move(*m_store.template get_at_index<ERROR_INDEX>());
}

template <typename ValueType, typename ErrorType>
inline const ErrorType& expected<ValueType, ErrorType>::get_error() const& noexcept
{
    return *m_store.template get_at_index<ERROR_INDEX>();
}

template <typename ErrorType>
inline ErrorType& expected<ErrorType>::get_error() & noexcept
{
    return *m_store.template get_at_index<ERROR_INDEX>();
}

template <typename ErrorType>
inline expected<ErrorType>& expected<ErrorType>::or_else(const cxx::function_ref<void(ErrorType&)>& callable) noexcept
{
    if (has_error() && callable)
    {
        callable(get_error());
    }

    return *this;
}

template <typename ErrorType>
inline const expected<ErrorType>&
expected<ErrorType>::or_else(const cxx::function_ref<void(ErrorType&)>& callable) const noexcept
{
    return const_cast<expected*>(this)->or_else(callable);
}

template <typename ErrorType>
inline expected<ErrorType>& expected<ErrorType>::and_then(const cxx::function_ref<void()>& callable) noexcept
{
    if (!has_error() && callable)
    {
        callable();
    }

    return *this;
}

template <typename ErrorType>
inline const expected<ErrorType>&
expected<ErrorType>::and_then(const cxx::function_ref<void()>& callable) const noexcept
{
    return const_cast<expected*>(this)->and_then(callable);
}
} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_EXPECTED_INL
