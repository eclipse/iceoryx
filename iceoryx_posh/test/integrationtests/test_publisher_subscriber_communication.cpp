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

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_posh/testing/roudi_gtest.hpp"
#include "iceoryx_utils/cxx/forward_list.hpp"
#include "iceoryx_utils/cxx/list.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/stack.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/variant.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"
#include "iceoryx_utils/testing/watch_dog.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

using namespace iox;
using namespace iox::popo;
using namespace iox::cxx;

namespace
{
template <typename T>
struct ComplexDataType
{
    int64_t someNumber = 0;
    T complexType;
};

class PublisherSubscriberCommunication_test : public RouDi_GTest
{
  public:
    void SetUp()
    {
        runtime::PoshRuntime::initRuntime("PublisherSubscriberCommunication_test");
        m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    };
    void TearDown(){};

    template <typename T>
    std::unique_ptr<iox::popo::Publisher<T>>
    createPublisher(const SubscriberTooSlowPolicy policy = SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA)
    {
        iox::popo::PublisherOptions options;
        options.subscriberTooSlowPolicy = policy;
        return std::make_unique<iox::popo::Publisher<T>>(m_serviceDescription, options);
    }

    template <typename T>
    std::unique_ptr<iox::popo::Subscriber<T>>
    createSubscriber(const QueueFullPolicy policy = QueueFullPolicy::DISCARD_OLDEST_DATA,
                     const uint64_t queueCapacity = SubscriberPortData::ChunkQueueData_t::MAX_CAPACITY)
    {
        iox::popo::SubscriberOptions options;
        options.queueFullPolicy = policy;
        options.queueCapacity = queueCapacity;
        return std::make_unique<iox::popo::Subscriber<T>>(m_serviceDescription, options);
    }


    Watchdog m_watchdog{units::Duration::fromSeconds(5)};
    capro::ServiceDescription m_serviceDescription{
        "PublisherSubscriberCommunication", "IntegrationTest", "AllHailHypnotoad"};
};
} // namespace

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_forward_list)
{
    using Type_t = ComplexDataType<forward_list<string<5>, 5>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 123;
                         sample->complexType.push_front("world");
                         sample->complexType.push_front("hello");
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(123));
                         ASSERT_THAT(sample->complexType.size(), Eq(2U));
                         auto begin = sample->complexType.begin();
                         EXPECT_THAT(*begin, Eq(cxx::string<5>("hello")));
                         ++begin;
                         EXPECT_THAT(*begin, Eq(cxx::string<5>("world")));
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_list)
{
    using Type_t = ComplexDataType<list<int64_t, 5>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 4123;
                         sample->complexType.push_front(77);
                         sample->complexType.push_front(66);
                         sample->complexType.push_front(55);
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(4123));
                         ASSERT_THAT(sample->complexType.size(), Eq(3U));
                         auto begin = sample->complexType.begin();
                         EXPECT_THAT(*begin, Eq(55));
                         ++begin;
                         EXPECT_THAT(*begin, Eq(66));
                         ++begin;
                         EXPECT_THAT(*begin, Eq(77));
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_optional)
{
    using Type_t = ComplexDataType<list<optional<int32_t>, 5>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 41231;
                         sample->complexType.push_front(177);
                         sample->complexType.push_front(nullopt);
                         sample->complexType.push_front(155);
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(41231));
                         ASSERT_THAT(sample->complexType.size(), Eq(3U));
                         auto begin = sample->complexType.begin();
                         EXPECT_THAT(*begin, Eq(cxx::optional<int32_t>(155)));
                         ++begin;
                         EXPECT_THAT(*begin, Eq(nullopt));
                         ++begin;
                         EXPECT_THAT(*begin, Eq(cxx::optional<int32_t>(177)));
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_stack)
{
    using Type_t = ComplexDataType<stack<int64_t, 10>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 41231;
                         for (uint64_t i = 0U; i < 10U; ++i)
                         {
                             sample->complexType.push(i + 123);
                         }
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(41231));
                         ASSERT_THAT(sample->complexType.size(), Eq(10U));
                         auto stackCopy = sample->complexType;
                         for (uint64_t i = 0U; i < 10U; ++i)
                         {
                             auto result = stackCopy.pop();
                             ASSERT_THAT(result.has_value(), Eq(true));
                             EXPECT_THAT(*result, Eq(123 + 9 - i));
                         }
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_string)
{
    using Type_t = ComplexDataType<string<128>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 123;
                         sample->complexType = "You're my Heart, You're my Seal!";
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(123));
                         EXPECT_THAT(sample->complexType, Eq(string<128>("You're my Heart, You're my Seal!")));
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_vector)
{
    using Type_t = ComplexDataType<vector<string<128>, 20>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 123;
                         sample->complexType.emplace_back("Don't stop the hypnotoad");
                         sample->complexType.emplace_back("Be like hypnotoad");
                         sample->complexType.emplace_back("Piep, piep little satellite");
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(123));
                         ASSERT_THAT(sample->complexType.size(), Eq(3));
                         EXPECT_THAT(sample->complexType[0], Eq(string<128>("Don't stop the hypnotoad")));
                         EXPECT_THAT(sample->complexType[1], Eq(string<128>("Be like hypnotoad")));
                         EXPECT_THAT(sample->complexType[2], Eq(string<128>("Piep, piep little satellite")));
                     })
                     .has_error());
}

