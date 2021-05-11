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
#ifndef IOX_UTILS_CXX_STACK_INL
#define IOX_UTILS_CXX_STACK_INL

namespace iox
{
namespace cxx
{
template <typename T, uint64_t Capacity>
inline cxx::optional<T> stack<T, Capacity>::pop() noexcept
{
    if (m_size == 0U)
    {
        return cxx::nullopt;
    }

    return *reinterpret_cast<T*>(m_data[--m_size]);
}

template <typename T, uint64_t Capacity>
template <typename... Targs>
inline bool stack<T, Capacity>::push(Targs&&... args) noexcept
{
    if (m_size >= Capacity)
    {
        return false;
    }

    new (reinterpret_cast<T*>(m_data[m_size++])) T(std::forward<Targs>(args)...);
    return true;
}

template <typename T, uint64_t Capacity>
inline uint64_t stack<T, Capacity>::size() const noexcept
{
    return m_size;
}

template <typename T, uint64_t Capacity>
inline constexpr uint64_t stack<T, Capacity>::capacity() noexcept
{
    return Capacity;
}


} // namespace cxx
} // namespace iox

#endif

