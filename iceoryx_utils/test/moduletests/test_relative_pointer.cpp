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

#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"
#include "iceoryx_utils/platform/fcntl.hpp"
#include "iceoryx_utils/platform/mman.hpp"
#include "iceoryx_utils/platform/stat.hpp"
#include "iceoryx_utils/platform/unistd.hpp"

#include "test.hpp"

#include <cstdint>
#include <cstring>

namespace
{
using namespace ::testing;
using namespace iox::rp;

constexpr mode_t ShmMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
constexpr int OFlags = O_CREAT | O_RDWR;
constexpr size_t ShmSize = 4096 * 32;

struct Data
{
    Data(uint32_t i, uint32_t j)
        : Data1(i)
        , Data2(j)
    {
    }
    uint32_t Data1 = 27;
    uint32_t Data2 = 72;
};

template <size_t n>
class Memory
{
  public:
    Memory()
    {
    }

    std::uint8_t* operator[](int i)
    {
        return &buf[i];
    }

    std::uint8_t buf[n];
};

class base_relative_ptr_test : public Test
{
  public:
    void SetUp() override
    {
        auto shmOpenC = iox::cxx::makeSmartC(
            shm_open, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, "TestShm", OFlags, ShmMode);

        if (shmOpenC.hasErrors())
        {
            std::cerr << "ftruncate failed with error: " << shmOpenC.getErrorString();
            exit(EXIT_FAILURE);
        }

        m_fileDescriptor = shmOpenC.getReturnValue();

        auto trunC = iox::cxx::makeSmartC(
            ftruncate, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_fileDescriptor, ShmSize);

        if (trunC.hasErrors())
        {
            std::cerr << "ftruncate failed with error: " << trunC.getErrorString();
            exit(EXIT_FAILURE);
        }


        internal::CaptureStderr();
    }

