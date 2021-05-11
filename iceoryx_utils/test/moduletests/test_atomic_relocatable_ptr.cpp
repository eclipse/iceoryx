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

#include "iceoryx_utils/internal/relocatable_pointer/atomic_relocatable_pointer.hpp"

#include "test.hpp"

#include <cstring>

namespace
{
using namespace ::testing;

using byte_t = std::uint8_t;

template <size_t n, size_t alignment>
class Memory
{
  public:
    Memory()
    {
        set(0);
    }

    void set(byte_t value = 0)
    {
        memset(buf, value, n);
    }

    void set(const Memory& memory)
    {
        memcpy(buf, memory.buf, n);
    }

    byte_t& operator[](size_t i)
    {
        return buf[i];
    }

    byte_t* ptr(size_t i)
    {
        return &buf[i];
    }

  private:
    alignas(alignment) byte_t buf[n];
};

class Foo
{
  public:
    void* self()
    {
        return this;
    }
};

template <typename T>
using Ptr = iox::rp::AtomicRelocatablePointer<T>;

class AtomicRelocatablePointer_test : public Test
{
  public:
    void SetUp() override
    {
        internal::CaptureStderr();
    }

    void TearDown() override
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};

TEST_F(AtomicRelocatablePointer_test, defaultConstructedPointerIsNull)
{
    Ptr<int> rp;
    EXPECT_EQ(rp, nullptr);
}

TEST_F(AtomicRelocatablePointer_test, constructedPointerPointsToData)
{
    int data = 38;
    Ptr<int> rp(&data);
    EXPECT_EQ(rp, &data);
    EXPECT_EQ(*rp, data);
}

TEST_F(AtomicRelocatablePointer_test, assignRawPointer)
{
    Ptr<int> rp;
    int data = 39;
    rp = &data;
    EXPECT_EQ(rp, &data);
    EXPECT_EQ(*rp, data);
}

TEST_F(AtomicRelocatablePointer_test, compareWithRawPointer)
{
    Ptr<int> rp;
    int data = 39;
    rp = &data;
    EXPECT_TRUE(rp == &data);
}

TEST_F(AtomicRelocatablePointer_test, resetToNull)
{
    Ptr<int> rp;
    int data = 40;
    rp = &data;
    rp = nullptr;
    EXPECT_TRUE(rp == nullptr);
}


TEST_F(AtomicRelocatablePointer_test, arrowOperator)
{
    Foo foo;
    Ptr<Foo> rp(&foo);
    EXPECT_EQ(rp->self(), foo.self());
}

TEST_F(AtomicRelocatablePointer_test, conversionOperator)
{
    Foo foo;
    Ptr<Foo> rp(&foo);
    Foo* p = rp;
    EXPECT_EQ(p->self(), foo.self());
}

TEST_F(AtomicRelocatablePointer_test, dereferenceOperator)
{
    Foo foo;
    Ptr<Foo> rp(&foo);
    EXPECT_EQ((*rp).self(), foo.self());
}

// create some memory with a relocatable pointer to some data in it
// copy the memory to another location and set the original memory to zero
// the relocatable pointer at the new location should point to the data at the
// copied location
TEST_F(AtomicRelocatablePointer_test, memoryRelocation)
{
    constexpr uint64_t ALIGNMENT_OF_PTR{alignof(Ptr<byte_t>)};
    constexpr size_t INDEX_OF_PTR{ALIGNMENT_OF_PTR};
    Memory<1024, ALIGNMENT_OF_PTR> memory;
    memory[1000] = 37;
    // EXPECT_EQ(memory[1000], 37);

    Ptr<byte_t>* rp = new (memory.ptr(INDEX_OF_PTR)) Ptr<byte_t>(memory.ptr(1000));

    // we have constructed a relocatable pointer at adress "INDEX_OF_PTR" in memory which points
    // to address at memory location 1000 which holds value 37
    EXPECT_EQ(*rp, memory.ptr(1000));
    EXPECT_EQ(**rp, 37);

    // copy this memory to a new destination which we then set to 0
    Memory<1024, ALIGNMENT_OF_PTR> dest;
    EXPECT_EQ(dest[1000], 0);
    dest.set(memory);
    memory.set(0);

    EXPECT_EQ(dest[1000], 37);
    EXPECT_EQ(memory[1000], 0);

    // reinterpret the memory where the relocatable was at destination, it now should point to
    // byte 1000 in dest which holds value 37 after memory was copied to dest
    rp = reinterpret_cast<Ptr<byte_t>*>(dest.ptr(INDEX_OF_PTR));
    EXPECT_EQ(*rp, dest.ptr(1000));
    EXPECT_EQ(**rp, 37);
}
} // namespace
