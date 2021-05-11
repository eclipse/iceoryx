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

#ifndef IOX_UTILS_CXX_LIST_HPP
#define IOX_UTILS_CXX_LIST_HPP

#include "iceoryx_utils/cxx/helplets.hpp"

#include <cstdint>
#include <iostream>

#include "iceoryx_utils/platform/platform_correction.hpp"

namespace iox
{
namespace cxx
{
/// @brief  C++11 compatible bi-directional list implementation.
/// @details Adjustments in the API were done to not use exceptions and serve the requirement of
///         a data structure movable over shared memory.
///         attempt to add elements to a full list will be ignored.
///         Capacity must at least be 1, (unintended) negative initialization is rejected with compile assertion
///         limitation: concurrency concerns have to be handled by client side.
///
///         overview of cxx::forward_list deviations to std::forward_list(C++11)
///         - list declaration with mandatory max list size argument
///         - member functions don't throw exception but will trigger different failure handling
///         - push_front/~_back returns a bool (instead of void) informing on successful insertion (true)
///         - pop_front/~_back returns a bool (instead of void) informing on successful removal (true), otherwise empty
///         (false)
///         - emplace_front/~_back returns a reference to the inserted element (instead of void), this is C++17-conform
///         - remove / remove_if returns a the number of removed elements (instead of void), this is C++20-conform
///
///         (yet) missing implementations
///         -------------------------------
///         - allocator, difference_type / range operations
///         - assign, resize, swap, merge, splice_after, reverse, rbegin/crbegin, rend/crend, unique, sort
///         - list operator==, operator!=, operator<, operator<=, operator>, operator>=
///
///
/// @param T type user data to be managed within list
/// @param Capacity number of maximum list elements a client can push to the list. minimum value is '1'
template <typename T, uint64_t Capacity>
class list
{
  private:
    // forward declarations, private
    struct ListLink;
    template <bool>
    class IteratorBase;

  public:
    using iterator = IteratorBase<false>;
    using const_iterator = IteratorBase<true>;
    using value_type = T;
    using size_type = decltype(Capacity);

    /// @brief constructor for an empty list (of T-types elements)
    list() noexcept;

    /// @brief destructs the list and also calls the destructor of all
    ///         contained elements
    ~list();

    /// @brief copy constructor list including elements
    /// @param[in] rhs is the list to copy from (same capacity)
    list(const list& rhs) noexcept;

    /// @brief move constructor list including elements
    /// @param[in] rhs is the list to move-construct elements from (same capacity)
    list(list&& rhs) noexcept;

    /// @brief copy assignment, each element is copied (added) to the constructed list
    ///         any existing elements in 'this'/lhs are removed (same behaviour as std::list : Assigns new contents to
    ///         the container, replacing its current contents, and modifying its size accordingly.)
    /// @param[in] rhs is the list to copy from (same capacity)
    /// @return reference to created list
    list& operator=(const list& rhs) noexcept;

    /// @brief move assignment, list is cleared and initialized, elements are moved from source list
    ///         any existing elements in 'this'/lhs are removed (same behaviour as std::list : Assigns new contents to
    ///         the container, replacing its current contents, and modifying its size accordingly.)
    /// @param[in] rhs is the list to move from ('source', same capacity)
    /// @return reference to created list
    list& operator=(list&& rhs) noexcept;


    /// @brief default list operation to retrieve an interator to first list element
    /// @return iterator to first list element, returns iterator to end() when list is empty
    iterator begin() noexcept;

    /// @brief default list operation to retrieve an const_iterator to first list element
    /// @return iterator to first list element, returns iterator to end() when list is empty
    const_iterator begin() const noexcept;

    /// @brief default list operation to retrieve an const_iterator to first list element
    /// @return iterator to first list element, returns iterator to end() when list is empty
    const_iterator cbegin() const noexcept;

    /// @brief default list operation to retrieve an interator to end of list (behind last valid element)
    ///         Terminated when content is attemted to read (operator*, operator->)
    /// @return iterator to end element, does not contain data.
    iterator end() noexcept;

