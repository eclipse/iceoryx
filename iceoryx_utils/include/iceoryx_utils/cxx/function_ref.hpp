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

#ifndef IOX_UTILS_CXX_FUNCTION_REF_HPP
#define IOX_UTILS_CXX_FUNCTION_REF_HPP

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/type_traits.hpp"

#include <cstddef>
#include <iostream>
#include <memory>

namespace iox
{
namespace cxx
{
template <typename SignatureType>
class function_ref;

template <typename...>
struct is_function_ref : std::false_type
{
};

template <typename... Targs>
struct is_function_ref<function_ref<Targs...>> : std::true_type
{
};

/// @brief cxx::function_ref is a non-owning reference to a callable.
///
///        It has these features:
///         * No heap usage
///         * No exceptions
///         * Stateful lambda support
///         * C++11/14 support
///
/// @attention Invoking an empty function_ref can lead to a program termination!
///
/// @code
///         // Usage as function parameter
///         void fuu(cxx::function_ref<void()> callback)
///         {
///             callback();
///         }
///         // Call the lambda
///         fuu([]{ doSomething(); });
///
///         // Usage with l-values
///         // Pitfall: Ensure that lifetime of callable suits the point in time of calling callback()
///         auto callable = [&]{ doSomething(); };
///         cxx::function_ref<void()> callback(callable);
///         // Call the callback
///         callback();
/// @endcode
template <class ReturnType, class... ArgTypes>
class function_ref<ReturnType(ArgTypes...)>
{
    using SignatureType = ReturnType(ArgTypes...);

  public:
    /// @brief Creates an empty function_ref in an invalid state
    /// @note Handle with care, program will terminate when calling an invalid function_ref
    function_ref() noexcept;

    ~function_ref() noexcept = default;

    function_ref(const function_ref&) noexcept = default;

    function_ref& operator=(const function_ref&) noexcept = default;

    /// @brief Creates a function_ref with a callable whose lifetime has to be longer than function_ref
    /// @param[in] callable that is not a function_ref
    template <typename CallableType,
              typename = std::enable_if_t<!is_function_ref<std::remove_reference_t<CallableType>>::value>,
              typename = std::enable_if_t<is_invocable<CallableType, ArgTypes...>::value>>
    function_ref(CallableType&& callable) noexcept;

    function_ref(function_ref&& rhs) noexcept;

    function_ref& operator=(function_ref&& rhs) noexcept;

    /// @brief Calls the provided callable
    /// @param[in] Arguments are forwarded to the underlying function pointer
    /// @return Returns the data type of the underlying function pointer
    /// @attention Invoking an empty function_ref can lead to a program termination!
    ReturnType operator()(ArgTypes... args) const noexcept;

    /// @brief Checks whether a valid target is contained
    /// @return True if valid target is contained, otherwise false
    explicit operator bool() const noexcept;

    /// @brief Swaps the contents of two function_ref's
    /// @param[in] Reference to another function_ref
    void swap(function_ref& rhs) noexcept;

  private:
    void* m_pointerToCallable{nullptr};
    ReturnType (*m_functionPointer)(void*, ArgTypes...){nullptr};
};

} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/function_ref.inl"

#endif
