// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_DATA_INL
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_DATA_INL

namespace iox
{
namespace popo
{
/// @todo Add min() without reference to iox::algorithm?! C++11 needs a declaration for constexpr!
///       Ex.: constexpr uint32_t DefaultChunkDistributorConfig::MAX_HISTORY_CAPACITY;
///       This wouldn't be an issue in C++17.
template <typename T>
constexpr T min(const T left, const T right)
{
    return (left < right) ? left : right;
}

template <typename ChunkDistributorDataProperties, typename LockingPolicy, typename ChunkQueuePusherType>
inline ChunkDistributorData<ChunkDistributorDataProperties, LockingPolicy, ChunkQueuePusherType>::ChunkDistributorData(
    const SubscriberTooSlowPolicy policy, const uint64_t historyCapacity) noexcept
    : LockingPolicy()
    , m_historyCapacity(min(historyCapacity, ChunkDistributorDataProperties_t::MAX_HISTORY_CAPACITY))
    , m_subscriberTooSlowPolicy(policy)
{
    if (m_historyCapacity != historyCapacity)
    {
        LogWarn() << "Chunk history too large, reducing from " << historyCapacity << " to "
                  << ChunkDistributorDataProperties_t::MAX_HISTORY_CAPACITY;
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_DISTRIBUTOR_DATA_INL