TEST_F(PublisherSubscriberCommunication_test, SendingComplexDataType_variant)
{
    using Type_t = ComplexDataType<vector<variant<string<128>, int>, 20>>;
    auto publisher = createPublisher<Type_t>();
    this->InterOpWait();
    auto subscriber = createSubscriber<Type_t>();
    this->InterOpWait();

    ASSERT_FALSE(publisher->loan()
                     .and_then([](auto& sample) {
                         sample->someNumber = 123;
                         sample->complexType.emplace_back(in_place_index<0>(), "Be aware! Bob is a vampire!");
                         sample->complexType.emplace_back(in_place_index<1>(), 1337);
                         sample->complexType.emplace_back(in_place_index<0>(), "Bob is an acronym for Bob Only Bob");
                         sample.publish();
                     })
                     .has_error());

    EXPECT_FALSE(subscriber->take()
                     .and_then([](auto& sample) {
                         EXPECT_THAT(sample->someNumber, Eq(123));
                         ASSERT_THAT(sample->complexType.size(), Eq(3));
                         ASSERT_THAT(sample->complexType[0].index(), Eq(0));
                         EXPECT_THAT(*sample->complexType[0].template get_at_index<0>(),
                                     Eq(string<128>("Be aware! Bob is a vampire!")));
                         ASSERT_THAT(sample->complexType[1].index(), Eq(1));
                         EXPECT_THAT(*sample->complexType[1].template get_at_index<1>(), Eq(1337));
                         ASSERT_THAT(sample->complexType[2].index(), Eq(0));
                         EXPECT_THAT(*sample->complexType[2].template get_at_index<0>(),
                                     Eq(string<128>("Bob is an acronym for Bob Only Bob")));
                     })
                     .has_error());
}


TEST_F(PublisherSubscriberCommunication_test, PublisherBlocksWhenBlockingActivatedOnBothSidesAndSubscriberQueueIsFull)
{
    auto publisher = createPublisher<string<128>>(SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER);
    this->InterOpWait();

    auto subscriber = createSubscriber<string<128>>(QueueFullPolicy::BLOCK_PUBLISHER, 2U);
    this->InterOpWait();

    EXPECT_FALSE(publisher->publishCopyOf("start your day with a smile").has_error());
    EXPECT_FALSE(publisher->publishCopyOf("and hypnotoad will smile back").has_error());

    auto threadSyncSemaphore = posix::Semaphore::create(posix::CreateUnnamedSingleProcessSemaphore, 0U);

    std::atomic_bool wasSampleDelivered{false};
    std::thread t1([&] {
        ASSERT_FALSE(threadSyncSemaphore->post().has_error());
        EXPECT_FALSE(publisher->publishCopyOf("oh no hypnotoad is staring at me").has_error());
        wasSampleDelivered.store(true);
    });

    constexpr int64_t TIMEOUT_IN_MS = 100;

    ASSERT_FALSE(threadSyncSemaphore->wait().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_FALSE(wasSampleDelivered.load());

    auto sample = subscriber->take();
    ASSERT_FALSE(sample.has_error());
    EXPECT_THAT(**sample, Eq(string<128>("start your day with a smile")));
    t1.join(); // join needs to be before the load to ensure the wasSampleDelivered store happens before the read
    EXPECT_TRUE(wasSampleDelivered.load());

    EXPECT_FALSE(subscriber->hasMissedData());    
    sample = subscriber->take();
    ASSERT_FALSE(sample.has_error());
    EXPECT_THAT(**sample, Eq(string<128>("and hypnotoad will smile back")));

    sample = subscriber->take();
    ASSERT_FALSE(sample.has_error());
    EXPECT_THAT(**sample, Eq(string<128>("oh no hypnotoad is staring at me")));
}

TEST_F(PublisherSubscriberCommunication_test, PublisherDoesNotBlockAndDiscardsSamplesWhenNonBlockingActivated)
{
    auto publisher = createPublisher<string<128>>(SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA);
    this->InterOpWait();

    auto subscriber = createSubscriber<string<128>>(QueueFullPolicy::DISCARD_OLDEST_DATA, 2U);
    this->InterOpWait();

    EXPECT_FALSE(publisher->publishCopyOf("first there was a blubb named mantua").has_error());
    EXPECT_FALSE(publisher->publishCopyOf("second hypnotoad ate it").has_error());

    auto threadSyncSemaphore = posix::Semaphore::create(posix::CreateUnnamedSingleProcessSemaphore, 0U);

    std::atomic_bool wasSampleDelivered{false};
    std::thread t1([&] {
        ASSERT_FALSE(threadSyncSemaphore->post().has_error());
        EXPECT_FALSE(publisher->publishCopyOf("third a tiny black hole smells like butter").has_error());
        wasSampleDelivered.store(true);
    });

    ASSERT_FALSE(threadSyncSemaphore->wait().has_error());
    t1.join();
    EXPECT_TRUE(wasSampleDelivered.load());

    EXPECT_TRUE(subscriber->hasMissedData());   
    auto sample = subscriber->take();
    ASSERT_FALSE(sample.has_error());
    EXPECT_THAT(**sample, Eq(string<128>("second hypnotoad ate it")));

    sample = subscriber->take();
    ASSERT_FALSE(sample.has_error());
    EXPECT_THAT(**sample, Eq(string<128>("third a tiny black hole smells like butter")));

}

TEST_F(PublisherSubscriberCommunication_test, NoSubscriptionWhenSubscriberWantsBlockingAndPublisherDoesNotOfferBlocking)
{
    auto publisher = createPublisher<string<128>>(SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA);
    this->InterOpWait();

    auto subscriber = createSubscriber<string<128>>(QueueFullPolicy::BLOCK_PUBLISHER, 2U);
    this->InterOpWait();

    EXPECT_FALSE(publisher->publishCopyOf("never kiss the hypnotoad").has_error());
    this->InterOpWait();

    auto sample = subscriber->take();
    EXPECT_THAT(sample.has_error(), Eq(true));
}

TEST_F(PublisherSubscriberCommunication_test, SubscriptionWhenSubscriberDoesNotRequireBlockingButPublisherSupportsIt)
{
    auto publisher = createPublisher<string<128>>(SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER);
    this->InterOpWait();

    auto subscriber = createSubscriber<string<128>>(QueueFullPolicy::DISCARD_OLDEST_DATA, 2U);
    this->InterOpWait();

    EXPECT_FALSE(publisher->publishCopyOf("never kiss the hypnotoad").has_error());
    this->InterOpWait();

    auto sample = subscriber->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(string<128>("never kiss the hypnotoad")));
}

