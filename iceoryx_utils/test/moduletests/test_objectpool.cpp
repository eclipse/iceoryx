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

#if !defined(_WIN32)

#define private public
#include "iceoryx_utils/internal/objectpool/objectpool.hpp"
#undef private

#include <vector>

#include "test.hpp"

namespace
{
using namespace ::testing;

constexpr int INVALID = -1;


// non primitive type for pool
class Foo
{
  public:
    Foo()
    {
        ++constructionCounter;
    }

    Foo(int& data)
        : m_data(&data)
    {
        *m_data = ++constructionCounter;
    }

    ~Foo()
    {
        if (m_data)
        {
            *m_data = INVALID; // invalidated by destructor, visible outside to check destructor call by Pool
        }
        ++destructionCounter;
    }

    int* m_data{nullptr};

    static void resetConstructionCounter()
    {
        constructionCounter = 0;
    }

    static int getConstructionCounter()
    {
        return constructionCounter;
    }

    static void resetDestructionCounter()
    {
        destructionCounter = 0;
    }

    static int getDestructionCounter()
    {
        return destructionCounter;
    }

    static int constructionCounter; // count the number of Foo objects constructed
    static int destructionCounter;  // count the number of Foo objects constructed
};

int Foo::constructionCounter{0};
int Foo::destructionCounter{0};

// needed signed and unsigned to avoid some sign comparison issues
constexpr int CAPACITY = 3;
constexpr size_t CAPACITY_UNSIGNED = static_cast<size_t>(CAPACITY);

using FooPool = iox::cxx::ObjectPool<Foo, CAPACITY>;
using Index_t = FooPool::Index_t;
constexpr int NO_INDEX = FooPool::NO_INDEX;


class ObjectPool_test : public Test
{
  public:
    void SetUp()
    {
        internal::CaptureStderr();
        Foo::resetConstructionCounter(); // reset Foo constructionCounter in each test
        Foo::resetDestructionCounter();  // reset Foo destructionCounter in each test
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    int data;
    int data1;
    int data2;
    int data3;
    FooPool pool;
};


// check whether the constructed pool objects (of type Foo) have the intended data and construction/destruction
// behaviour  many other tests depend on this behaviour to track construction/destruction and associated data
TEST_F(ObjectPool_test, poolObjectBehaviour)
{
    // check that Foo objects behave correctly (i.e. static constructionCounter works and destructor effects data)
    int data = 73; // databuffer of foo
    {
        Foo foo(data);
        EXPECT_THAT(data, Eq(1));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(data));
        // Foo destructor called -> data invalid
    }
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(1));
    EXPECT_THAT(data, Eq(INVALID));

    {
        Foo foo(data);
        EXPECT_THAT(data, Eq(2));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(data));
    }
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(2));
    EXPECT_THAT(data, Eq(INVALID));

    Foo::resetConstructionCounter();
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0));

    Foo::resetDestructionCounter();
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
}


TEST_F(ObjectPool_test, construction)
{
    // pool initialized correctly, check size and capacity
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
}


TEST_F(ObjectPool_test, reserve)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto index = pool.reserve();
        EXPECT_THAT(index, Ne(NO_INDEX));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full
    auto index = pool.reserve();
    EXPECT_THAT(index, Eq(NO_INDEX));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}


TEST_F(ObjectPool_test, default_construct)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto index = pool.construct();
        EXPECT_THAT(index, Ne(NO_INDEX));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, nothing constructed
    auto index = pool.construct();
    EXPECT_THAT(index, Eq(NO_INDEX));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, parameter_construct)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    data = 0;
    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto index = pool.construct(data);
        EXPECT_THAT(index, Ne(NO_INDEX));
        EXPECT_THAT(data, Eq(i));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, nothing constructed
    auto index = pool.construct(data);
    EXPECT_THAT(index, Eq(NO_INDEX));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(CAPACITY));
    EXPECT_THAT(data, Eq(CAPACITY));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, add)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    data = 0;
    for (int i = 1; i <= CAPACITY; ++i)
    {
        Foo foo(data);
        EXPECT_THAT(data, Eq(i));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));

        auto index = pool.add(foo);
        EXPECT_THAT(index, Ne(NO_INDEX));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, cannot add
    Foo foo(data);
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(CAPACITY + 1));
    EXPECT_THAT(data, Eq(CAPACITY + 1));
    auto index = pool.add(foo);
    EXPECT_THAT(index, Eq(NO_INDEX));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}


