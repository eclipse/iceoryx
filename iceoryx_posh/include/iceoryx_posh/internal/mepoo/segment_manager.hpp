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
#ifndef IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP
#define IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/mepoo_segment.hpp"
#include "iceoryx_posh/mepoo/segment_config.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

namespace iox
{
namespace roudi
{
template <typename MemoryManager, typename SegmentManager, typename PublisherPort>
class MemPoolIntrospection;
}

namespace mepoo
{
template <typename SegmentType = MePooSegment<>>
class SegmentManager
{
  public:
    SegmentManager(const SegmentConfig& segmentConfig, posix::Allocator* managementAllocator) noexcept;
    ~SegmentManager() noexcept = default;

    SegmentManager(const SegmentManager& rhs) = delete;
    SegmentManager(SegmentManager&& rhs) = delete;

    SegmentManager& operator=(const SegmentManager& rhs) = delete;
    SegmentManager& operator=(SegmentManager&& rhs) = delete;

    struct SegmentMapping
    {
      public:
        SegmentMapping(const ShmName_t& sharedMemoryName,
                       void* startAddress,
                       uint64_t size,
                       bool isWritable,
                       uint64_t segmentId,
                       const iox::mepoo::MemoryInfo& memoryInfo = iox::mepoo::MemoryInfo()) noexcept
            : m_sharedMemoryName(sharedMemoryName)
            , m_startAddress(startAddress)
            , m_size(size)
            , m_isWritable(isWritable)
            , m_segmentId(segmentId)
            , m_memoryInfo(memoryInfo)

        {
        }

        ShmName_t m_sharedMemoryName{""};
        void* m_startAddress{nullptr};
        uint64_t m_size{0};
        bool m_isWritable{false};
        uint64_t m_segmentId{0};
        iox::mepoo::MemoryInfo m_memoryInfo; // we can specify additional info about a segments memory here
    };

    struct SegmentUserInformation
    {
        cxx::optional<std::reference_wrapper<MemoryManager>> m_memoryManager;
        uint64_t m_segmentID;
    };

    using SegmentMappingContainer = cxx::vector<SegmentMapping, MAX_SHM_SEGMENTS>;

    SegmentMappingContainer getSegmentMappings(const posix::PosixUser& user) noexcept;
    SegmentUserInformation getSegmentInformationWithWriteAccessForUser(const posix::PosixUser& user) noexcept;

    static uint64_t requiredManagementMemorySize(const SegmentConfig& config) noexcept;
    static uint64_t requiredChunkMemorySize(const SegmentConfig& config) noexcept;
    static uint64_t requiredFullMemorySize(const SegmentConfig& config) noexcept;

  private:
    void createSegment(const SegmentConfig::SegmentEntry& segmentEntry) noexcept;

  private:
    template <typename MemoryManger, typename SegmentManager, typename PublisherPort>
    friend class roudi::MemPoolIntrospection;

    posix::Allocator* m_managementAllocator;
    cxx::vector<SegmentType, MAX_SHM_SEGMENTS> m_segmentContainer;
    bool m_createInterfaceEnabled{true};
};


} // namespace mepoo
} // namespace iox

#include "iceoryx_posh/internal/mepoo/segment_manager.inl"

#endif // IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP
