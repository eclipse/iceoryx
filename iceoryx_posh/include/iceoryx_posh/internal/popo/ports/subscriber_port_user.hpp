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

#ifndef IOX_POPO_SUBSCRIBER_PORT_USER_HPP_
#define IOX_POPO_SUBSCRIBER_PORT_USER_HPP_

#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_data.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

namespace iox
{
namespace popo
{
/// @brief The SubscriberPortUser provides the API for accessing a subscriber port from the user side. The subscriber
/// port is divided in the parts SubscriberPortData, SubscriberPortUser and different classes for RouDi side access.
/// The SubscriberPortUser uses the functionality of a ChunkReceiver for receiving shared memory chunks. Additionally it
/// provides the subscribe / unsubscribe API which controls whether the subscriber ports shall try to subscribe to
/// matching publisher ports
class SubscriberPortUser : public BasePort
{
  public:
    using MemberType_t = SubscriberPortData;

    explicit SubscriberPortUser(cxx::not_null<MemberType_t* const> subscriberPortDataPtr) noexcept;

    SubscriberPortUser(const SubscriberPortUser& other) = delete;
    SubscriberPortUser& operator=(const SubscriberPortUser&) = delete;
    SubscriberPortUser(SubscriberPortUser&& rhs) = default;
    SubscriberPortUser& operator=(SubscriberPortUser&& rhs) = default;
    ~SubscriberPortUser() = default;

    /// @brief try to subscribe to all matching publishers
    void subscribe() noexcept;

    /// @brief unsubscribe from publishers, if there are any to which we are currently subscribed
    void unsubscribe() noexcept;

    /// @brief get the current subscription state. Caution: There can be delays between calling subscribe and a change
    /// in the subscription state. The subscription state can also change without user interaction if publishers come
    /// and go
    /// @return SubscribeState
    SubscribeState getSubscriptionState() const noexcept;

    /// @brief Tries to get the next chunk from the queue. If there is a new one, the ChunkHeader of the oldest chunk in
    /// the queue is returned (FiFo queue)
    /// @return New chunk header, ChunkReceiveResult on error
    /// or if there are no new chunks in the underlying queue
    cxx::expected<const mepoo::ChunkHeader*, ChunkReceiveResult> tryGetChunk() noexcept;

    /// @brief Release a chunk that was obtained with tryGetChunk
    /// @param[in] chunkHeader, pointer to the ChunkHeader to release
    void releaseChunk(const mepoo::ChunkHeader* const chunkHeader) noexcept;

    /// @brief Release all the chunks that are currently queued up.
    void releaseQueuedChunks() noexcept;

    /// @brief check if there are chunks in the queue
    /// @return if there are chunks in the queue return true, otherwise false
    bool hasNewChunks() const noexcept;

    /// @brief check if there was a queue overflow since the last call of hasLostChunksSinceLastCall
    /// @return true if the underlying queue overflowed since last call of this method, otherwise false
    bool hasLostChunksSinceLastCall() noexcept;

    /// @brief attach a condition variable (via its pointer) to subscriber
    void setConditionVariable(ConditionVariableData& conditionVariableData, const uint64_t notificationIndex) noexcept;

    /// @brief detach a condition variable from subscriber
    void unsetConditionVariable() noexcept;

    /// @brief check if there's a condition variable attached
    /// @return true if a condition variable attached, otherwise false
    bool isConditionVariableSet() noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    ChunkReceiver<SubscriberPortData::ChunkReceiverData_t> m_chunkReceiver;
};

} // namespace popo
} // namespace iox


#endif