TEST_F(ObjectPool_test, size_and_remove)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto index1 = pool.construct(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    data2 = 0;
    Foo foo(data2);
    auto index2 = pool.add(foo);
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));
    ASSERT_THAT(pool.get(index1), Ne(nullptr));

    pool.remove(index1);
    EXPECT_THAT(pool.get(index1), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    ASSERT_THAT(pool.get(index2), Ne(nullptr));
    pool.remove(index2, true);
    EXPECT_THAT(data2, Eq(INVALID));
    EXPECT_THAT(pool.get(index2), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(1));
    EXPECT_THAT(pool.size(), Eq(0U));
}

TEST_F(ObjectPool_test, bracket_operator)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto index1 = pool.construct(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));
    ASSERT_THAT(pool.get(index1), Ne(nullptr));

    data2 = 0;
    Foo foo(data2);
    auto index2 = pool.add(foo);
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));
    ASSERT_THAT(pool.get(index2), Ne(nullptr));

    Foo& ret1 = pool[index1]; // check whether we get the reference to the correct object (associated with data1)
    EXPECT_THAT(ret1.m_data, Eq(&data1));

    Foo& ret2 = pool[index2]; // check whether we get the reference to the correct object (associated with data2)
    EXPECT_THAT(ret2.m_data, Eq(&data2));
}

TEST_F(ObjectPool_test, allocate)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto ptr = pool.allocate();
        EXPECT_THAT(ptr, Ne(nullptr));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full
    auto ptr = pool.allocate();
    EXPECT_THAT(ptr, Eq(nullptr));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, default_create)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto ptr = pool.create();
        EXPECT_THAT(ptr, Ne(nullptr));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, nothing constructed
    auto ptr = pool.create();
    EXPECT_THAT(ptr, Eq(nullptr));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, parameter_create)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    data = 0;
    for (int i = 1; i <= CAPACITY; ++i)
    {
        auto ptr = pool.create(data);
        EXPECT_THAT(ptr, Ne(nullptr));
        EXPECT_THAT(data, Eq(i));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, nothing constructed
    auto ptr = pool.create(data);
    EXPECT_THAT(ptr, Eq(nullptr));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(CAPACITY));
    EXPECT_THAT(data, Eq(CAPACITY));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, destruct_free)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto ptr1 = pool.create(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    data2 = 0;
    Foo foo(data2);
    auto ptr2 = pool.insert(foo);
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));

    ASSERT_THAT(ptr1, Ne(nullptr));
    pool.free(ptr1, false);
    EXPECT_THAT(pool.get(ptr1), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    ASSERT_THAT(ptr2, Ne(nullptr));
    pool.free(ptr2, true);
    EXPECT_THAT(data2, Eq(INVALID));
    EXPECT_THAT(pool.get(ptr2), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(1));
    EXPECT_THAT(pool.size(), Eq(0U));
}

TEST_F(ObjectPool_test, default_free)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto ptr1 = pool.create(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    data2 = 0;
    Foo foo(data2);
    auto ptr2 = pool.insert(foo);
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));

    auto ptr3 = pool.allocate();
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(3U));

    ASSERT_THAT(ptr1, Ne(nullptr));
    pool.free(ptr1);
    EXPECT_THAT(pool.get(ptr1), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(1));
    EXPECT_THAT(pool.size(), Eq(2U));

    ASSERT_THAT(ptr2, Ne(nullptr));
    pool.free(ptr2);
    EXPECT_THAT(data2, Eq(INVALID));
    EXPECT_THAT(pool.get(ptr2), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(2));
    EXPECT_THAT(pool.size(), Eq(1U));

    ASSERT_THAT(ptr3, Ne(nullptr));
    pool.free(ptr3);
    EXPECT_THAT(pool.get(ptr3), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(2));
    EXPECT_THAT(pool.size(), Eq(0U));
}

TEST_F(ObjectPool_test, insert)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    data = 0;
    for (int i = 1; i <= CAPACITY; ++i)
    {
        Foo foo(data);
        EXPECT_THAT(data, Eq(i));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(i));
        auto ptr = pool.insert(foo);
        EXPECT_THAT(ptr, Ne(nullptr));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full, cannot add
    Foo foo(data);
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(CAPACITY + 1));
    EXPECT_THAT(data, Eq(CAPACITY + 1));
    auto ptr = pool.insert(foo);
    EXPECT_THAT(ptr, Eq(nullptr));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));
}

