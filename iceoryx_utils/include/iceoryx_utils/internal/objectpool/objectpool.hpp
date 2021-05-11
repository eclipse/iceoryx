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
#ifndef IOX_UTILS_OBJECTPOOL_OBJECTPOOL_HPP
#define IOX_UTILS_OBJECTPOOL_OBJECTPOOL_HPP

#include <cstddef> //for size_t
#include <cstdint>
#include <utility> //for std::forward

namespace iox
{
namespace cxx
{
// TODO: finalize API and add doxygen comments
// the API will be safer and more concise
// furthermore the free position computation will be improved to gain performance

// note: as in e.g. std::vector, no index bounds checking for efficiency (using illegal indices leads to undefined
// behaviour)
template <typename T, int CAPACITY = 1> // TODO: use sensible and compatible type for this and NO_INDEX
class ObjectPool
{
  public:
    using Index_t = int; // TODO: choose Index_t types correctly wrt. size, conversion compatibility
    static constexpr int NO_INDEX = -1;

  private:
    static constexpr size_t CHUNKSIZE = sizeof(T);
    using Chunk = char[CHUNKSIZE];
    using Container = Chunk[CAPACITY]; // we need uninitalized memory without calling any constructors
                                       // we cannot use typed C arrays for this reason, since it would call T() (maybe
                                       // we could use a one element as CHUNK)


    Index_t m_freeIndex{0};
    size_t m_size{0u};

    // todo: maybe this metainformation can be combined, e.g. the data pointer == nullptr to indicate that
    // the data is invalid
    struct CellInfo
    {
        bool isValid{false};        // todo: rename into isUsed?
        bool wasConstructed{false}; // we want to use this to determine whether T destruction should occur in destructor
        T* data{nullptr};
    };

    alignas(T) Container m_values;
    CellInfo m_cellInfo[CAPACITY];
    char* m_first;
    char* m_last;

  public:
    class Iterator
    {
      private:
        Index_t index;
        ObjectPool<T, CAPACITY>* pool;

      public:
        Iterator(Index_t index, ObjectPool<T, CAPACITY>& pool)
            : index(index)
            , pool(&pool)
        {
        }

        T& operator*()
        {
            return *(pool->m_cellInfo[index].data);
        }

        // in contrast to operator* we do checking in operator->
        // for operator* this is not as straightforward, since we need
        // to return a reference (what should happen if iterator is end()?)
        T* operator->()
        {
            if (index >= CAPACITY)
            {
                return nullptr;
            }
            if ((pool->m_cellInfo[index]).isValid)
            {
                return pool->m_cellInfo[index].data;
            }
            return nullptr;
        }

        // pre-increment
        Iterator& operator++()
        {
            for (Index_t i = index + 1; i < CAPACITY; ++i)
            {
                if (pool->m_cellInfo[i].isValid == true)
                {
                    index = i;
                    return *this;
                }
            }
            index = CAPACITY;
            return *this;
        }

        // post increment
        Iterator operator++(int)
        {
            auto ret = Iterator(index, *pool);
            for (Index_t i = index + 1; i < CAPACITY; ++i)
            {
                if (pool->m_cellInfo[i].isValid == true)
                {
                    index = i;
                    return ret;
                }
            }
            index = CAPACITY;
            return ret;
        }

        bool operator!=(const Iterator& other) const
        {
            return (this->index != other.index || this->pool != other.pool);
        }

        bool operator==(const Iterator& other) const
        {
            return (this->index == other.index && this->pool == other.pool);
        }
    };

    Iterator begin()
    {
        for (Index_t i = 0; i < CAPACITY; ++i)
        {
            if (m_cellInfo[i].isValid == true)
            {
                return Iterator(i, *this);
            }
        }
        return Iterator(CAPACITY, *this);
    }

    Iterator end()
    {
        return Iterator(CAPACITY, *this);
    }

    ObjectPool()
        : m_first(reinterpret_cast<char*>(&(m_values[0])))
        , m_last(reinterpret_cast<char*>(&(m_values[CAPACITY - 1])))
    {
    }


    ~ObjectPool()
    {
        // destroy objects if they where constructed by the pool
        for (Index_t i = 0; i < CAPACITY; ++i)
        {
            if (m_cellInfo[i].isValid && m_cellInfo[i].wasConstructed)
            {
                m_cellInfo[i].data->~T();
            }
        }
    }

    //***********************************index API ***********************************************************

    Index_t reserve()
    {
        auto index = nextFree();

        if (index >= 0)
        {
            m_freeIndex = index;
            m_cellInfo[m_freeIndex].isValid = true;
            m_cellInfo[m_freeIndex].wasConstructed = false;
            ++m_size;
        }

        return index;
    }

    Index_t construct()
    {
        auto index = nextFree();

        if (index >= 0)
        {
            m_freeIndex = index;
            m_cellInfo[index].data = new (&m_values[index]) T; // use placement new
            m_cellInfo[m_freeIndex].isValid = true;
            m_cellInfo[m_freeIndex].wasConstructed = true;
            ++m_size;
        }

        return index;
    }

