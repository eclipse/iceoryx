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

#ifndef IOX_POPO_SUBSCRIBER_PORT_ROUDI_HPP_
#define IOX_POPO_SUBSCRIBER_PORT_ROUDI_HPP_

#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port.hpp"
#include "iceoryx_posh/internal/popo/ports/subscriber_port_data.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

namespace iox
{
namespace popo
{
/// @brief The SubscriberPortRouDi provides the API for accessing a subscriber port from the RouDi middleware daemon
/// side. The subscriber port is divided in the sevaral parts like SubscriberPortData, SubscriberPortRouDi and
/// SubscriberPortUser. The SubscriberPortRouDi provides service discovery functionality that is based on CaPro
/// messages. With this API the dynamic connections between publisher and subscriber ports can be established
class SubscriberPortRouDi : public BasePort
{
  public:
    using MemberType_t = SubscriberPortData;

    explicit SubscriberPortRouDi(cxx::not_null<MemberType_t* const> subscriberPortDataPtr) noexcept;

    SubscriberPortRouDi(const SubscriberPortRouDi& other) = delete;
    SubscriberPortRouDi& operator=(const SubscriberPortRouDi&) = delete;
    SubscriberPortRouDi(SubscriberPortRouDi&& rhs) = default;
    SubscriberPortRouDi& operator=(SubscriberPortRouDi&& rhs) = default;
    virtual ~SubscriberPortRouDi() = default;

    /// @brief Returns behaviour in case of a full delivery queue
    /// @return QueueFullPolicy What happens if the delivery queue is full
    QueueFullPolicy getQueueFullPolicy() const noexcept;

    /// @brief get an optional CaPro message that requests changes to the subscription state of the subscriber
    /// @return CaPro message with new subscription requet, empty optional if no state change
    virtual cxx::optional<capro::CaproMessage> tryGetCaProMessage() noexcept = 0;

    /// @brief dispatch a CaPro message to the subscriber for processing
    /// @param[in] caProMessage to process
    /// @return CaPro message with an immediate response the provided CaPro message, empty optional if no response
    virtual cxx::optional<capro::CaproMessage>
    dispatchCaProMessageAndGetPossibleResponse(const capro::CaproMessage& caProMessage) noexcept = 0;

    /// @brief cleanup the subscriber and release all the chunks it currently holds
    /// Caution: Contract is that user process is no more running when cleanup is called
    void releaseAllChunks() noexcept;

  protected:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;

    ChunkReceiver<SubscriberPortData::ChunkReceiverData_t> m_chunkReceiver;
};

} // namespace popo
} // namespace iox

#endif
