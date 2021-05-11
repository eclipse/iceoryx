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

#ifndef IOX_UTILS_CXX_LIST_INL
#define IOX_UTILS_CXX_LIST_INL


#include "iceoryx_utils/cxx/list.hpp"


namespace iox
{
namespace cxx
{
template <typename T, uint64_t Capacity>
inline list<T, Capacity>::list() noexcept
{
    init();
}

template <typename T, uint64_t Capacity>
inline list<T, Capacity>::list(const list& rhs) noexcept
{
    init();
    *this = rhs;
}

template <typename T, uint64_t Capacity>
inline list<T, Capacity>::list(list&& rhs) noexcept
{
    init();

    auto iter = cbegin();
    for (auto& element : rhs)
    {
        emplace(iter, std::move(element));
    }

    // optional clear(), but prefered to have defined state of source
    rhs.clear();
}


template <typename T, uint64_t Capacity>
inline list<T, Capacity>& list<T, Capacity>::list::operator=(const list& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0U;
        auto iterThis = begin();
        auto citerRhs = rhs.cbegin();
        auto startSize = size();
        auto minOfLhsRhsSize = std::min(rhs.size(), startSize);

        // copy using copy assignment
        for (; i < minOfLhsRhsSize; ++i)
        {
            *iterThis = *citerRhs;
            ++iterThis;
            ++citerRhs;
        }

        // with rhs.size bigger than this.size: copy further elements from rhs to this
        for (; i < rhs.size(); ++i)
        {
            emplace(cend(), *citerRhs);
            ++citerRhs;
        }

        // with rhs.size smaller than this.size: delete remaining elements of this
        for (; i < startSize; ++i)
        {
            iterThis = erase(iterThis);
        }
    }
    return *this;
}

template <typename T, uint64_t Capacity>
inline list<T, Capacity>& list<T, Capacity>::list::operator=(list&& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0U;
        auto iterThis = begin();
        auto citerRhs = rhs.begin();
        auto startSize = size();
        auto minOfLhsRhsSize = std::min(rhs.size(), startSize);

        // move using move assignment
        for (; i < minOfLhsRhsSize; ++i)
        {
            *iterThis = std::move(*citerRhs);
            ++iterThis;
            ++citerRhs;
        }

        // with rhs.size bigger than this.size: move construct further elements from rhs to this
        for (; i < rhs.size(); ++i)
        {
            emplace(cend(), std::move(*citerRhs));
            ++citerRhs;
        }

        // with rhs.size smaller than this.size: delete remaining elements of this
        for (; i < startSize; ++i)
        {
            iterThis = erase(iterThis);
        }

        rhs.clear();
    }
    return *this;
}


template <typename T, uint64_t Capacity>
inline list<T, Capacity>::~list()
{
    clear();
}