    /// @brief default list operation to retrieve an const_iterator to end of list (behind last valid element)
    ///         Terminated when content is attemted to read (operator*, operator->)
    /// @return iterator to end element, does not contain data.
    const_iterator end() const noexcept;

    /// @brief default list operation to retrieve an const_iterator to end of list (behind last valid element)
    ///         Terminated when content is attemted to read (operator*, operator->)
    /// @return iterator to end element, does not contain data.
    const_iterator cend() const noexcept;

    /// @brief list meta information on filling
    /// @return no elements in list (true), otherwise (false)
    bool empty() const noexcept;

    /// @brief list meta information on filling
    /// @return whether list is full (filled with 'capacity' / 'max_size' elements) (true), otherwise (false)
    bool full() const noexcept;

    /// @brief list meta information on filling
    /// @return current number of elements in list
    /// @min    returns min 0
    /// @max    returns max capacity
    size_type size() const noexcept;

    /// @brief list meta information, maximum number of elements the list can contain
    /// @return list has been initialized with the following number of elements.
    size_type capacity() const noexcept;

    /// @brief list meta information, maximum number of elements the list can contain
    /// @return list has been initialized with the following number of elements, same as capacity()
    size_type max_size() const noexcept;

    /// @brief Returns a reference to the first element in the container.
    ///         calling front() on an empty list will terminate() the processing
    /// @return reference to the first element
    T& front() noexcept;

    /// @brief Returns a reference to the first element in the container.
    ///         calling front() on an empty list will terminate() the processing
    /// @return const reference to the first element
    const T& front() const noexcept;

    /// @brief Returns a reference to the last element in the container.
    ///         calling back() on an empty list will terminate() the processing
    /// @return reference to the last element
    T& back() noexcept;

    /// @brief Returns a reference to the last element in the container.
    ///         calling back() on an empty list will terminate() the processing
    /// @return const reference to the last element
    const T& back() const noexcept;

    /// @brief add element to the beginning of the list
    /// @param[in] data reference to data element
    /// @return successful insertion (true), otherwise no element could be added to list (e.g. full -> false)
    bool push_front(const T& data) noexcept;

    /// @brief add element to the beginning of the list via move
    /// @param[in] data universal reference perfectly forwarded to client class
    /// @return successful insertion (true), otherwise no element could be added to list (e.g. full -> false)
    bool push_front(T&& data) noexcept;

    /// @brief add element to the end of the list
    /// @param[in] data reference to data element
    /// @return successful insertion (true), otherwise no element could be added to list (e.g. full -> false)
    bool push_back(const T& data) noexcept;

    /// @brief add element to the end of the list via move
    /// @param[in] data universal reference perfectly forwarded to client class
    /// @return successful insertion (true), otherwise no element could be added to list (e.g. full -> false)
    bool push_back(T&& data) noexcept;

    /// @brief remove the first element from the begining of the list
    ///         element destructor will be invoked
    /// @return successful removal (true), otherwise no element could be taken from list (e.g. empty -> false)
    bool pop_front() noexcept;

    /// @brief remove the last element from the end of the list
    ///         element destructor will be invoked
    /// @return successful removal (true), otherwise no element could be taken from list (e.g. empty -> false)
    bool pop_back() noexcept;

    /// @brief remove all elements from the list, list will be empty
    ///         element destructors will be invoked
    void clear() noexcept;

    /// @brief remove next element from linked iterator position
    ///         element destructors will be invoked
    ///         recursive calls to erase_after only delete each 2nd element
    /// @param[in] iter iterator linking the to-be-removed element
    /// @return an (non-const_) iterator to the element after the removed element,
    ///         returns end() element when reached end of list
    iterator erase(const_iterator iter) noexcept;

    /// @brief remove all elements which matches the given comparing element (compare by value)
    ///         requires a the template type T to have operator== defined.
    /// @param[in] data value to compare to
    /// @return the number of elements removed, return is C++20-conform
    size_type remove(const T& data) noexcept;

