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

#ifndef IOX_UTILS_CXX_FORWARD_LIST_INL
#define IOX_UTILS_CXX_FORWARD_LIST_INL


#include "iceoryx_utils/cxx/forward_list.hpp"


namespace iox
{
namespace cxx
{
template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>::forward_list() noexcept
{
    init();
}

template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>::forward_list(const forward_list& rhs) noexcept
{
    init();
    *this = rhs;
}

template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>::forward_list(forward_list&& rhs) noexcept
{
    init();
    *this = std::move(rhs);
}


template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>& forward_list<T, Capacity>::forward_list::operator=(const forward_list& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0U;
        auto iterThis = before_begin();
        auto citerRhs = rhs.cbefore_begin();
        auto startSize = size();
        auto minOfLhsRhsSize = std::min(rhs.size(), startSize);

        // copy using copy assignment
        for (; i < minOfLhsRhsSize; ++i)
        {
            ++iterThis;
            ++citerRhs;
            *iterThis = *citerRhs;
        }

        // with rhs.size bigger than this.size: copy further elements from rhs to this
        for (; i < rhs.size(); ++i)
        {
            ++citerRhs;
            iterThis = emplace_after(iterThis, *citerRhs);
        }

        // with rhs.size smaller than this.size: delete remaining elements of this
        for (; i < startSize; ++i)
        {
            erase_after(iterThis);
        }
    }
    return *this;
}

template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>& forward_list<T, Capacity>::forward_list::operator=(forward_list&& rhs) noexcept
{
    if (this != &rhs)
    {
        uint64_t i = 0U;
        auto iterThis = before_begin();
        auto iterRhs = rhs.before_begin();
        auto startSize = size();
        auto minOfLhsRhsSize = std::min(rhs.size(), startSize);

        // move using move assignment
        for (; i < minOfLhsRhsSize; ++i)
        {
            ++iterThis;
            ++iterRhs;
            *iterThis = std::move(*iterRhs);
        }

        // with rhs.size bigger than this.size: move construct further elements from rhs to this
        for (; i < rhs.size(); ++i)
        {
            ++iterRhs;
            iterThis = emplace_after(iterThis, std::move(*iterRhs));
        }

        // with rhs.size smaller than this.size: delete remaining elements of this
        for (; i < startSize; ++i)
        {
            erase_after(iterThis);
        }

        rhs.clear();
    }
    return *this;
}


template <typename T, uint64_t Capacity>
inline forward_list<T, Capacity>::~forward_list()
{
    clear();
}


template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator forward_list<T, Capacity>::before_begin() noexcept
{
    return iterator{this, BEFORE_BEGIN_INDEX};
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::before_begin() const noexcept
{
    return cbefore_begin();
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::cbefore_begin() const noexcept
{
    return const_iterator{this, BEFORE_BEGIN_INDEX};
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator forward_list<T, Capacity>::begin() noexcept
{
    return iterator{this, getNextIdx(BEFORE_BEGIN_INDEX)};
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::begin() const noexcept
{
    return cbegin();
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::cbegin() const noexcept
{
    return const_iterator{this, getNextIdx(BEFORE_BEGIN_INDEX)};
}


template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator forward_list<T, Capacity>::end() noexcept
{
    return iterator{this, END_INDEX};
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::end() const noexcept
{
    return cend();
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::const_iterator forward_list<T, Capacity>::cend() const noexcept
{
    return const_iterator{this, END_INDEX};
}


template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::empty() const noexcept
{
    return (m_size == 0U);
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::full() const noexcept
{
    return (m_size >= Capacity);
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type forward_list<T, Capacity>::size() const noexcept
{
    return m_size;
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type forward_list<T, Capacity>::capacity() const noexcept
{
    return Capacity;
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type forward_list<T, Capacity>::max_size() const noexcept
{
    return capacity();
}


template <typename T, uint64_t Capacity>
template <typename... ConstructorArgs>
inline T& forward_list<T, Capacity>::emplace_front(ConstructorArgs&&... args) noexcept
{
    return *emplace_after(before_begin(), std::forward<ConstructorArgs>(args)...);
}

template <typename T, uint64_t Capacity>
template <typename... ConstructorArgs>
inline typename forward_list<T, Capacity>::iterator
forward_list<T, Capacity>::emplace_after(const_iterator toBeEmplacedAfterIter, ConstructorArgs&&... args) noexcept
{
    if (isInvalidIterOrDifferentLists(toBeEmplacedAfterIter))
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

    // enable operations / allow access on element (e.g. getDataPtrFromIdx() )
    setInvalidElement(toBeAddedIdx, false);

    // data class c'tor
    new (getDataPtrFromIdx(toBeAddedIdx)) T(std::forward<ConstructorArgs>(args)...);

    // add to usedList
    setNextIdx(toBeAddedIdx, getNextIdx(toBeEmplacedAfterIter));
    setNextIdx(toBeEmplacedAfterIter.m_iterListNodeIdx, toBeAddedIdx);

    ++m_size;

    return iterator{this, toBeAddedIdx};
}


template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator
forward_list<T, Capacity>::erase_after(const_iterator toBeErasedAfterIter) noexcept
{
    if (isInvalidIterOrDifferentLists(toBeErasedAfterIter))
    {
        return end();
    }

    auto eraseIdx = getNextIdx(toBeErasedAfterIter);

    // additional validity check on to-be-erase element
    if (!isValidElementIdx(eraseIdx) || empty())
    {
        errorMessage(__PRETTY_FUNCTION__, " iterator is end() or list is empty");
        return end();
    }

    // unlink from usedList
    size_type retIdx = getNextIdx(eraseIdx);
    setNextIdx(toBeErasedAfterIter.m_iterListNodeIdx, retIdx);

    // d'tor data class
    getDataPtrFromIdx(eraseIdx)->~T();

    // add to freeList
    setInvalidElement(eraseIdx, true);
    setNextIdx(eraseIdx, m_freeListHeadIdx);
    m_freeListHeadIdx = eraseIdx;

    --m_size;

    // Iterator to the element following the erased one, or end() if no such element exists.
    return iterator{this, retIdx};
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type forward_list<T, Capacity>::remove(const T& data) noexcept
{
    return remove_if([&data](T& eachListElementData) { return eachListElementData == data; });
}


template <typename T, uint64_t Capacity>
template <typename UnaryPredicate>
inline typename forward_list<T, Capacity>::size_type forward_list<T, Capacity>::remove_if(UnaryPredicate pred) noexcept
{
    size_type removedCount = 0U;

    auto iter = before_begin();
    auto next_iter = begin();

    while (next_iter != cend())
    {
        if (pred(*next_iter))
        {
            next_iter = erase_after(iter);
            ++removedCount;
        }
        else
        {
            iter = next_iter;
            ++next_iter;
        }
    }
    return removedCount;
}


template <typename T, uint64_t Capacity>
inline T& forward_list<T, Capacity>::front() noexcept
{
    auto iter = begin();
    cxx::Expects(isValidElementIdx(iter.m_iterListNodeIdx) && "Invalid list element");
    return *iter;
}

template <typename T, uint64_t Capacity>
inline const T& forward_list<T, Capacity>::front() const noexcept
{
    auto citer = cbegin();
    cxx::Expects(isValidElementIdx(citer.m_iterListNodeIdx) && "Invalid list element");
    return *citer;
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::push_front(const T& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace_front(data);
    }
    return (m_size == ++sizeBeforePush);
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::push_front(T&& data) noexcept
{
    auto sizeBeforePush = m_size;
    if (m_size < Capacity)
    {
        emplace_front(std::forward<T>(data));
    }
    return (m_size == ++sizeBeforePush);
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::pop_front() noexcept
{
    auto sizeBeforeErase = m_size;
    erase_after(before_begin());
    return ((m_size + 1U) == sizeBeforeErase);
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator forward_list<T, Capacity>::insert_after(const_iterator citer,
                                                                                            const T& data) noexcept
{
    return emplace_after(citer, data);
}

template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::iterator forward_list<T, Capacity>::insert_after(const_iterator citer,
                                                                                            T&& data) noexcept
{
    return emplace_after(citer, std::forward<T>(data));
}


template <typename T, uint64_t Capacity>
inline void forward_list<T, Capacity>::clear() noexcept
{
    while (m_size)
    {
        erase_after(before_begin());
    }
}

/*************************/
// iterator

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline forward_list<T, Capacity>::IteratorBase<IsConstIterator>::IteratorBase(parentListPointer parent,
                                                                              size_type idx) noexcept
    : m_list(parent)
    , m_iterListNodeIdx(idx)
{
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline forward_list<T, Capacity>::IteratorBase<IsConstIterator>::IteratorBase(const IteratorBase<false>& iter) noexcept
    : m_list(iter.m_list)
    , m_iterListNodeIdx(iter.m_iterListNodeIdx)
{
}

template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename forward_list<T, Capacity>::template IteratorBase<IsConstIterator>&
forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator=(const IteratorBase<false>& rhs) noexcept
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
inline typename forward_list<T, Capacity>::template IteratorBase<IsConstIterator>&
forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator++() noexcept
{
    if (!m_list->isInvalidIterator(*this))
    {
        m_iterListNodeIdx = m_list->getNextIdx(m_iterListNodeIdx);
    }
    return *this;
}


template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
template <bool IsConstIteratorOther>
inline bool forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator==(
    const forward_list<T, Capacity>::IteratorBase<IsConstIteratorOther>& rhs) const noexcept
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
inline bool forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator!=(
    const forward_list<T, Capacity>::IteratorBase<IsConstIteratorOther>& rhs) const noexcept
{
    return !operator==(rhs);
}


template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename forward_list<T, Capacity>::template IteratorBase<IsConstIterator>::reference
    forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator*() const noexcept
{
    return *operator->();
}


template <typename T, uint64_t Capacity>
template <bool IsConstIterator>
inline typename forward_list<T, Capacity>::template IteratorBase<IsConstIterator>::pointer
    forward_list<T, Capacity>::IteratorBase<IsConstIterator>::operator->() const noexcept

{
    return m_list->getDataPtrFromIdx(m_iterListNodeIdx);
}


/*************************/
// private member functions

template <typename T, uint64_t Capacity>
inline void forward_list<T, Capacity>::init() noexcept
{
    for (size_type i = m_freeListHeadIdx; i < (Capacity - 1U); ++i)
    {
        setInvalidElement(i, true);
        setNextIdx(i, i + 1U);
    }
    // last of concatenated indices with a counterpart data-element in m_data
    setInvalidElement(Capacity - 1U, true);
    setNextIdx(Capacity - 1U, END_INDEX);

    // 'before_begin' element, pointing to 'begin' element or 'end' when list is empty
    setInvalidElement(Capacity, false);
    setNextIdx(Capacity, END_INDEX);
    // 'end' element, only pointing to itself
    setInvalidElement(Capacity + 1U, false);
    setNextIdx(Capacity + 1U, END_INDEX);
    m_freeListHeadIdx = 0U;

    m_size = 0U;
}


/*************************/
// list-node access

template <typename T, uint64_t Capacity>
inline const typename forward_list<T, Capacity>::size_type&
forward_list<T, Capacity>::getNextIdx(const size_type idx) const noexcept
{
    return m_links[idx].nextIdx;
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type&
forward_list<T, Capacity>::getNextIdx(const size_type idx) noexcept
{
    return const_cast<size_type&>(static_cast<const forward_list<T, Capacity>*>(this)->getNextIdx(idx));
}

template <typename T, uint64_t Capacity>
inline const typename forward_list<T, Capacity>::size_type&
forward_list<T, Capacity>::getNextIdx(const const_iterator& iter) const noexcept
{
    return getNextIdx(iter.m_iterListNodeIdx);
}
template <typename T, uint64_t Capacity>
inline typename forward_list<T, Capacity>::size_type&
forward_list<T, Capacity>::getNextIdx(const const_iterator& iter) noexcept
{
    return const_cast<size_type&>(static_cast<const forward_list<T, Capacity>*>(this)->getNextIdx(iter));
}

template <typename T, uint64_t Capacity>
inline void forward_list<T, Capacity>::setNextIdx(const size_type idx, const size_type nextIdx) noexcept
{
    m_links[idx].nextIdx = nextIdx;
}


template <typename T, uint64_t Capacity>
inline const T* forward_list<T, Capacity>::getDataPtrFromIdx(const size_type idx) const noexcept
{
    cxx::Expects(isValidElementIdx(idx) && "Invalid list element");

    return &(reinterpret_cast<const T*>(&m_data)[idx]);
}

template <typename T, uint64_t Capacity>
inline T* forward_list<T, Capacity>::getDataPtrFromIdx(const size_type idx) noexcept
{
    return const_cast<T*>(static_cast<const forward_list<T, Capacity>*>(this)->getDataPtrFromIdx(idx));
}

/*************************/
// failure handling / messaging

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::isInvalidElement(const size_type idx) const noexcept
{
    return m_links[idx].invalidElement;
}

template <typename T, uint64_t Capacity>
inline void forward_list<T, Capacity>::setInvalidElement(const size_type idx, const bool value) noexcept
{
    m_links[idx].invalidElement = value;
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::isValidElementIdx(const size_type idx) const noexcept
{
    return ((idx < Capacity) && !isInvalidElement(idx));
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::isInvalidIterator(const const_iterator& iter) const noexcept
{
    // iterator's member m_iterListNodeIdx and nextIndex are not checked here to be <= END_INDEX as this
    // should (can) never happen though normal list operations.
    cxx::Expects(!isInvalidElement(iter.m_iterListNodeIdx) && "invalidated iterator");
    return false;
}

template <typename T, uint64_t Capacity>
inline bool forward_list<T, Capacity>::isInvalidIterOrDifferentLists(const const_iterator& iter) const noexcept
{
    cxx::Expects((this == iter.m_list) && "iterator of other list can't be used");
    return isInvalidIterator(iter);
}

template <typename T, uint64_t Capacity>
inline void forward_list<T, Capacity>::errorMessage(const char* source, const char* msg) noexcept
{
    std::cerr << source << " ::: " << msg << std::endl;
}


} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_FORWARD_LIST_INL