    void TearDown() override
    {
        auto shmUnlinkC =
            iox::cxx::makeSmartC(shm_unlink, iox::cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, "TestShm");

        if (shmUnlinkC.hasErrors())
        {
            std::cerr << "ftruncate failed with error: " << shmUnlinkC.getErrorString();
            exit(EXIT_FAILURE);
        }

        BaseRelativePointer::unregisterAll();
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
    uint32_t m_fileDescriptor{0U};
};

template <typename T>
class relativeptrtests : public base_relative_ptr_test
{
};
class MemMap
{
  public:
    MemMap(int fileDescriptor)
    {
        m_mapAddr = mmap(nullptr, ShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    }
    ~MemMap()
    {
        munmap(m_mapAddr, ShmSize);
        m_mapAddr = nullptr;
    }

    void* getMappedAddress()
    {
        return m_mapAddr;
    }

  private:
    void* m_mapAddr = nullptr;
};

typedef testing::Types<uint8_t, int8_t, double> Types;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(relativeptrtests, Types);
#pragma GCC diagnostic pop


TYPED_TEST(relativeptrtests, ConstrTests)
{
    MemMap memMap(this->m_fileDescriptor);
    MemMap memMap1(this->m_fileDescriptor);

    EXPECT_EQ(BaseRelativePointer::registerPtr(1, memMap.getMappedAddress(), ShmSize), true);
    EXPECT_EQ(BaseRelativePointer::registerPtr(2, memMap1.getMappedAddress(), ShmSize), true);

    {
        auto offset = ShmSize / 2;
        void* adr = static_cast<uint8_t*>(memMap.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp(memMap.getMappedAddress());
        EXPECT_EQ(rp.getOffset(), 0);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize / 2;
        void* adr = static_cast<uint8_t*>(memMap.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp(adr);
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize - 1;
        void* adr = static_cast<uint8_t*>(memMap.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp(adr);
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp(memMap1.getMappedAddress());
        EXPECT_EQ(rp.getOffset(), 0);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize / 2;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp(adr);
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize - 1;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp(adr);
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp(nullptr);
        EXPECT_EQ(rp, nullptr);
    }

    {
        auto offset = ShmSize + 1;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp(adr);
        EXPECT_NE(rp, nullptr);
    }
}

TYPED_TEST(relativeptrtests, AssignmentOperatorTests)
{
    MemMap memMap(this->m_fileDescriptor);
    MemMap memMap1(this->m_fileDescriptor);

    EXPECT_EQ(BaseRelativePointer::registerPtr(1, memMap.getMappedAddress(), ShmSize), true);
    EXPECT_EQ(BaseRelativePointer::registerPtr(2, memMap1.getMappedAddress(), ShmSize), true);

    {
        RelativePointer<TypeParam> rp;
        rp = memMap.getMappedAddress();
        EXPECT_EQ(rp.getOffset(), 0);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp;
        rp = memMap.getMappedAddress();
        BaseRelativePointer basePointer(rp);
        RelativePointer<TypeParam> recovered(basePointer);

        EXPECT_EQ(rp, recovered);
        EXPECT_EQ(rp.getOffset(), recovered.getOffset());
        EXPECT_EQ(rp.getId(), recovered.getId());

        recovered = basePointer;
        EXPECT_EQ(rp, recovered);
        EXPECT_EQ(rp.getOffset(), recovered.getOffset());
        EXPECT_EQ(rp.getId(), recovered.getId());
    }

    {
        auto offset = ShmSize / 2;
        void* adr = static_cast<uint8_t*>(memMap.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize - 1;
        void* adr = static_cast<uint8_t*>(memMap.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 1);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp;
        rp = memMap1.getMappedAddress();
        EXPECT_EQ(rp.getOffset(), 0);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize / 2;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        auto offset = ShmSize - 1;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_EQ(rp.getOffset(), offset);
        EXPECT_EQ(rp.getId(), 2);
        EXPECT_NE(rp, nullptr);
    }

    {
        RelativePointer<TypeParam> rp;
        rp = nullptr;
        EXPECT_EQ(rp, nullptr);
    }

    {
        auto offset = ShmSize + 1;
        void* adr = static_cast<uint8_t*>(memMap1.getMappedAddress()) + offset;
        RelativePointer<TypeParam> rp;
        rp = adr;
        EXPECT_NE(rp, nullptr);
    }
}

TYPED_TEST(relativeptrtests, IdAndOffset)
{
    MemMap memMap(this->m_fileDescriptor);
    void* basePtr1 = memMap.getMappedAddress();

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(rp1.getOffset(), reinterpret_cast<std::ptrdiff_t>(basePtr1));
    EXPECT_EQ(rp1.getId(), 1);

    int offset = ShmSize / 2;
    auto offsetAddr1 = reinterpret_cast<TypeParam*>(static_cast<uint8_t*>(memMap.getMappedAddress()) + offset);
    RelativePointer<TypeParam> rp2(offsetAddr1, 1);
    EXPECT_EQ(rp2.getOffset(), offset);
    EXPECT_EQ(rp2.getId(), 1);
    EXPECT_EQ(rp2.get(), offsetAddr1);
}

TYPED_TEST(relativeptrtests, getOffset)
{
    MemMap memMap(this->m_fileDescriptor);

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(BaseRelativePointer::getOffset(1, memMap.getMappedAddress()), 0);

    int offset = ShmSize / 2;
    auto offsetAddr1 = reinterpret_cast<TypeParam*>(static_cast<uint8_t*>(memMap.getMappedAddress()) + offset);
    RelativePointer<TypeParam> rp2(offsetAddr1, 1);
    EXPECT_EQ(BaseRelativePointer::getOffset(1, offsetAddr1), offset);
}

TYPED_TEST(relativeptrtests, getPtr)
{
    MemMap memMap(this->m_fileDescriptor);

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(BaseRelativePointer::getPtr(1, 0), memMap.getMappedAddress());

    int offset = ShmSize / 2;
    auto offsetAddr1 = reinterpret_cast<TypeParam*>(static_cast<uint8_t*>(memMap.getMappedAddress()) + offset);
    RelativePointer<TypeParam> rp2(offsetAddr1, 1);
    EXPECT_EQ(BaseRelativePointer::getPtr(1, offset), offsetAddr1);
}

TYPED_TEST(relativeptrtests, registerPtr)
{
    MemMap memMap(this->m_fileDescriptor);
    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);

    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), false);
    EXPECT_EQ(rp1.unregisterPtr(1), true);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
}

TYPED_TEST(relativeptrtests, unRegisterPointerTest_Valid)
{
    MemMap memMap(this->m_fileDescriptor);
    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);

    rp1.registerPtr(1, memMap.getMappedAddress());
    EXPECT_EQ(rp1.unregisterPtr(1), true);
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
}

TYPED_TEST(relativeptrtests, unregisterPointerAll)
{
    MemMap memMap(this->m_fileDescriptor);
    MemMap memMap1(this->m_fileDescriptor);

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    RelativePointer<TypeParam> rp2(memMap1.getMappedAddress(), 9999);

    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(rp2.registerPtr(9999, memMap1.getMappedAddress()), true);
    BaseRelativePointer::unregisterAll();
    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(rp2.registerPtr(9999, memMap1.getMappedAddress()), true);
}

TYPED_TEST(relativeptrtests, registerPtrWithId)
{
    MemMap memMap(this->m_fileDescriptor);
    MemMap memMap1(this->m_fileDescriptor);

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    RelativePointer<TypeParam> rp2(memMap1.getMappedAddress(), 10000);

    EXPECT_EQ(rp1.registerPtr(1, memMap.getMappedAddress()), true);
    EXPECT_EQ(rp2.registerPtr(10000, memMap1.getMappedAddress()), false);
}

TYPED_TEST(relativeptrtests, basePointerValid)
{
    MemMap memMap(this->m_fileDescriptor);
    void* basePtr1 = memMap.getMappedAddress();

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    EXPECT_EQ(rp1.getBasePtr(1), nullptr);
    rp1.registerPtr(1, memMap.getMappedAddress());
    EXPECT_EQ(basePtr1, rp1.getBasePtr(1));
}

TYPED_TEST(relativeptrtests, assignmentOperator)
{
    MemMap memMap(this->m_fileDescriptor);

    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);
    RelativePointer<TypeParam> rp2 = rp1;