TEST_F(ObjectPool_test, get)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto index1 = pool.construct(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    data2 = 0;
    Foo foo(data2);
    auto ptr2 = pool.insert(foo);
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));

    ASSERT_THAT(index1, Ne(NO_INDEX));
    ASSERT_THAT(ptr2, Ne(nullptr));

    auto ptr1 = pool.get(index1);
    ASSERT_THAT(ptr1, Ne(nullptr));

    auto index2 = pool.get(ptr2);
    auto ptr = pool.get(index2);
    ASSERT_THAT(ptr, Ne(nullptr));

    // check whether indicies/pointers are associated with the correct values
    EXPECT_THAT(ptr1->m_data, Eq(&data1));
    EXPECT_THAT(ptr2->m_data, Eq(&data2));
    EXPECT_THAT(ptr->m_data, Eq(&data2));

    // remove element and check whether we get a nullptr
    pool.free(ptr1, false);
    EXPECT_THAT(pool.get(ptr1), Eq(nullptr));
    EXPECT_THAT(pool.get(index1), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));
}

TEST_F(ObjectPool_test, pointerToIndexConversion)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto index1 = pool.construct(data1);
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));
    ASSERT_THAT(index1, Ne(NO_INDEX));

    auto ptr1 = pool.get(index1);
    ASSERT_THAT(ptr1, Ne(nullptr));

    // check whether indicies/pointers are associated with the correct values
    EXPECT_THAT(ptr1->m_data, Eq(&data1));

    EXPECT_THAT(pool.pointerToIndex(ptr1), Eq(index1));
    EXPECT_THAT(pool.indexToPointer(index1), Eq(ptr1));

    // remove element and check whether we get a nullptr
    pool.free(ptr1);
    EXPECT_THAT(pool.get(ptr1), Eq(nullptr));
    EXPECT_THAT(pool.get(index1), Eq(nullptr));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(1));
    EXPECT_THAT(pool.size(), Eq(0U));

    // conversion does not care about valid data, index1 still corresponds to ptr1
    EXPECT_THAT(pool.pointerToIndex(ptr1), Eq(index1));
    EXPECT_THAT(pool.indexToPointer(index1), Eq(ptr1));

    // legal pointer checks
    auto first = reinterpret_cast<Foo*>(pool.m_first);
    auto last = reinterpret_cast<Foo*>(pool.m_last);
    auto alignedPtr = reinterpret_cast<Foo*>(pool.m_first + sizeof(Foo));

    EXPECT_THAT(pool.pointerToIndex(first), Eq(0));
    EXPECT_THAT(pool.pointerToIndex(last), Eq(CAPACITY - 1));
    EXPECT_THAT(pool.pointerToIndex(alignedPtr), Eq(1));


    // illegal pointer checks
    auto lowPtr = reinterpret_cast<Foo*>(pool.m_first - 1);  // out of reserved memory ranged (too small)
    auto highPtr = reinterpret_cast<Foo*>(pool.m_first + 1); // out of reserved memory range (too large)
    // pointer - first is divisable by sizeof(Foo),  assumes Foo is larger than one byte
    auto nonalignedPtr = reinterpret_cast<Foo*>(pool.m_first + sizeof(Foo) + 1);

    EXPECT_THAT(pool.pointerToIndex(lowPtr), Eq(NO_INDEX));
    EXPECT_THAT(pool.pointerToIndex(highPtr), Eq(NO_INDEX));
    EXPECT_THAT(pool.pointerToIndex(nonalignedPtr), Eq(NO_INDEX));
}

// private API, important for correct behaviour of all other functions
// also test whether finding the next free cell (if it exists) works correctly
TEST_F(ObjectPool_test, nextFree)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

    for (int i = 1; i <= CAPACITY; ++i)
    {
        // changes object state but does not matter, if there is some free cell it has to be found
        EXPECT_THAT(pool.nextFree(), Ne(NO_INDEX));

        // populate pool
        auto index = pool.reserve();
        EXPECT_THAT(index, Ne(NO_INDEX));
    }
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    // pool is full
    auto index = pool.reserve();
    EXPECT_THAT(index, Eq(NO_INDEX));
    EXPECT_THAT(pool.size(), Eq(CAPACITY_UNSIGNED));

    EXPECT_THAT(pool.nextFree(), Eq(NO_INDEX));
}