    /// @brief remove all elements which matches the provided comparison function
    ///         requires a the template type T to have a operator== defined.
    /// @param[in] pred unary predicate which returns true if the element should be removed
    /// @return the number of elements removed, return is C++20-conform
    template <typename UnaryPredicate>
    size_type remove_if(UnaryPredicate pred) noexcept;

    /// @brief construct element inplace at begining of list
    /// @param[in] args T-typed construction parameters (initializer list)
    /// @return referene to generated element, return is C++17-conform
    template <typename... ConstructorArgs>
    T& emplace_front(ConstructorArgs&&... args) noexcept;

    /// @brief construct element inplace at end of list
    /// @param[in] args T-typed construction parameters (initializer list)
    /// @return referene to generated element, return is C++17-conform
    template <typename... ConstructorArgs>
    T& emplace_back(ConstructorArgs&&... args) noexcept;

    /// @brief construct element inplace at iterator position
    /// @param[in] args T-typed construction parameters (initializer list)
    /// @param[in] iter position in list to (construct)insert after
    /// @return iterator to the newly added element
    template <typename... ConstructorArgs>
    iterator emplace(const_iterator iter, ConstructorArgs&&... args) noexcept;

    /// @brief insert element before iterator position
    /// @param[in] citer iterator with the position to insert after
    /// @param[in] data reference to element to add
    /// @return iterator to the newly added element
    iterator insert(const_iterator citer, const T& data) noexcept;

    /// @brief add element before the pointed-to element via move
    /// @param[in] citer iterator with the position to insert after
    /// @param[in] data universal reference perfectly forwarded to client class
    /// @return iterator to the newly added element
    iterator insert(const_iterator citer, T&& data) noexcept;

  private:
    /// @brief nested iterator class for list element operations including element access
    ///         comparison of iterator from different list is rejected by terminate()
    template <bool IsConstIterator = true>
    class IteratorBase
    {
      public:
        // provide the following public types for a std::iterator compatible iterator_category interface
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename std::conditional<IsConstIterator, const T, T>::type;
        using difference_type = void;
        using pointer = typename std::conditional<IsConstIterator, const T*, T*>::type;
        using reference = typename std::conditional<IsConstIterator, const T&, T&>::type;


        /// @brief construct a const_iterator from an iterator
        /// @param[in] iter is the iterator which will deliver list and index info for the const_iterator
        IteratorBase(const IteratorBase<false>& iter) noexcept;

        /// @brief assigns a const_iterator from an iterator; needs to be implemented because the copy c'tor is also
        /// explicitly implemented
        /// @param[in] rhs is the iterator which will deliver list and index info for the const_iterator
        /// @return reference to this iterator object
        IteratorBase& operator=(const IteratorBase<false>& rhs) noexcept;

        /// @brief prefix increment iterator, so it points to the next list element
        ///         when trying to increment beyond the end of the list, iterator stays pointing at the end
        /// @return reference to this iterator object
        IteratorBase& operator++() noexcept;

        /// @brief prefix decrement iterator, so it points to the previous list element
        ///         decrementing an iterator pointing already towards begin() has no effect (iterator stays at begin())
        /// @return reference to this iterator object
        IteratorBase& operator--() noexcept;


        /// @brief comparing list iterators for equality
        ///         the referenced list position is compared, not the content of the list element (T-typed)
        ///         -> there is no content for fictional elements at BEGIN_END_LINK_INDEX
        ///         only iterators of the same parent list can be compared; in case of misuse, terminate() is invoked
        /// @param[in] rhs is the 2nd iterator to compare to
        /// @return list position for two iterators is the same (true) or different (false)
        template <bool IsConstIteratorOther>
        bool operator==(const IteratorBase<IsConstIteratorOther>& rhs) const noexcept;

