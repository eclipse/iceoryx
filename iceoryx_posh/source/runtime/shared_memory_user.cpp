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

#include "iceoryx_posh/internal/runtime/shared_memory_user.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/internal/mepoo/segment_manager.hpp"
#include "iceoryx_utils/cxx/convert.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

namespace iox
{
namespace runtime
{
SharedMemoryUser::SharedMemoryUser(const bool doMapSharedMemoryIntoThread,
                                   const size_t topicSize,
                                   const uint64_t segmentId,
                                   const rp::BaseRelativePointer::offset_t segmentManagerAddressOffset)
{
    if (doMapSharedMemoryIntoThread)
    {
        // create and map the already existing shared memory region
        posix::SharedMemoryObject::create(roudi::SHM_NAME,
                                          topicSize,
                                          posix::AccessMode::READ_WRITE,
                                          posix::OwnerShip::OPEN_EXISTING,
                                          posix::SharedMemoryObject::NO_ADDRESS_HINT)
            .and_then([this, segmentId, segmentManagerAddressOffset](auto& sharedMemoryObject) {
                rp::BaseRelativePointer::registerPtr(
                    segmentId, sharedMemoryObject.getBaseAddress(), sharedMemoryObject.getSizeInBytes());
                LogDebug() << "Application registered management segment "
                           << iox::log::HexFormat(reinterpret_cast<uint64_t>(sharedMemoryObject.getBaseAddress()))
                           << " with size " << sharedMemoryObject.getSizeInBytes() << " to id " << segmentId;

                this->openDataSegments(segmentId, segmentManagerAddressOffset);

                m_shmObject.emplace(std::move(sharedMemoryObject));
            })
            .or_else([](auto&) { errorHandler(Error::kPOSH__SHM_APP_MAPP_ERR); });
    }
}

void SharedMemoryUser::openDataSegments(const uint64_t segmentId,
                                        const rp::BaseRelativePointer::offset_t segmentManagerAddressOffset) noexcept
{
    auto ptr = rp::BaseRelativePointer::getPtr(segmentId, segmentManagerAddressOffset);
    auto segmentManager = reinterpret_cast<mepoo::SegmentManager<>*>(ptr);

    auto segmentMapping = segmentManager->getSegmentMappings(posix::PosixUser::getUserOfCurrentProcess());
    for (const auto& segment : segmentMapping)
    {
        auto accessMode = segment.m_isWritable ? posix::AccessMode::READ_WRITE : posix::AccessMode::READ_ONLY;
        posix::SharedMemoryObject::create(segment.m_sharedMemoryName,
                                          segment.m_size,
                                          accessMode,
                                          posix::OwnerShip::OPEN_EXISTING,
                                          posix::SharedMemoryObject::NO_ADDRESS_HINT)
            .and_then([this, &segment](auto& sharedMemoryObject) {
                if (static_cast<uint32_t>(m_dataShmObjects.size()) >= MAX_SHM_SEGMENTS)
                {
                    errorHandler(Error::kPOSH__SHM_APP_SEGMENT_COUNT_OVERFLOW);
                }

                rp::BaseRelativePointer::registerPtr(
                    segment.m_segmentId, sharedMemoryObject.getBaseAddress(), sharedMemoryObject.getSizeInBytes());

                LogDebug() << "Application registered payload data segment "
                           << iox::log::HexFormat(reinterpret_cast<uint64_t>(sharedMemoryObject.getBaseAddress()))
                           << " with size " << sharedMemoryObject.getSizeInBytes() << " to id " << segment.m_segmentId;

                m_dataShmObjects.emplace_back(std::move(sharedMemoryObject));
            })
            .or_else([](auto&) { errorHandler(Error::kPOSH__SHM_APP_SEGMENT_MAPP_ERR); });
    }
}
} // namespace runtime
} // namespace iox
