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
#ifndef IOX_UTILS_CXX_VECTOR_HPP
#define IOX_UTILS_CXX_VECTOR_HPP

#include "iceoryx_utils/cxx/helplets.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace iox
{
namespace cxx
{
/// @brief  C++11 compatible vector implementation. We needed to do some
///         adjustments in the API since we do not use exceptions and we require
///         a data structure which can be located fully in the shared memory.
///
/// @attention Out of bounds access or accessing an empty vector can lead to a program termination!
///
template <typename T, uint64_t Capacity>
class vector
{
  public:
    using value_type = T;

    using iterator = T*;
    using const_iterator = const T*;

    /// @brief creates an empty vector
    vector() noexcept = default;

    /// @brief creates a vector with count copies of elements with value value
    /// @param [in] count is the number copies which are inserted into the vector
    /// @param [in] value is the value which is inserted into the vector
    vector(const uint64_t count, const T& value) noexcept;

    /// @brief creates a vector with count copies of elements constructed with the default constructor of T
    /// @param [in] count is the number copies which are inserted into the vector
    vector(const uint64_t count) noexcept;

    /// @brief copy constructor to copy a vector of the same capacity
    vector(const vector& rhs) noexcept;

    /// @brief move constructor to move a vector of the same capacity
    vector(vector&& rhs) noexcept;

    /// @brief destructs the vector and also calls the destructor of all
    ///         contained elements
    ~vector() noexcept;

    /// @brief copy assignment. if the destination vector contains more
    ///         elements than the source the remaining elements will be
    ///         destructed
    vector& operator=(const vector& rhs) noexcept;

    /// @brief move assignment. if the destination vector contains more
    ///         elements than the source the remaining elements will be
    ///         destructed
    vector& operator=(vector&& rhs) noexcept;

    /// @brief returns an iterator to the first element of the vector,
    ///         if the vector is empty it returns the same iterator as
    ///         end (the first iterator which is outside of the vector)
    iterator begin() noexcept;

    /// @brief returns a const iterator to the first element of the vector,
    ///         if the vector is empty it returns the same iterator as
    ///         end (the first iterator which is outside of the vector)
    const_iterator begin() const noexcept;

    /// @brief returns an iterator to the element which comes after the last
    ///         element (the first element which is outside of the vector)
    iterator end() noexcept;

    /// @brief returns a const iterator to the element which comes after the last
    ///         element (the first element which is outside of the vector)
    const_iterator end() const noexcept;

    /// @brief return the pointer to the underlying array
    /// @return pointer to underlying array
    T* data() noexcept;

    /// @brief return the const pointer to the underlying array
    /// @return const pointer to underlying array
    const T* data() const noexcept;

    /// @brief returns a reference to the element stored at index. the behavior
    //          is undefined if the element at index does not exist.
    /// @attention Out of bounds access can lead to a program termination!
    T& at(const uint64_t index) noexcept;

    /// @brief returns a const reference to the element stored at index. the
    ///         behavior is undefined if the element at index does not exist.
    /// @attention Out of bounds access can lead to a program termination!
    const T& at(const uint64_t index) const noexcept;

    /// @brief returns a reference to the element stored at index. the behavior
    //          is undefined if the element at index does not exist.
    /// @attention Out of bounds access can lead to a program termination!
    T& operator[](const uint64_t index) noexcept;

    /// @brief returns a const reference to the element stored at index. the
    ///         behavior is undefined if the element at index does not exist.
    /// @attention Out of bounds access can lead to a program termination!
    const T& operator[](const uint64_t index) const noexcept;

    /// @brief returns a reference to the first element; terminates if the vector is empty
    /// @return reference to the first element
    /// @attention Accessing an empty vector can lead to a program termination!
    T& front() noexcept;

    /// @brief returns a const reference to the first element; terminates if the vector is empty
    /// @return const reference to the first element
    /// @attention Accessing an empty vector can lead to a program termination!
    const T& front() const noexcept;

    /// @brief returns a reference to the last element; terminates if the vector is empty
    /// @return reference to the last element
    /// @attention Accessing an empty vector can lead to a program termination!
    T& back() noexcept;

    /// @brief returns a const reference to the last element; terminates if the vector is empty
    /// @return const reference to the last element
    /// @attention Accessing an empty vector can lead to a program termination!
    const T& back() const noexcept;

    /// @brief returns the capacity of the vector which was given via the template
    ///         argument
    uint64_t capacity() const noexcept;

    /// @brief returns the number of elements which are currently stored in the
    ///         vector
    uint64_t size() const noexcept;

    /// @brief returns true if the vector is emtpy, otherwise false
    bool empty() const noexcept;

    /// @brief calls the destructor of all contained elements and removes them
    void clear() noexcept;

    /// @brief resizes the vector. If the vector size increases new elements
    /// will be constructed with the given arguments. If count is greater than the capacity
    /// the vector will stay unchanged.
    /// @param[in] count new size of the vector
    /// @param[in] args arguments which are used by the constructor of newly created elements
    /// @return true if the resize was successful, false if count is greater than the capacity.
    /// @note perfect forwarded arguments are explicitly not wanted here. think of what happens if resize
    ///       creates two new elements via move construction. The first one has a valid source but the second
    ///       gets an already moved parameter.
    template <typename... Targs>
    bool resize(const uint64_t count, const Targs&... args) noexcept;

    /// @brief forwards all arguments to the constructor of the contained element
    ///         and performs a placement new at the provided position
    /// @param[in] position the position where the element should be created
    template <typename... Targs>
    bool emplace(const uint64_t position, Targs&&... args) noexcept;

    /// @brief forwards all arguments to the constructor of the contained element
    ///         and performs a placement new at the end
    template <typename... Targs>
    bool emplace_back(Targs&&... args) noexcept;

    /// @brief appends the given element at the end of the vector
    /// @return true if successful, false if vector already full
    bool push_back(const T& value) noexcept;

    /// @brief appends the given element at the end of the vector
    /// @return true if successful, false if vector already full
    bool push_back(T&& value) noexcept;

    /// @brief removes the last element of the vector; calling pop_back on an empty container does nothing
    /// @return true if the last element was removed. If the vector is empty it returns false.
    bool pop_back() noexcept;

    /// @brief removes an element at the given position. if this element is in
    ///         the middle of the vector every element is moved one place to the
    ///         left to ensure that the elements are stored contiguously
    iterator erase(iterator position) noexcept;

  private:
    using element_t = uint8_t[sizeof(T)];
    alignas(T) element_t m_data[Capacity];
    uint64_t m_size = 0u;
};
} // namespace cxx
} // namespace iox

template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
bool operator==(const iox::cxx::vector<T, CapacityLeft>& lhs, const iox::cxx::vector<T, CapacityRight>& rhs) noexcept;

template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
bool operator!=(const iox::cxx::vector<T, CapacityLeft>& lhs, const iox::cxx::vector<T, CapacityRight>& rhs) noexcept;

#include "iceoryx_utils/internal/cxx/vector.inl"

#endif // IOX_UTILS_CXX_VECTOR_HPP
