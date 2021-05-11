// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_MOCKS_SUBSCRIBER_MOCK_HPP
#define IOX_POSH_MOCKS_SUBSCRIBER_MOCK_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/popo/base_subscriber.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_posh/popo/trigger.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::_;

class MockSubscriberPortUser
{
  public:
    using MemberType_t = iox::popo::SubscriberPortData;
    MockSubscriberPortUser() = default;
    MockSubscriberPortUser(std::nullptr_t)
    {
    }

    MockSubscriberPortUser(iox::popo::SubscriberPortData*){};
    iox::capro::ServiceDescription getCaProServiceDescription() const noexcept
    {
        return getServiceDescription();
    }
    MOCK_CONST_METHOD0(getServiceDescription, iox::capro::ServiceDescription());
    MOCK_METHOD0(subscribe, void());
    MOCK_METHOD0(unsubscribe, void());
    MOCK_CONST_METHOD0(getSubscriptionState, iox::SubscribeState());
    MOCK_METHOD0(tryGetChunk, iox::cxx::expected<const iox::mepoo::ChunkHeader*, iox::popo::ChunkReceiveResult>());
    MOCK_METHOD1(releaseChunk, void(const void* const));
    MOCK_METHOD0(releaseQueuedChunks, void());
    MOCK_CONST_METHOD0(hasNewChunks, bool());
    MOCK_METHOD0(hasLostChunksSinceLastCall, bool());
    MOCK_METHOD2(setConditionVariable, bool(iox::popo::ConditionVariableData&, uint64_t));
    MOCK_METHOD0(isConditionVariableSet, bool());
    MOCK_METHOD0(unsetConditionVariable, bool());
    MOCK_METHOD0(destroy, bool());
    MOCK_CONST_METHOD0(getUniqueID, iox::UniquePortId());
};

template <typename T, typename Port = MockSubscriberPortUser>
class MockBaseSubscriber
{
  public:
    using SelfType = MockBaseSubscriber<T, Port>;
    using PortType = Port;

    MockBaseSubscriber(const iox::capro::ServiceDescription&, const iox::popo::SubscriberOptions&){};
    MOCK_CONST_METHOD0(getUid, iox::popo::uid_t());
    MOCK_CONST_METHOD0(getServiceDescription, iox::capro::ServiceDescription());
    MOCK_METHOD1(subscribe, void(uint64_t));
    MOCK_CONST_METHOD0(getSubscriptionState, iox::SubscribeState());
    MOCK_METHOD0(unsubscribe, void());
    MOCK_CONST_METHOD0(hasData, bool());
    MOCK_METHOD0(hasMissedData, bool());
    MOCK_METHOD0(takeChunk, iox::cxx::expected<const iox::mepoo::ChunkHeader*, iox::popo::ChunkReceiveResult>());
    MOCK_METHOD0(releaseQueuedData, void());
    MOCK_METHOD1(invalidateTrigger, bool(const uint64_t));
    MOCK_METHOD1(disableEvent, void(const iox::popo::SubscriberEvent));

    const Port& port() const noexcept
    {
        return m_port;
    }

    Port& port() noexcept
    {
        return m_port;
    }

    Port m_port;
    iox::popo::TriggerHandle m_trigger;
};

#endif // IOX_POSH_MOCKS_SUBSCRIBER_MOCK_HPP
