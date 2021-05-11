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
#ifndef IOX_POSH_MOCKS_ROUDI_MEMORY_PROVIDER_MOCK_HPP
#define IOX_POSH_MOCKS_ROUDI_MEMORY_PROVIDER_MOCK_HPP

#include "test.hpp"

#include "iceoryx_posh/roudi/memory/memory_block.hpp"

#include "iceoryx_posh/roudi/memory/memory_provider.hpp"

#if defined(QNX) || defined(QNX__) || defined(__QNX__)
#include <malloc.h>
#endif

class MemoryProviderTestImpl : public iox::roudi::MemoryProvider
{
  public:
    ~MemoryProviderTestImpl()
    {
        if (isAvailable())
        {
            EXPECT_FALSE(destroy().has_error());
        }
    }

    iox::cxx::expected<void*, iox::roudi::MemoryProviderError> createMemory(const uint64_t size,
                                                                            const uint64_t alignment) noexcept override
    {
        if (m_mockCallsEnabled)
        {
            createMemoryMock(size, alignment);
        }

        dummyMemory = static_cast<uint8_t*>(iox::cxx::alignedAlloc(alignment, size));
        return iox::cxx::success<void*>(dummyMemory);
    }
    MOCK_METHOD2(createMemoryMock, void(uint64_t, uint64_t));

    iox::cxx::expected<iox::roudi::MemoryProviderError> destroyMemory() noexcept override
    {
        if (m_mockCallsEnabled)
        {
            destroyMemoryMock();
        }

        iox::cxx::alignedFree(dummyMemory);
        dummyMemory = nullptr;

        return iox::cxx::success<void>();
    }
    MOCK_METHOD0(destroyMemoryMock, void());

    uint8_t* dummyMemory{nullptr};

  protected:
    bool m_mockCallsEnabled{false};
};

class MemoryProviderMock final : public MemoryProviderTestImpl
{
  public:
    MemoryProviderMock()
        : MemoryProviderTestImpl()
    {
        m_mockCallsEnabled = true;
    }
};

#endif // IOX_POSH_MOCKS_ROUDI_MEMORY_PROVIDER_MOCK_HPP