    template <typename... Args>
    Index_t construct(Args&&... args)
    {
        auto index = nextFree();

        if (index >= 0)
        {
            m_freeIndex = index;
            m_cellInfo[index].data = new (&m_values[index]) T(std::forward<Args>(args)...); // use placement new
            m_cellInfo[m_freeIndex].isValid = true;
            m_cellInfo[m_freeIndex].wasConstructed = true;
            ++m_size;
        }

        return index;
    }

    Index_t add(const T& element)
    {
        auto index = nextFree();

        if (index >= 0)
        {
            m_freeIndex = index;
            auto& cellInfo = m_cellInfo[m_freeIndex];
            cellInfo.data = new (m_values[m_freeIndex]) T(element);
            cellInfo.isValid = true;
            cellInfo.wasConstructed = true;
            ++m_size;
        }

        return index;
    }

    void remove(Index_t index, bool destruct = false)
    {
        if (m_cellInfo[index].isValid)
        {
            if (destruct)
            {
                m_cellInfo[index].data->~T();
            }
            m_cellInfo[index].isValid = false;
            --m_size;
        }
    }

    // unsafe by design (as std::vector), remove ...?
    T& operator[](Index_t index)
    {
        return *(m_cellInfo[index].data);
    }

    Iterator iterator(Index_t index)
    {
        if (m_cellInfo[index].isValid)
        {
            return Iterator(index, *this);
        }
        return end();
    }

    size_t size() const
    {
        return m_size;
    }

    size_t capacity() const
    {
        return CAPACITY;
    }

    //**************************** pointer API ********************************

    // get raw memory for object T
    T* allocate()
    {
        auto index = reserve();
        if (index == NO_INDEX)
        {
            return nullptr;
        }
        // access to raw memory where nothing was constructed yet
        // cannot be avoided easily with current logic
        // since the idea is that the user could also use this memory pointer
        // to place objects of type T in (should probably not be supported in the future
        // because it is dangerous if used wrongly)

        return reinterpret_cast<T*>(m_values[index]);
    }

    // default construct object T
    T* create()
    {
        auto index = construct();
        if (index == NO_INDEX)
        {
            return nullptr;
        }
        return get(index);
    }

    // construct object T with constructors taking arguments
    template <typename... Args>
    T* create(Args&&... args)
    {
        auto index = construct(std::forward<Args>(args)...);
        if (index == NO_INDEX)
        {
            return nullptr;
        }
        return get(index);
    }

    // free the cell associated with ptr and call destructor if destruct is true
    void free(T* ptr, bool destruct)
    {
        auto index = pointerToIndex(ptr);
        if (index >= 0)
        {
            remove(index, destruct);
        }
    }

    // free cell associated with ptr
    // call object destructor if and only if  it was constructed by the pool
    // note: this can be dangerous if the user also destroyed the objects via pointer
    void free(T* ptr)
    {
        auto index = pointerToIndex(ptr);
        if (index >= 0)
        {
            remove(index, m_cellInfo[index].wasConstructed);
        }
    }

    T* insert(const T& element)
    {
        auto index = add(element);

        if (index >= 0)
        {
            get(index);
        }
        else
        {
            return nullptr;
        }

        return get(index);
    }

    T* get(Index_t index)
    {
        return (m_cellInfo[index].isValid) ? m_cellInfo[index].data : nullptr;
    }

    T* get(T* ptr)
    {
        auto index = pointerToIndex(ptr);
        if (index != NO_INDEX)
        {
            return (m_cellInfo[index].isValid) ? m_cellInfo[index].data : nullptr;
        }
        return nullptr;
    }

    Iterator iterator(T* ptr)
    {
        auto index = pointerToIndex(ptr);
        if (index >= 0)
        {
            return iterator(index);
        }
        return end();
    }

    Index_t pointerToIndex(T* ptr)
    {
        /// todo: not critical, but refactor for pointer arithmetic later
        char* p = reinterpret_cast<char*>(ptr);
        if (p < m_first || p > m_last)
        {
            return NO_INDEX;
        }
        auto delta = p - m_first;
        if (static_cast<uint64_t>(delta) % sizeof(T) != 0)
        {
            return NO_INDEX;
        }

        // if the cell is valid and contains data we expect the pointer to equal data (alignment has to match)
        auto index = static_cast<Index_t>(static_cast<uint64_t>(delta) / sizeof(T));
        if (m_cellInfo[index].isValid && m_cellInfo[index].data)
        {
            if (m_cellInfo[index].data == ptr)
            {
                return index;
            }
            else
            {
                // pointer is not aligned to object in cell
                return NO_INDEX;
            }
        }

        return index;
    }

    T* indexToPointer(Index_t index)
    {
        return m_cellInfo[index].data;
    }

  private:
    // TODO: use fifo/index set for efficient search of free elements
    Index_t nextFree()
    {
        if (m_size >= CAPACITY)
            return NO_INDEX; // container is full

        for (; m_cellInfo[m_freeIndex].isValid; m_freeIndex = (m_freeIndex + 1) % CAPACITY)
            ;

        return m_freeIndex;
    }
};
} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_OBJECTPOOL_OBJECTPOOL_HPP
