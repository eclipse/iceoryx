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

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/shared_pointer.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/base_relative_pointer.hpp"
#include "test.hpp"

using namespace ::testing;

using namespace iox::mepoo;

class SharedPointer_Test : public Test
{
  public:
    struct counter_t
    {
        uint64_t ctor{0};
        uint64_t dtor{0};
        uint64_t moveCtor{0};
        uint64_t copyCtor{0};
        uint64_t moveAssignment{0};
        uint64_t copyAssignment{0};
    };


    class TestClass
    {
      public:
        TestClass()
        {
            counter.ctor++;
        }

        TestClass(int a, int b)
            : a{a}
            , b{b}
        {
            counter.ctor++;
        }

        TestClass(const TestClass& v)
        {
            this->a = v.a;
            this->b = v.b;
            counter.copyCtor++;
        }

        TestClass(TestClass&& v)
        {
            this->a = v.a;
            this->b = v.b;
            counter.moveCtor++;
        }

        TestClass& operator=(const TestClass& v)
        {
            this->a = v.a;
            this->b = v.b;
            counter.copyAssignment++;
            return *this;
        }

        TestClass& operator=(TestClass&& v)
        {
            this->a = v.a;
            this->b = v.b;
            counter.moveAssignment++;
            return *this;
        }

        ~TestClass()
        {
            counter.dtor++;
        }

        void Increase()
        {
            a += 12;
            b += 819;
        }

        int a;
        int b;
        static counter_t counter;
    };

    void SetUp() override
    {
        iox::rp::BaseRelativePointer::registerPtr(memory, 4096);
    }
    void TearDown() override
    {
        iox::rp::BaseRelativePointer::unregisterAll();
    }

    ChunkManagement* GetChunkManagement(void* memoryChunk)
    {
        ChunkManagement* v = static_cast<ChunkManagement*>(chunkMgmtPool.getChunk());

        auto chunkSettingsResult = ChunkSettings::create(USER_PAYLOAD_SIZE, iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
        EXPECT_FALSE(chunkSettingsResult.has_error());
        if (chunkSettingsResult.has_error())
        {
            return nullptr;
        }
        auto& chunkSettings = chunkSettingsResult.value();

        ChunkHeader* chunkHeader = new (memoryChunk) ChunkHeader(mempool.getChunkSize(), chunkSettings);
        new (v) ChunkManagement{chunkHeader, &mempool, &chunkMgmtPool};
        return v;
    }

    int ResetCounter()
    {
        TestClass::counter = counter_t();
        return 0;
    }

    int resetCounter = ResetCounter();

    static constexpr uint32_t USER_PAYLOAD_SIZE{64U};

    char memory[4096U];
    iox::posix::Allocator allocator{memory, 4096U};
    MemPool mempool{sizeof(ChunkHeader) + USER_PAYLOAD_SIZE, 10U, allocator, allocator};
    MemPool chunkMgmtPool{64U, 10U, allocator, allocator};

    void* memoryChunk{mempool.getChunk()};
    ChunkManagement* chunkManagement = GetChunkManagement(memoryChunk);
    SharedChunk chunk{chunkManagement};

    void* memoryChunk2{mempool.getChunk()};
    ChunkManagement* chunkManagement2 = GetChunkManagement(memoryChunk2);
    SharedChunk chunk2{chunkManagement2};

    void* memoryChunk3{mempool.getChunk()};
    ChunkManagement* chunkManagement3 = GetChunkManagement(memoryChunk3);
    SharedChunk chunk3{chunkManagement3};

    void* memoryChunk4{mempool.getChunk()};
    ChunkManagement* chunkManagement4 = GetChunkManagement(memoryChunk4);
    SharedChunk chunk4{chunkManagement4};

    SharedPointer<int> sut = SharedPointer<int>::create(chunk, 42).value();
    SharedPointer<TestClass> sutComplex = SharedPointer<TestClass>::create(chunk2, 1337, 851).value();
};

SharedPointer_Test::counter_t SharedPointer_Test::TestClass::counter;

TEST_F(SharedPointer_Test, DefaultCTor)
{
    EXPECT_THAT(SharedPointer_Test::TestClass::counter.ctor, Eq(1));
}

TEST_F(SharedPointer_Test, ConstGetMethod)
{
    EXPECT_THAT(*const_cast<const SharedPointer<int>&>(sut).get(), Eq(42));
}

TEST_F(SharedPointer_Test, GetMethod)
{
    *sut.get() = 7781;
    EXPECT_THAT(*sut.get(), Eq(7781));
}

TEST_F(SharedPointer_Test, ConstArrowOperator)
{
    EXPECT_THAT(const_cast<const SharedPointer<TestClass>&>(sutComplex)->a, Eq(1337));
}

TEST_F(SharedPointer_Test, ArrowOperator)
{
    sutComplex->Increase();
    EXPECT_THAT(sutComplex->a, Eq(1349));
}

TEST_F(SharedPointer_Test, ConstStarOperator)
{
    EXPECT_THAT((*const_cast<const SharedPointer<TestClass>&>(sutComplex)).b, Eq(851));
}

TEST_F(SharedPointer_Test, StarOperator)
{
    (*sut)++;
    EXPECT_THAT(*sut, Eq(43));
}

TEST_F(SharedPointer_Test, CopyConstructor)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 313, 1313).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1
        {
            SharedPointer<TestClass> sut4(sut3);
            EXPECT_THAT(sut4->a, Eq(313));
            EXPECT_THAT(sut4->b, Eq(1313));

            EXPECT_THAT(TestClass::counter.ctor, Eq(2));
            EXPECT_THAT(TestClass::counter.dtor, Eq(0));
            EXPECT_THAT(TestClass::counter.moveCtor, Eq(0));
            EXPECT_THAT(TestClass::counter.copyCtor, Eq(0));
            EXPECT_THAT(TestClass::counter.moveAssignment, Eq(0));
            EXPECT_THAT(TestClass::counter.copyAssignment, Eq(0));
        }
        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, MoveConstructor)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 15, 25).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1
        {
            SharedPointer<TestClass> sut4(std::move(sut3));
            EXPECT_THAT(sut4->a, Eq(15));
            EXPECT_THAT(sut4->b, Eq(25));

            EXPECT_THAT(TestClass::counter.ctor, Eq(2));
            EXPECT_THAT(TestClass::counter.dtor, Eq(0));
            EXPECT_THAT(TestClass::counter.moveCtor, Eq(0));
            EXPECT_THAT(TestClass::counter.copyCtor, Eq(0));
            EXPECT_THAT(TestClass::counter.moveAssignment, Eq(0));
            EXPECT_THAT(TestClass::counter.copyAssignment, Eq(0));
        }
        EXPECT_THAT(TestClass::counter.dtor, Eq(1));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, CopyAssignment)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>::create(chunk4, 3, 4).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(3));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut4 = sut3;
        EXPECT_THAT(TestClass::counter.dtor, Eq(1));
        EXPECT_THAT(sut4->a, Eq(1));
        EXPECT_THAT(sut4->b, Eq(2));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(2));
}

