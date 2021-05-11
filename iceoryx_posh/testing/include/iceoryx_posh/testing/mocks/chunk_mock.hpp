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
#ifndef IOX_POSH_MOCKS_CHUNK_MOCK_HPP
#define IOX_POSH_MOCKS_CHUNK_MOCK_HPP

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"

#include <cstdlib>
#include <cstring>

#if defined(QNX) || defined(QNX__) || defined(__QNX__)
#include <malloc.h>
#endif

template <typename Topic, typename UserHeader = iox::mepoo::NoUserHeader>
class ChunkMock
{
  public:
    ChunkMock()
    {
        const uint32_t userPayloadSize = sizeof(Topic);
        const uint32_t userPayloadAlignment = alignof(Topic);
        const uint32_t userHeaderSize =
            std::is_same<UserHeader, iox::mepoo::NoUserHeader>::value ? 0U : sizeof(UserHeader);
        const uint32_t userHeaderAlignment = alignof(UserHeader);

        auto chunkSettingsResult = iox::mepoo::ChunkSettings::create(
            userPayloadSize, userPayloadAlignment, userHeaderSize, userHeaderAlignment);

        iox::cxx::Ensures(!chunkSettingsResult.has_error() && "Invalid parameter for ChunkMock");
        auto& chunkSettings = chunkSettingsResult.value();
        auto chunkSize = chunkSettings.requiredChunkSize();

        m_rawMemory = static_cast<uint8_t*>(iox::cxx::alignedAlloc(alignof(iox::mepoo::ChunkHeader), chunkSize));
        assert(m_rawMemory != nullptr && "Could not get aligned memory");
        memset(m_rawMemory, 0xFF, chunkSize);


        m_chunkHeader = new (m_rawMemory) iox::mepoo::ChunkHeader(chunkSize, chunkSettings);
        m_topic = static_cast<Topic*>(m_chunkHeader->userPayload());
    }
    ~ChunkMock()
    {
        if (m_chunkHeader != nullptr)
        {
            m_chunkHeader->~ChunkHeader();
        }
        if (m_rawMemory != nullptr)
        {
            iox::cxx::alignedFree(m_rawMemory);
            m_rawMemory = nullptr;
        }
    }

    iox::mepoo::ChunkHeader* chunkHeader()
    {
        return m_chunkHeader;
    }

    const iox::mepoo::ChunkHeader* chunkHeader() const
    {
        return m_chunkHeader;
    }

    UserHeader* userHeader()
    {
        return static_cast<UserHeader*>(m_chunkHeader->userHeader());
    }

    const UserHeader* userHeader() const
    {
        return const_cast<ChunkMock*>(this)->userHeader();
    }

    Topic* sample()
    {
        return m_topic;
    }

    const Topic* sample() const
    {
        return m_topic;
    }

    ChunkMock(const ChunkMock&) = delete;
    ChunkMock(ChunkMock&&) = delete;
    ChunkMock& operator=(const ChunkMock&) = delete;
    ChunkMock& operator=(ChunkMock&&) = delete;

  private:
    uint8_t* m_rawMemory{nullptr};
    iox::mepoo::ChunkHeader* m_chunkHeader = nullptr;
    Topic* m_topic = nullptr;
};

#endif // IOX_POSH_MOCKS_CHUNK_MOCK_HPP