TEST_F(ObjectPool_test, destructor)
{
    // allocate objects without construction
    {
        FooPool localPool; // local pool, to check destruction of objects

        EXPECT_THAT(localPool.size(), Eq(0U));
        EXPECT_THAT(localPool.capacity(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

        for (int i = 1; i <= CAPACITY; ++i)
        {
            // populate pool
            auto index = localPool.reserve();
            EXPECT_THAT(index, Ne(NO_INDEX));
        }
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));

        // pool is full
        auto index = localPool.reserve();
        EXPECT_THAT(index, Eq(NO_INDEX));
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    }
    // localPool destructor was called, but since the objects of the pool
    // were not construced by the pool (merely allocated) no Foo destructors are called
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));

    // default construction of Foo objects by pool
    {
        FooPool localPool; // local pool, to check destruction of objects

        EXPECT_THAT(localPool.size(), Eq(0U));
        EXPECT_THAT(localPool.capacity(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

        for (int i = 1; i <= CAPACITY; ++i)
        {
            // populate pool
            auto index = localPool.construct();
            EXPECT_THAT(index, Ne(NO_INDEX));
        }
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));

        // pool is full
        auto index = localPool.reserve();
        EXPECT_THAT(index, Eq(NO_INDEX));
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    }
    // localPool destructor was called, and since the objects were constructed by the pool
    // Foo destructors are called CAPACITY times
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(CAPACITY));

    Foo::resetConstructionCounter();
    Foo::resetDestructionCounter();
    data = 0;

    // default construction of Foo objects by pool
    {
        FooPool localPool; // local pool, to check destruction of objects

        EXPECT_THAT(localPool.size(), Eq(0U));
        EXPECT_THAT(localPool.capacity(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
        EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool

        for (int i = 1; i <= CAPACITY; ++i)
        {
            // populate pool
            auto index = localPool.construct(data);
            EXPECT_THAT(index, Ne(NO_INDEX));
        }
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));

        // pool is full
        auto index = localPool.reserve();
        EXPECT_THAT(index, Eq(NO_INDEX));
        EXPECT_THAT(localPool.size(), Eq(CAPACITY_UNSIGNED));
        EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    }
    // localPool destructor was called, and since the objects were constructed by the pool
    // Foo destructors are called CAPACITY times
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(CAPACITY));
    EXPECT_THAT(data, Eq(INVALID));
}

