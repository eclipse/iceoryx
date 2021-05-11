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
#ifndef IOX_POSH_MEPOO_MEPOO_SEGMENT_HPP
#define IOX_POSH_MEPOO_MEPOO_SEGMENT_HPP

#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/internal/posix_wrapper/access_control.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_utils/internal/posix_wrapper/shared_memory_object/allocator.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

namespace iox
{
namespace mepoo
{
template <typename SharedMemoryObjectType = posix::SharedMemoryObject, typename MemoryManagerType = MemoryManager>
class MePooSegment
{
  public:
    MePooSegment(const MePooConfig& mempoolConfig,
                 posix::Allocator& managementAllocator,
                 const posix::PosixGroup& readerGroup,
                 const posix::PosixGroup& writerGroup,
                 const iox::mepoo::MemoryInfo& memoryInfo = iox::mepoo::MemoryInfo()) noexcept;

    posix::PosixGroup getWriterGroup() const noexcept;
    posix::PosixGroup getReaderGroup() const noexcept;
    const SharedMemoryObjectType& getSharedMemoryObject() const noexcept;
    MemoryManagerType& getMemoryManager() noexcept;

    uint64_t getSegmentId() const noexcept;

  protected:
    SharedMemoryObjectType createSharedMemoryObject(const MePooConfig& mempoolConfig,
                                                    const posix::PosixGroup& writerGroup) noexcept;

  protected:
    SharedMemoryObjectType m_sharedMemoryObject;
    MemoryManagerType m_memoryManager;
    posix::PosixGroup m_readerGroup;
    posix::PosixGroup m_writerGroup;
    uint64_t m_segmentId;
    iox::mepoo::MemoryInfo m_memoryInfo;

  private:
    void setSegmentId(const uint64_t segmentId) noexcept;
};
} // namespace mepoo
} // namespace iox

#include "iceoryx_posh/internal/mepoo/mepoo_segment.inl"

#endif // IOX_POSH_MEPOO_MEPOO_SEGMENT_HPP