TEST_F(SharedPointer_Test, MoveAssignment)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>::create(chunk4, 3, 4).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(3));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut4 = std::move(sut3);
        EXPECT_THAT(TestClass::counter.dtor, Eq(1));

        EXPECT_THAT(sut4->a, Eq(1));
        EXPECT_THAT(sut4->b, Eq(2));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(2));
}

TEST_F(SharedPointer_Test, CopyToEmpty)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut4 = sut3;
        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        EXPECT_THAT(sut4->a, Eq(1));
        EXPECT_THAT(sut4->b, Eq(2));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, CopyFromEmpty)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut3 = sut4;
        EXPECT_THAT(TestClass::counter.dtor, Eq(1));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, MoveToEmpty)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut4 = std::move(sut3);
        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        EXPECT_THAT(sut4->a, Eq(1));
        EXPECT_THAT(sut4->b, Eq(2));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, MoveFromEmpty)
{
    {
        auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2)); // sutComplex is 1

        auto sut4 = SharedPointer<TestClass>();
        EXPECT_THAT(TestClass::counter.ctor, Eq(2));

        EXPECT_THAT(TestClass::counter.dtor, Eq(0));
        sut3 = std::move(sut4);
        EXPECT_THAT(TestClass::counter.dtor, Eq(1));
    }
    EXPECT_THAT(TestClass::counter.dtor, Eq(1));
}

TEST_F(SharedPointer_Test, DefaultCTorProvidesInvalidSharedPointer)
{
    EXPECT_THAT(static_cast<bool>(SharedPointer<int>()), Eq(false));
}

TEST_F(SharedPointer_Test, SharedPointerWithContentIsValid)
{
    auto sut3 = SharedPointer<TestClass>::create(chunk3, 1, 2).value();
    EXPECT_THAT(static_cast<bool>(sut3), Eq(true));
}