    EXPECT_EQ(rp1.getBasePtr(), rp2.getBasePtr());
    EXPECT_EQ(rp1.getId(), rp2.getId());
    EXPECT_EQ(rp1.getOffset(), rp2.getOffset());
}

TYPED_TEST(relativeptrtests, pointerOperator)
{
    MemMap memMap(this->m_fileDescriptor);

    auto baseAddr = reinterpret_cast<TypeParam*>(memMap.getMappedAddress());
    *baseAddr = static_cast<TypeParam>(88);
    RelativePointer<TypeParam> rp1(memMap.getMappedAddress(), 1);

    EXPECT_EQ(*rp1, *baseAddr);
    *baseAddr = static_cast<TypeParam>(99);
    EXPECT_EQ(*rp1, *baseAddr);
}

/// central use case of the relative pointer:
/// it is tested that changing the (static) lookup table of a relative pointer causes existing
/// relative pointers point to changed locations relative to the new lookup table
TEST_F(base_relative_ptr_test, memoryRemapping)
{
    constexpr size_t BLOCK_SIZE = 1024;
    // simulate 3 consecutive memory blocks on the stack
    Memory<BLOCK_SIZE> block1;
    Memory<BLOCK_SIZE> block2;

    uint8_t* base1 = block1[0];
    uint8_t* base2 = block2[0];

    // uint8 write
    *base1 = 37U;
    *base2 = 73U;

    EXPECT_EQ(*base1, 37U);
    EXPECT_EQ(*base2, 73U);

    int offset = BLOCK_SIZE / 2;
    auto adr1 = reinterpret_cast<int*>(base1 + offset);
    auto adr2 = reinterpret_cast<int*>(base2 + offset);

    // int write
    *adr1 = 12;
    *adr2 = 21;

    EXPECT_EQ(*adr1, 12);
    EXPECT_EQ(*adr2, 21);

    EXPECT_EQ(BaseRelativePointer::registerPtr(1, base1), true);
    EXPECT_EQ(BaseRelativePointer::registerPtr(2, base2), true);

    {
        // the relative pointers point to base 1 and base 2l
        RelativePointer<uint8_t> rp1(base1, 1);
        RelativePointer<uint8_t> rp2(base2, 2);

        EXPECT_EQ(rp1.getId(), 1);
        EXPECT_EQ(rp2.getId(), 2);

        EXPECT_EQ(rp1.getOffset(), 0);
        EXPECT_EQ(rp2.getOffset(), 0);

        EXPECT_EQ(*rp1, 37U);
        EXPECT_EQ(*rp2, 73U);
    }

    {
        // now test with a type that is larger than 1 byte
        RelativePointer<int> rp1(adr1, 1);
        RelativePointer<int> rp2(adr2, 2);

        EXPECT_EQ(rp1.getId(), 1);
        EXPECT_EQ(rp2.getId(), 2);

        // relative to their respective memory block base adress both have the same offset
        EXPECT_EQ(rp1.getOffset(), offset);
        EXPECT_EQ(rp2.getOffset(), offset);

        //*** rp1 points to 12 and rp2 to 21
        EXPECT_EQ(*rp1, 12);
        EXPECT_EQ(*rp2, 21);

        // simulate a remapping, index 1 now refers to base 2 and vice versa ...
        EXPECT_EQ(BaseRelativePointer::unregisterPtr(1), true);
        EXPECT_EQ(BaseRelativePointer::unregisterPtr(2), true);

        EXPECT_EQ(BaseRelativePointer::registerPtr(1, base2), true);
        EXPECT_EQ(BaseRelativePointer::registerPtr(2, base1), true);

        // which, despite the relative pointer objects not having changed themselves,
        // leads to them referencing the respective other value now (compared to *** above)
        EXPECT_EQ(*rp1, 21);
        EXPECT_EQ(*rp2, 12);

        // this would also happen in another application where the static base pointer lookup table
        // is might differ from application to application
    }
}

TEST_F(base_relative_ptr_test, MemoryReMapping_SharedMemory)
{
    MemMap memMapWriter(this->m_fileDescriptor);
    MemMap memMapReader(this->m_fileDescriptor);

    Data* dataPointer1 = new (memMapWriter.getMappedAddress()) Data(12, 21);

    EXPECT_EQ(dataPointer1->Data1, reinterpret_cast<Data*>(memMapReader.getMappedAddress())->Data1);

    int offset = ShmSize / 2;
    auto offsetAddr1 = reinterpret_cast<int*>(static_cast<uint8_t*>(memMapWriter.getMappedAddress()) + offset);
    auto offsetAddr2 = reinterpret_cast<int*>(static_cast<uint8_t*>(memMapReader.getMappedAddress()) + offset);
    *offsetAddr1 = 37;

    EXPECT_EQ(*offsetAddr2, *offsetAddr1);

    EXPECT_EQ(BaseRelativePointer::registerPtr(1, reinterpret_cast<void*>(memMapWriter.getMappedAddress())), true);
    {
        RelativePointer<int> rp1(offsetAddr1, 1);

        EXPECT_EQ(rp1.getId(), 1);
        EXPECT_EQ(rp1.getOffset(), offset);
        EXPECT_EQ(*rp1, 37);
        EXPECT_EQ(rp1.get(), offsetAddr1);
    }
    EXPECT_EQ(BaseRelativePointer::unregisterPtr(1), true);

    EXPECT_EQ(BaseRelativePointer::registerPtr(1, reinterpret_cast<void*>(memMapReader.getMappedAddress())), true);
    {
        RelativePointer<int> rp1(offsetAddr2, 1);

        EXPECT_EQ(rp1.getId(), 1);
        EXPECT_EQ(rp1.getOffset(), offset);
        EXPECT_EQ(*rp1, 37);
        EXPECT_EQ(rp1.get(), offsetAddr2);
    }
    EXPECT_EQ(BaseRelativePointer::unregisterPtr(1), true);
}

TEST_F(base_relative_ptr_test, compileTest)
{
    // No functional test. Tests if code compiles
    RelativePointer<void> p1;
    RelativePointer<const void> p2;
}

} // namespace