template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::iterator list<T, Capacity>::begin() noexcept
{
    return iterator{this, getNextIdx(BEGIN_END_LINK_INDEX)};
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::const_iterator list<T, Capacity>::begin() const noexcept
{
    return cbegin();
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::const_iterator list<T, Capacity>::cbegin() const noexcept
{
    return const_iterator{this, getNextIdx(BEGIN_END_LINK_INDEX)};
}


template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::iterator list<T, Capacity>::end() noexcept
{
    return iterator{this, BEGIN_END_LINK_INDEX};
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::const_iterator list<T, Capacity>::end() const noexcept
{
    return cend();
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::const_iterator list<T, Capacity>::cend() const noexcept
{
    return const_iterator{this, BEGIN_END_LINK_INDEX};
}


template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::empty() const noexcept
{
    return (m_size == 0U);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::full() const noexcept
{
    return (m_size >= Capacity);
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type list<T, Capacity>::size() const noexcept
{
    return m_size;
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type list<T, Capacity>::capacity() const noexcept
{
    return Capacity;
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type list<T, Capacity>::max_size() const noexcept
{
    return capacity();
}


template <typename T, uint64_t Capacity>
template <typename... ConstructorArgs>
inline T& list<T, Capacity>::emplace_front(ConstructorArgs&&... args) noexcept
{
    return *emplace(cbegin(), std::forward<ConstructorArgs>(args)...);
}

template <typename T, uint64_t Capacity>
template <typename... ConstructorArgs>
inline T& list<T, Capacity>::emplace_back(ConstructorArgs&&... args) noexcept
{
    return *emplace(cend(), std::forward<ConstructorArgs>(args)...);
}

template <typename T, uint64_t Capacity>
template <typename... ConstructorArgs>
inline typename list<T, Capacity>::iterator list<T, Capacity>::emplace(const_iterator iter,
                                                                       ConstructorArgs&&... args) noexcept
{
    if (isInvalidIterOrDifferentLists(iter))
    {
        return end();
    }

    if (m_size >= Capacity)
    {
        errorMessage(__PRETTY_FUNCTION__, " capacity exhausted ");
        return end();
    }


    // unlink from freeList
    const size_type toBeAddedIdx = m_freeListHeadIdx;
    m_freeListHeadIdx = getNextIdx(m_freeListHeadIdx);

    // set valid links / allowing access on element (e.g. getDataPtrFromIdx() )
    setPrevIdx(toBeAddedIdx, getPrevIdx(iter));
    setNextIdx(toBeAddedIdx, iter.m_iterListNodeIdx);

    // data class c'tor
    new (getDataPtrFromIdx(toBeAddedIdx)) T(std::forward<ConstructorArgs>(args)...);

    // add to active list (before iter position)
    setNextIdx(getPrevIdx(iter), toBeAddedIdx);
    setPrevIdx(iter.m_iterListNodeIdx, toBeAddedIdx);

    ++m_size;

    return iterator{this, toBeAddedIdx};
}


template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::iterator list<T, Capacity>::erase(const_iterator iter) noexcept
{
    if (isInvalidIterOrDifferentLists(iter))
    {
        return end();
    }

    auto eraseIdx = iter.m_iterListNodeIdx;

    // further narrow-down checks
    if (!isValidElementIdx(eraseIdx) || empty())
    {
        errorMessage(__PRETTY_FUNCTION__, " iterator is end() or list is empty");
        return end();
    }

    // unlink from active list
    size_type retIdx = getNextIdx(iter);
    setPrevIdx(retIdx, getPrevIdx(iter));
    setNextIdx(getPrevIdx(iter), retIdx);

    // d'tor data class
    (*iter).~T();

    // mark index as unused / invalid via 'previous index' set to INVALID_INDEX while index is handled within freeList
    setPrevIdx(eraseIdx, INVALID_INDEX);
    // add to freeList
    setNextIdx(eraseIdx, m_freeListHeadIdx);
    m_freeListHeadIdx = eraseIdx;

    --m_size;

    // Iterator to the element following the erased one, or end() if no such element exists.
    return iterator{this, retIdx};
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type list<T, Capacity>::remove(const T& data) noexcept
{
    return remove_if([&data](T& eachListElementData) { return eachListElementData == data; });
}


template <typename T, uint64_t Capacity>
template <typename UnaryPredicate>
inline typename list<T, Capacity>::size_type list<T, Capacity>::remove_if(UnaryPredicate pred) noexcept
{
    size_type removedCount = 0U;

    auto iter = begin();

    while (iter != cend())
    {
        if (pred(*iter))
        {
            iter = erase(iter);
            ++removedCount;
        }
        else
        {
            ++iter;
        }
    }
    return removedCount;
}


template <typename T, uint64_t Capacity>
inline T& list<T, Capacity>::front() noexcept
{
    auto iter = begin();
    cxx::Expects(isValidElementIdx(iter.m_iterListNodeIdx) && "invalid list element");
    return *iter;
}

template <typename T, uint64_t Capacity>
inline const T& list<T, Capacity>::front() const noexcept
{
    auto citer = cbegin();
    cxx::Expects(isValidElementIdx(citer.m_iterListNodeIdx) && "invalid list element");
    return *citer;
}

template <typename T, uint64_t Capacity>
inline T& list<T, Capacity>::back() noexcept
{
    auto iter = end();
    cxx::Expects(isValidElementIdx((--iter).m_iterListNodeIdx) && "invalid list element");
    return *iter;
}

template <typename T, uint64_t Capacity>
inline const T& list<T, Capacity>::back() const noexcept
{
    auto citer = cend();
    cxx::Expects(isValidElementIdx((--citer).m_iterListNodeIdx) && "invalid list element");
    return *citer;
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::push_front(const T& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace(cbegin(), data);
    }
    return (m_size == ++sizeBeforePush);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::push_front(T&& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace(cbegin(), std::forward<T>(data));
    }
    return (m_size == ++sizeBeforePush);
}


template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::push_back(const T& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace(cend(), data);
    }
    return (m_size == ++sizeBeforePush);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::push_back(T&& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace(cend(), std::forward<T>(data));
    }
    return (m_size == ++sizeBeforePush);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::pop_front() noexcept
{
    auto sizeBeforeErase = m_size;
    erase(begin());
    return ((m_size + 1U) == sizeBeforeErase);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::pop_back() noexcept
{
    auto sizeBeforeErase = m_size;
    erase(--end());
    return ((m_size + 1U) == sizeBeforeErase);
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::iterator list<T, Capacity>::insert(const_iterator citer, const T& data) noexcept
{
    return emplace(citer, data);
}

template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::iterator list<T, Capacity>::insert(const_iterator citer, T&& data) noexcept
{
    return emplace(citer, std::forward<T>(data));
}


template <typename T, uint64_t Capacity>
inline void list<T, Capacity>::clear() noexcept
{
    while (m_size)
    {
        erase(begin());
    }
}

/*************************/
// iterator

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline list<T, Capacity>::IteratorBase<IsConstIterator>::IteratorBase(parentListPointer parent, size_type idx) noexcept
    : m_list(parent)
    , m_iterListNodeIdx(idx)
{
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline list<T, Capacity>::IteratorBase<IsConstIterator>::IteratorBase(const IteratorBase<false>& iter) noexcept
    : m_list(iter.m_list)
    , m_iterListNodeIdx(iter.m_iterListNodeIdx)
{
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename list<T, Capacity>::template IteratorBase<IsConstIterator>&
list<T, Capacity>::IteratorBase<IsConstIterator>::operator=(const IteratorBase<false>& rhs) noexcept
{
    if (reinterpret_cast<const void*>(this) != reinterpret_cast<const void*>(&rhs))
    {
        m_list = rhs.m_list;
        m_iterListNodeIdx = rhs.m_iterListNodeIdx;
    }

    return *this;
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename list<T, Capacity>::template IteratorBase<IsConstIterator>&
list<T, Capacity>::IteratorBase<IsConstIterator>::operator++() noexcept
{
    if (!m_list->isInvalidIterator(*this))
    {
        // no increment beyond end() / no restart at begin()
        if (m_list->isValidElementIdx(m_iterListNodeIdx))
        {
            m_iterListNodeIdx = m_list->getNextIdx(m_iterListNodeIdx);
        }
    }
    return *this;
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename list<T, Capacity>::template IteratorBase<IsConstIterator>&
list<T, Capacity>::IteratorBase<IsConstIterator>::operator--() noexcept

{
    if (!m_list->isInvalidIterator(*this))
    {
        // no decrement beyond begin() / no restart at end()
        // decrementing an iterator pointing towards begin() has no effect (iterator stays at begin())
        if (m_list->isValidElementIdx(m_list->getPrevIdx(m_iterListNodeIdx)))
        {
            m_iterListNodeIdx = m_list->getPrevIdx(m_iterListNodeIdx);
        }
    }
    return *this;
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
template <bool IsConstIteratorOther>
inline bool list<T, Capacity>::IteratorBase<IsConstIterator>::operator==(
    const list<T, Capacity>::IteratorBase<IsConstIteratorOther>& rhs) const noexcept
{
    if (m_list->isInvalidIterOrDifferentLists(rhs) || m_list->isInvalidIterator(*this))
    {
        return false;
    }
    // index comparison
    return (m_iterListNodeIdx == rhs.m_iterListNodeIdx);
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
template <bool IsConstIteratorOther>
inline bool list<T, Capacity>::IteratorBase<IsConstIterator>::operator!=(
    const list<T, Capacity>::IteratorBase<IsConstIteratorOther>& rhs) const noexcept
{
    return !operator==(rhs);
}


template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename list<T, Capacity>::template IteratorBase<IsConstIterator>::reference
    list<T, Capacity>::IteratorBase<IsConstIterator>::operator*() const noexcept
{
    return *operator->();
}


template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename list<T, Capacity>::template IteratorBase<IsConstIterator>::pointer
    list<T, Capacity>::IteratorBase<IsConstIterator>::operator->() const noexcept

{
    return m_list->getDataPtrFromIdx(m_iterListNodeIdx);
}


/*************************/
// private member functions

template <typename T, uint64_t Capacity>
inline void list<T, Capacity>::init() noexcept
{
    // all list elements are concatenated and become accessible via the 'freeListHead'
    setPrevIdx(0U, INVALID_INDEX);
    setNextIdx(0U, 1U);

    for (size_type i = 1U; i < (BEGIN_END_LINK_INDEX - 1U); ++i)
    {
        setPrevIdx(i, INVALID_INDEX);
        setNextIdx(i, i + 1U);
    }
    setPrevIdx(Capacity - 1U, INVALID_INDEX);
    setNextIdx(Capacity - 1U, BEGIN_END_LINK_INDEX);
    // reset Heads
    // index '(Capacity)' is the 'end' element of the list which does not have a m_data value (payload), instead
    // it will only provide a link to the first and last _used_-element of the list
    setPrevIdx(Capacity, BEGIN_END_LINK_INDEX);
    setNextIdx(Capacity, BEGIN_END_LINK_INDEX);
    m_freeListHeadIdx = 0U;

    m_size = 0U;
}


/*************************/
// list-node access

template <typename T, uint64_t Capacity>
inline const typename list<T, Capacity>::size_type& list<T, Capacity>::getPrevIdx(const size_type idx) const noexcept
{
    return m_links[idx].prevIdx;
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type& list<T, Capacity>::getPrevIdx(const size_type idx) noexcept
{
    return const_cast<size_type&>(static_cast<const list<T, Capacity>*>(this)->getPrevIdx(idx));
}

template <typename T, uint64_t Capacity>
inline const typename list<T, Capacity>::size_type& list<T, Capacity>::getNextIdx(const size_type idx) const noexcept
{
    return m_links[idx].nextIdx;
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type& list<T, Capacity>::getNextIdx(const size_type idx) noexcept
{
    return const_cast<size_type&>(static_cast<const list<T, Capacity>*>(this)->getNextIdx(idx));
}

template <typename T, uint64_t Capacity>
inline const typename list<T, Capacity>::size_type& list<T, Capacity>::getPrevIdx(const const_iterator& iter) const
    noexcept
{
    return getPrevIdx(iter.m_iterListNodeIdx);
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type& list<T, Capacity>::getPrevIdx(const const_iterator& iter) noexcept
{
    return getPrevIdx(iter.m_iterListNodeIdx);
}

template <typename T, uint64_t Capacity>
inline const typename list<T, Capacity>::size_type& list<T, Capacity>::getNextIdx(const const_iterator& iter) const
    noexcept
{
    return getNextIdx(iter.m_iterListNodeIdx);
}
template <typename T, uint64_t Capacity>
inline typename list<T, Capacity>::size_type& list<T, Capacity>::getNextIdx(const const_iterator& iter) noexcept
{
    return getNextIdx(iter.m_iterListNodeIdx);
}


template <typename T, uint64_t Capacity>
inline void list<T, Capacity>::setPrevIdx(const size_type idx, const size_type prevIdx) noexcept
{
    m_links[idx].prevIdx = prevIdx;
}
template <typename T, uint64_t Capacity>
inline void list<T, Capacity>::setNextIdx(const size_type idx, const size_type nextIdx) noexcept
{
    m_links[idx].nextIdx = nextIdx;
}


template <typename T, uint64_t Capacity>
inline const T* list<T, Capacity>::getDataPtrFromIdx(const size_type idx) const noexcept
{
    cxx::Expects(isValidElementIdx(idx) && "invalid list element");

    return &(reinterpret_cast<const T*>(&m_data)[idx]);
}

template <typename T, uint64_t Capacity>
inline T* list<T, Capacity>::getDataPtrFromIdx(const size_type idx) noexcept
{
    return const_cast<T*>(static_cast<const list<T, Capacity>*>(this)->getDataPtrFromIdx(idx));
}

/*************************/
// failure handling / messaging

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::isValidElementIdx(const size_type idx) const noexcept
{
    return (idx < Capacity) && (getPrevIdx(idx) < INVALID_INDEX);
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::isInvalidIterator(const const_iterator& iter) const noexcept
{
    // freeList / invalid elements will have the prevIdx set to INVALID_INDEX
    // additional check on e.g. nextIdx or m_iterListNodeIdx (<INVALID_INDEX) are omitted as this
    // should (can) never happen though normal list operations.
    cxx::Expects((getPrevIdx(iter) < INVALID_INDEX) && "invalidated iterator");
    return false;
}

template <typename T, uint64_t Capacity>
inline bool list<T, Capacity>::isInvalidIterOrDifferentLists(const const_iterator& iter) const noexcept
{
    cxx::Expects((this == iter.m_list) && "iterator of other list can't be used");
    return isInvalidIterator(iter);
}

template <typename T, uint64_t Capacity>
inline void list<T, Capacity>::errorMessage(const char* source, const char* msg) noexcept
{
    std::cerr << source << " ::: " << msg << std::endl;
}


} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_LIST_INL
