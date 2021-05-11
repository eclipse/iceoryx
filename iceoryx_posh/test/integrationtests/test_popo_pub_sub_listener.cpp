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

#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/testing/roudi_gtest.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

using namespace iox::popo;
using namespace iox::capro;
using namespace iox::runtime;

namespace
{
void onSampleReceivedCallback(Subscriber<int>* subscriber IOX_MAYBE_UNUSED)
{
}

void onSampleReceivedCallbackForUntypedSub(UntypedSubscriber* subscriber IOX_MAYBE_UNUSED)
{
}

class PubSubListener_IntegrationTest : public RouDi_GTest
{
  public:
    void SetUp() override
    {
        PoshRuntime::initRuntime("PubSubListener_IntegrationTest");
        m_listener = std::make_unique<Listener>();
        m_subscriber = std::make_unique<Subscriber<int>>(m_serviceDescr);
        m_untypedSubscriber = std::make_unique<UntypedSubscriber>(m_serviceDescr);
    }
    void TearDown() override
    {
    }

    ServiceDescription m_serviceDescr{"Radar", "FrontLeft", "Counter"};
    std::unique_ptr<Listener> m_listener;
    std::unique_ptr<Subscriber<int>> m_subscriber;
    std::unique_ptr<UntypedSubscriber> m_untypedSubscriber;
};
} // namespace

/// @note Here we test that the trigger reset methods are called correctly in the d'tor of SubscriberImpl. They must not
/// be called in the BaseSubscriber d'tor since the SubscriberImpl was attached to the Listener. When it goes out of
/// scope, the trigger tries to access it but SubscriberImpl does not longer exist. This is caught by the
/// UndefinedBehaviorSanitizer.
TEST_F(PubSubListener_IntegrationTest, SubscriberGoesOutOfScopeAndDetachingWorks)
{
    m_listener
        ->attachEvent(*m_subscriber,
                      iox::popo::SubscriberEvent::DATA_RECEIVED,
                      iox::popo::createNotificationCallback(onSampleReceivedCallback))
        .or_else([](auto) { ASSERT_TRUE(false); });

    m_subscriber.reset();
}

/// @note Here we test that the trigger reset methods are called correctly in the d'tor of UntypedSubscriberImpl. They
/// must not be called in the BaseSubscriber d'tor since the UntypedSubscriberImpl was attached to the Listener. When it
/// goes out of scope, the trigger tries to access it but UntypedSubscriberImpl does not longer exist. This is caught
/// by the UndefinedBehaviorSanitizer.
TEST_F(PubSubListener_IntegrationTest, UntypedSubscriberGoesOutOfScopeAndDetachingWorks)
{
    m_listener
        ->attachEvent(*m_untypedSubscriber,
                      iox::popo::SubscriberEvent::DATA_RECEIVED,
                      iox::popo::createNotificationCallback(onSampleReceivedCallbackForUntypedSub))
        .or_else([](auto) { ASSERT_TRUE(false); });

    m_untypedSubscriber.reset();
}