// test all iterator functions in this test since they are closely related
TEST_F(ObjectPool_test, iterator)
{
    EXPECT_THAT(pool.size(), Eq(0U));
    EXPECT_THAT(pool.capacity(), Eq(CAPACITY_UNSIGNED));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(0)); // no Foo objects constructed by pool
    EXPECT_THAT(pool.size(), Eq(0U));

    data1 = 0;
    auto index1 = pool.construct(data1);
    EXPECT_THAT(index1, Ne(NO_INDEX));
    EXPECT_THAT(data1, Eq(1));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(1));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(1U));

    data2 = 0;
    auto index2 = pool.construct(data2);
    EXPECT_THAT(index2, Ne(NO_INDEX));
    EXPECT_THAT(data2, Eq(2));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(2));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(2U));

    data3 = 0;
    auto index3 = pool.construct(data3);
    EXPECT_THAT(index3, Ne(NO_INDEX));
    EXPECT_THAT(data3, Eq(3));
    EXPECT_THAT(Foo::getConstructionCounter(), Eq(3));
    EXPECT_THAT(Foo::getDestructionCounter(), Eq(0));
    EXPECT_THAT(pool.size(), Eq(3U));

    // container is full

    // construct iterators

    auto iter1 = pool.iterator(index1);
    auto iter2 = pool.iterator(index2);
    auto iter3 = pool.iterator(index3);
    auto iterBegin = pool.begin();
    auto iterEnd = pool.end();


    // comparison test (operator!=)
    EXPECT_THAT(iterBegin, Ne(iterEnd));
    EXPECT_THAT(iter1, Ne(iterEnd));
    EXPECT_THAT(iter2, Ne(iterEnd));
    EXPECT_THAT(iter3, Ne(iterEnd));

    // pairwise not equal
    EXPECT_THAT(iter1, Ne(iter2));
    EXPECT_THAT(iter1, Ne(iter3));
    EXPECT_THAT(iter2, Ne(iter3));

    EXPECT_THAT(iter1, Eq(iter1));
    EXPECT_THAT(iter2, Eq(iter2));
    EXPECT_THAT(iter3, Eq(iter3));
    EXPECT_THAT(iterBegin, Eq(iterBegin));
    EXPECT_THAT(iterEnd, Eq(iterEnd));

    // operator++

    // post increment
    auto iter = iter1; // we need the copy since operator++ changes the iterator
    EXPECT_THAT(iter++, Eq(iter1));

    iter = iter2;
    EXPECT_THAT(iter++, Eq(iter2));

    iter = iter3;
    EXPECT_THAT(iter++, Eq(iter3));

    iter = iterBegin;
    EXPECT_THAT(iter++, Eq(iterBegin));

    iter = iterEnd;
    EXPECT_THAT(iter++, Eq(iterEnd));


    // pre increment
    iter = iter1;
    EXPECT_THAT(++iter, Ne(iter1));

    iter = iter2;
    EXPECT_THAT(++iter, Ne(iter2));

    iter = iter3;
    EXPECT_THAT(++iter, Ne(iter3));

    iter = iterBegin;
    EXPECT_THAT(++iter, Ne(iterBegin));

    iter = iterEnd;
    EXPECT_THAT(++iter, Eq(iterEnd));

    // operator*
    EXPECT_THAT(((*iter1).m_data), Eq(&data1));
    EXPECT_THAT(((*iter2).m_data), Eq(&data2));
    EXPECT_THAT(((*iter3).m_data), Eq(&data3));

    // operator->
    EXPECT_THAT(iter1->m_data, Eq(&data1));
    EXPECT_THAT(iter2->m_data, Eq(&data2));
    EXPECT_THAT(iter3->m_data, Eq(&data3));
    EXPECT_THAT(iterEnd.operator->(), Eq(nullptr));

    // check that after CAPACITY increments we have reached end
    //(to reduce potential for unbounded loops)
    auto iterPre = pool.begin();
    auto iterPost = pool.begin();

    for (size_t i = 0; i < pool.size(); ++i)
    {
        ++iterPre;
        iterPost++;
    }
    ASSERT_THAT(iterPre, Eq(iterEnd));
    ASSERT_THAT(iterPost, Eq(iterEnd));

    // we now know that the iterator increment does not lead to unbounded loops...
    // test range based loop which relies on iterators internally

    std::vector<int> count(4, 0);
    int numElements = 0;
    for (auto& foo : pool)
    {
        auto value = *foo.m_data;
        if (value >= 1 && value <= 3)
        {
            count[value]++;
        }
        ++numElements;
    }

    EXPECT_THAT(numElements, Eq(3));
    for (int i = 1; i <= 3; ++i)
    {
        EXPECT_THAT(count[i], Eq(1)); // expect each value exactly once
    }

    // remove an element and iterate over pool

    ASSERT_THAT(pool.get(index2), Ne(nullptr));
    pool.remove(index2);
    EXPECT_THAT(pool.size(), Eq(2U));

    // only 1 and 3 remain in the pool
    numElements = 0;
    for (auto& foo : pool)
    {
        auto value = *foo.m_data;
        if (value >= 1 && value <= 3)
        {
            count[value]++;
        }
        ++numElements;
    }

    EXPECT_THAT(numElements, Eq(2));
    EXPECT_THAT(count[1], Eq(2));
    EXPECT_THAT(count[2], Eq(1)); // count unchanged because element 2 was removed
    EXPECT_THAT(count[3], Eq(2));

    // remove remaining elements

    ASSERT_THAT(pool.get(index1), Ne(nullptr));
    pool.remove(index1);
    EXPECT_THAT(pool.size(), Eq(1U));


    // only 3 remains in the pool
    numElements = 0;
    for (auto& foo : pool)
    {
        auto value = *foo.m_data;
        if (value >= 1 && value <= 3)
        {
            count[value]++;
        }
        ++numElements;
    }

    EXPECT_THAT(numElements, Eq(1));
    EXPECT_THAT(count[1], Eq(2)); // count unchanged because element 1 was removed
    EXPECT_THAT(count[2], Eq(1)); // count unchanged because element 2 was removed
    EXPECT_THAT(count[3], Eq(3));

    ASSERT_THAT(pool.get(index3), Ne(nullptr));
    pool.remove(index3);
    EXPECT_THAT(pool.size(), Eq(0U));

    // pool is empty
    numElements = 0;
    for (auto& foo : pool)
    {
        auto value = *foo.m_data;
        if (value >= 1 && value <= 3)
        {
            count[value]++;
        }
        ++numElements;
    }

    EXPECT_THAT(numElements, Eq(0));
    // all counts unchanged
    EXPECT_THAT(count[1], Eq(2));
    EXPECT_THAT(count[2], Eq(1));
    EXPECT_THAT(count[3], Eq(3));

    // empty pool, begin equals end
    EXPECT_THAT(pool.begin(), Eq(pool.end()));
}
} // namespace

#endif
