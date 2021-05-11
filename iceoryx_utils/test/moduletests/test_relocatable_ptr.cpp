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

#include "iceoryx_utils/internal/relocatable_pointer/relocatable_pointer.hpp"

#include "test.hpp"

#include <cstring>

namespace
{
using namespace ::testing;
using namespace iox::rp;

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

class RelocatablePointer_test : public Test
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
    int m_fileDescriptor;
};

//@todo: extend test cases, but the relocatable is not used currently so this is postponed
// the relocatable will be useful later, so we need to keep it

// central use case of relocatable pointers
// it is tested that moving (relocating via memcopy) the relocatable pointer causes the pointer to point to the correct
// location relative to itself
// this is similar to remap the shared memory (including the pointer and pointee) to a different location
// while keeping their relative offset to each other the same
TEST_F(RelocatablePointer_test, relocation)
{
    constexpr size_t BLOCK_SIZE = 1024;
    // simulate 3 consecutive memory blocks on the stack
    Memory<BLOCK_SIZE> block1;
    Memory<BLOCK_SIZE> block2;

    uint8_t* base1 = block1[0];
    uint8_t* base2 = block2[0];

    int offset = BLOCK_SIZE / 2;
    auto adr1 = reinterpret_cast<int*>(base1 + offset);
    auto adr2 = reinterpret_cast<int*>(base2 + offset);

    // int write
    *adr1 = 12;
    *adr2 = 21;

    EXPECT_EQ(*adr1, 12);
    EXPECT_EQ(*adr2, 21);

    // placement new at base adress (we have enough memory to do so)
    RelocatablePointer<int>* rp = new (base1) RelocatablePointer<int>(adr1);
    auto& rp1 = *rp;
    EXPECT_EQ(*rp1, 12);

    // copy the relocatable from base address in block1 to base address in block2
    std::memcpy(base2, base1, sizeof(rp1));

    // the value in block 2 is still unchanged
    EXPECT_EQ(*adr2, 21);

    // read the relocatable pointer in block 2 at the base address
    auto& rp2 = *(reinterpret_cast<RelocatablePointer<int>*>(base2));

    // it now points to the value in block2 (since its measured relative to this, which is now at base2 for rp2)
    EXPECT_EQ(*rp2, 21);

    // copy the entire block1 to block2, including the relocatable pointer at the base adress and
    // the data at adr1 (so at adr1 the value changed from 21 to 12)
    std::memcpy(base2, base1, BLOCK_SIZE);

    // now value the value also changed at adr2 ...
    EXPECT_EQ(*adr2, 12);

    // and dereferencing the relocatable pointer yields this value
    EXPECT_EQ(*rp2, 12);

    // was created via placement new, need to manually call the dtor (does not really matter here since the object
    // only has primitive members and the test ends here)
    rp->~RelocatablePointer<int>();
}
} // namespace