        /// @brief comparing list iterators for non-equality
        ///         the referenced list position is compared, not the content of the list element (T-typed)
        ///         -> there is no content for fictional elements at BEGIN_END_LINK_INDEX
        ///         only iterators of the same parent list can be compared; in case of misuse, terminate() is invoked
        /// @param[in] rhs is the 2nd iterator to compare to
        /// @return list position for two iterators is the same (true) or different (false)
        template <bool IsConstIteratorOther>
        bool operator!=(const IteratorBase<IsConstIteratorOther>& rhs) const noexcept;

        /// @brief dereferencing element content via iterator-position element
        /// @return reference to list element data
        reference operator*() const noexcept;

        /// @brief dereferencing element content via iterator-position element
        /// @return pointer to const list data element
        pointer operator->() const noexcept;


      private:
        using parentListPointer =
            typename std::conditional<IsConstIterator, const list<T, Capacity>*, list<T, Capacity>*>::type;

        /// @brief private construct for an iterator, the iterator is bundled to
        ///         an existing parent (object) of type list,
        ///         an iterator is only constructed through calls to begin() or end()
        /// @param[in] parent is the const list the this iterator operates on
        /// @param[in] idx is the index of the list element (within allocated memory of parent list)
        explicit IteratorBase(parentListPointer parent, size_type idx) noexcept;

        // Make IteratorBase<true> a friend class of IteratorBase<false> so the copy constructor can access the
        // private member variables.
        friend class IteratorBase<true>;
        friend class list<T, Capacity>;
        parentListPointer m_list;
        size_type m_iterListNodeIdx;
    };

    struct NodeLink
    {
        size_type nextIdx;
        size_type prevIdx;
    };

    void init() noexcept;
    T* getDataPtrFromIdx(const size_type idx) noexcept;
    const T* getDataPtrFromIdx(const size_type idx) const noexcept;

    bool isValidElementIdx(const size_type idx) const noexcept;
    bool isInvalidIterator(const const_iterator& iter) const noexcept;
    bool isInvalidIterOrDifferentLists(const const_iterator& iter) const noexcept;
    size_type& getPrevIdx(const size_type idx) noexcept;
    size_type& getNextIdx(const size_type idx) noexcept;
    size_type& getPrevIdx(const const_iterator& iter) noexcept;
    size_type& getNextIdx(const const_iterator& iter) noexcept;
    const size_type& getPrevIdx(const size_type idx) const noexcept;
    const size_type& getNextIdx(const size_type idx) const noexcept;
    const size_type& getPrevIdx(const const_iterator& iter) const noexcept;
    const size_type& getNextIdx(const const_iterator& iter) const noexcept;
    void setPrevIdx(const size_type idx, const size_type prevIdx) noexcept;
    void setNextIdx(const size_type idx, const size_type nextIdx) noexcept;

    static void errorMessage(const char* source, const char* msg) noexcept;

    //***************************************
    //    members
    //***************************************

    static constexpr size_type BEGIN_END_LINK_INDEX{size_type(Capacity)};
    static constexpr size_type NODE_LINK_COUNT{size_type(Capacity) + 1U};
    static constexpr size_type INVALID_INDEX{NODE_LINK_COUNT};

    // unused/free elements are stored in an internal list (freeList), this freeList is accessed via the
    // member variable m_freeListHeadIdx; user insert- and erase- operations move elements between the freeList and
    // active list
    size_type m_freeListHeadIdx{0U};

    // m_links array is one element bigger than request element count. In this additional element links are stored
    // to the beginning and end of the list. This additional element (index position 'capacity' aka
    // BEGIN_END_LINK_INDEX) 'previous' will point to the last valid element (end()) and 'next' will point to the
    // first used list element (begin())
    NodeLink m_links[NODE_LINK_COUNT];
    using element_t = uint8_t[sizeof(T)];
    alignas(T) element_t m_data[Capacity];

    size_type m_size{0U};
}; // list

} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/list.inl"

#endif // IOX_UTILS_CXX_LIST_HPP