TEST_F(PublisherSubscriberCommunication_test, MixedOptionsSetupWorksWithBlocking)
{
    auto publisherBlocking = createPublisher<string<128>>(SubscriberTooSlowPolicy::WAIT_FOR_SUBSCRIBER);
    auto publisherNonBlocking = createPublisher<string<128>>(SubscriberTooSlowPolicy::DISCARD_OLDEST_DATA);
    this->InterOpWait();

    auto subscriberBlocking = createSubscriber<string<128>>(QueueFullPolicy::BLOCK_PUBLISHER, 2U);
    auto subscriberNonBlocking = createSubscriber<string<128>>(QueueFullPolicy::DISCARD_OLDEST_DATA, 2U);
    this->InterOpWait();

    EXPECT_FALSE(publisherBlocking->publishCopyOf("hypnotoads real name is Salsabarh Slimekirkdingle").has_error());
    EXPECT_FALSE(publisherBlocking->publishCopyOf("hypnotoad wants a cookie").has_error());
    EXPECT_FALSE(publisherNonBlocking->publishCopyOf("hypnotoad has a sister named hypnoodle").has_error());

    auto threadSyncSemaphore = posix::Semaphore::create(posix::CreateUnnamedSingleProcessSemaphore, 0U);

    std::atomic_bool wasSampleDelivered{false};
    std::thread t1([&] {
        ASSERT_FALSE(threadSyncSemaphore->post().has_error());
        EXPECT_FALSE(publisherBlocking->publishCopyOf("chucky is the only one who can ride the hypnotoad").has_error());
        wasSampleDelivered.store(true);
    });

    constexpr int64_t TIMEOUT_IN_MS = 100;

    ASSERT_FALSE(threadSyncSemaphore->wait().has_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_IN_MS));
    EXPECT_FALSE(wasSampleDelivered.load());

    // verify blocking subscriber
    auto sample = subscriberBlocking->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(cxx::string<128>("hypnotoads real name is Salsabarh Slimekirkdingle")));
    t1.join(); // join needs to be before the load to ensure the wasSampleDelivered store happens before the read
    EXPECT_TRUE(wasSampleDelivered.load());

    EXPECT_FALSE(subscriberBlocking->hasMissedData());  // we don't loose samples here
    sample = subscriberBlocking->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(cxx::string<128>("hypnotoad wants a cookie")));

    sample = subscriberBlocking->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(cxx::string<128>("chucky is the only one who can ride the hypnotoad")));
    EXPECT_THAT(subscriberBlocking->take().has_error(), Eq(true));

    // verify non blocking subscriber
    EXPECT_TRUE(subscriberNonBlocking->hasMissedData()); // we do loose samples here
    sample = subscriberNonBlocking->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(cxx::string<128>("hypnotoad has a sister named hypnoodle")));

    sample = subscriberNonBlocking->take();
    EXPECT_THAT(sample.has_error(), Eq(false));
    EXPECT_THAT(**sample, Eq(cxx::string<128>("chucky is the only one who can ride the hypnotoad")));
    EXPECT_THAT(subscriberNonBlocking->take().has_error(), Eq(true));

}
