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
#ifndef IOX_UTILS_CXX_VECTOR_INL
#define IOX_UTILS_CXX_VECTOR_INL

#include "iceoryx_utils/cxx/vector.hpp"

#include <iostream>

namespace iox
{
namespace cxx
{
template <typename T, uint64_t Capacity>
inline vector<T, Capacity>::vector(const uint64_t count, const T& value) noexcept
{
    if (count > Capacity)
    {
        std::cerr << "Attemting to initialize a vector of capacity " << Capacity << " with " << count
                  << " elements. This exceeds the capacity and only " << Capacity << " elements will be created!"
                  << std::endl;
    }

    for (uint64_t i = 0u; i < count && i < Capacity; ++i)
    {
        emplace_back(value);
    }
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>::vector(const uint64_t count) noexcept
{
    if (count > Capacity)
    {
        std::cerr << "Attemting to initialize a vector of capacity " << Capacity << " with " << count
                  << " elements. This exceeds the capacity and only " << Capacity << " elements will be created!"
                  << std::endl;
    }

    m_size = std::min(count, Capacity);
    for (uint64_t i = 0U; i < m_size; ++i)
    {
        new (&at(i)) T();
    }
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>::vector(const vector& rhs) noexcept
{
    *this = rhs;
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>::vector(vector&& rhs) noexcept
{
    *this = std::move(rhs);
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>::~vector() noexcept
{
    clear();
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>& vector<T, Capacity>::operator=(const vector& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0u;
        // copy using copy assignment
        for (; i < std::min(rhs.size(), size()); ++i)
        {
            at(i) = rhs.at(i);
        }

        // copy using copy ctor
        for (; i < rhs.size(); ++i)
        {
            emplace_back(rhs.at(i));
        }

        // delete remaining elements
        for (; i < size(); ++i)
        {
            at(i).~T();
        }

        m_size = rhs.m_size;
    }
    return *this;
}

template <typename T, uint64_t Capacity>
inline vector<T, Capacity>& vector<T, Capacity>::operator=(vector&& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0u;
        // move using move assignment
        for (; i < std::min(rhs.size(), size()); ++i)
        {
            at(i) = std::move(rhs.at(i));
        }

        // move using move ctor
        for (; i < rhs.size(); ++i)
        {
            emplace_back(std::move(rhs.at(i)));
        }

        // delete remaining elements
        for (; i < size(); ++i)
        {
            at(i).~T();
        }

        m_size = rhs.m_size;
        rhs.clear();
    }
    return *this;
}

template <typename T, uint64_t Capacity>
inline bool vector<T, Capacity>::empty() const noexcept
{
    return m_size == 0u;
}

template <typename T, uint64_t Capacity>
inline uint64_t vector<T, Capacity>::size() const noexcept
{
    return m_size;
}

template <typename T, uint64_t Capacity>
inline uint64_t vector<T, Capacity>::capacity() const noexcept
{
    return Capacity;
}

template <typename T, uint64_t Capacity>
inline void vector<T, Capacity>::clear() noexcept
{
    while (pop_back())
    {
    }
}

template <typename T, uint64_t Capacity>
template <typename... Targs>
inline bool vector<T, Capacity>::emplace_back(Targs&&... args) noexcept
{
    if (m_size < Capacity)
    {
        new (&at(m_size++)) T(std::forward<Targs>(args)...);
        return true;
    }
    return false;
}

template <typename T, uint64_t Capacity>
template <typename... Targs>
inline bool vector<T, Capacity>::emplace(const uint64_t position, Targs&&... args) noexcept
{
    if (m_size >= Capacity || position >= Capacity || position > m_size)
    {
        return false;
    }

    if (position == m_size)
    {
        return emplace_back(std::forward<Targs>(args)...);
    }

    emplace_back(std::move(*reinterpret_cast<T*>(m_data[m_size - 1U])));
    for (uint64_t i = m_size - 1U; i > position; --i)
    {
        reinterpret_cast<T*>(m_data)[i] = std::move(reinterpret_cast<T*>(m_data)[i - 1U]);
    }

    new (&at(position)) T(std::forward<Targs>(args)...);
    return true;
}

template <typename T, uint64_t Capacity>
inline bool vector<T, Capacity>::push_back(const T& value) noexcept
{
    return emplace_back(value);
}

template <typename T, uint64_t Capacity>
inline bool vector<T, Capacity>::push_back(T&& value) noexcept
{
    return emplace_back(std::forward<T>(value));
}

template <typename T, uint64_t Capacity>
inline bool vector<T, Capacity>::pop_back() noexcept
{
    if (m_size > 0U)
    {
        reinterpret_cast<const T*>(m_data)[--m_size].~T();
        return true;
    }
    return false;
}

template <typename T, uint64_t Capacity>
template <typename... Targs>
inline bool vector<T, Capacity>::resize(const uint64_t count, const Targs&... args) noexcept
{
    if (count > Capacity)
    {
        return false;
    }

    if (count < m_size)
    {
        while (count != m_size)
        {
            pop_back();
        }
    }
    else if (count > m_size)
    {
        while (count != m_size)
        {
            emplace_back(args...);
        }
    }
    return true;
}

template <typename T, uint64_t Capacity>
inline T* vector<T, Capacity>::data() noexcept
{
    return reinterpret_cast<T*>(m_data);
}

template <typename T, uint64_t Capacity>
inline const T* vector<T, Capacity>::data() const noexcept
{
    return reinterpret_cast<const T*>(m_data);
}

template <typename T, uint64_t Capacity>
inline T& vector<T, Capacity>::at(const uint64_t index) noexcept
{
    // PRQA S 3066 1 # const cast to avoid code duplication
    return const_cast<T&>(const_cast<const vector<T, Capacity>*>(this)->at(index));
}

template <typename T, uint64_t Capacity>
inline const T& vector<T, Capacity>::at(const uint64_t index) const noexcept
{
    cxx::Expects((index < m_size) && "Out of bounds access");
    return reinterpret_cast<const T*>(m_data)[index];
}

template <typename T, uint64_t Capacity>
inline T& vector<T, Capacity>::operator[](const uint64_t index) noexcept
{
    return at(index);
}

template <typename T, uint64_t Capacity>
inline const T& vector<T, Capacity>::operator[](const uint64_t index) const noexcept
{
    return at(index);
}

template <typename T, uint64_t Capacity>
inline T& vector<T, Capacity>::front() noexcept
{
    cxx::Expects(!empty() && "Attempting to access the front of an empty vector");
    return at(0);
}

template <typename T, uint64_t Capacity>
inline const T& vector<T, Capacity>::front() const noexcept
{
    return const_cast<vector<T, Capacity>*>(this)->front();
}

template <typename T, uint64_t Capacity>
inline T& vector<T, Capacity>::back() noexcept
{
    cxx::Expects(!empty() && "Attempting to access the back of an empty vector");
    return at(size() - 1u);
}

template <typename T, uint64_t Capacity>
inline const T& vector<T, Capacity>::back() const noexcept
{
    return const_cast<vector<T, Capacity>*>(this)->back();
}

template <typename T, uint64_t Capacity>
inline typename vector<T, Capacity>::iterator vector<T, Capacity>::begin() noexcept
{
    return reinterpret_cast<iterator>(&(m_data[0]));
}

template <typename T, uint64_t Capacity>
inline typename vector<T, Capacity>::const_iterator vector<T, Capacity>::begin() const noexcept
{
    return reinterpret_cast<const_iterator>(&(m_data[0]));
}

template <typename T, uint64_t Capacity>
inline typename vector<T, Capacity>::iterator vector<T, Capacity>::end() noexcept
{
    return reinterpret_cast<iterator>((&(m_data[0]) + m_size)[0]);
}

template <typename T, uint64_t Capacity>
inline typename vector<T, Capacity>::const_iterator vector<T, Capacity>::end() const noexcept
{
    return reinterpret_cast<const_iterator>((&(m_data[0]) + m_size)[0]);
}

template <typename T, uint64_t Capacity>
inline typename vector<T, Capacity>::iterator vector<T, Capacity>::erase(iterator position) noexcept
{
    if (begin() <= position && position < end())
    {
        uint64_t index = static_cast<uint64_t>(position - begin()) % (sizeof(element_t) * Capacity);
        size_t n = index;
        for (; n + 1u < size(); ++n)
        {
            at(n) = std::move(at(n + 1u));
        }
        at(n).~T();
        m_size--;
    }
    return nullptr;
}

} // namespace cxx
} // namespace iox

template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
inline bool operator==(const iox::cxx::vector<T, CapacityLeft>& lhs,
                       const iox::cxx::vector<T, CapacityRight>& rhs) noexcept
{
    uint64_t vectorSize = lhs.size();
    if (vectorSize != rhs.size())
    {
        return false;
    }

    for (uint64_t i = 0u; i < vectorSize; ++i)
    {
        if (lhs[i] != rhs[i])
        {
            return false;
        }
    }
    return true;
}

template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
inline bool operator!=(const iox::cxx::vector<T, CapacityLeft>& lhs,
                       const iox::cxx::vector<T, CapacityRight>& rhs) noexcept
{
    return !(lhs == rhs);
}


#endif // IOX_UTILS_CXX_VECTOR_INL
