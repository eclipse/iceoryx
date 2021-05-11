// Copyright (c) 2019 - 2021 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_posh/roudi/roudi_app.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_utils/posix_wrapper/timer.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

#define private public
#define protected public

#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/testing/roudi_environment/roudi_environment.hpp"

#undef private
#undef protected

#include "test.hpp"

using namespace ::testing;
using namespace iox::units::duration_literals;
using iox::mepoo::MePooConfig;
using iox::roudi::RouDiEnvironment;
using ::testing::Return;

using iox::posix::Timer;

class Mepoo_IntegrationTest : public Test
{
  public:
    using MemPoolInfoContainer = iox::roudi::MemPoolInfoContainer;
    using MePooConfig = iox::mepoo::MePooConfig;

    struct TestMemPoolConfig
    {
        TestMemPoolConfig(uint32_t chunkSize, uint32_t chunkCount)
            : chunkSize(chunkSize)
            , chunkCount(chunkCount)
        {
        }
        uint32_t chunkSize;
        uint32_t chunkCount;
    };

    Mepoo_IntegrationTest()
    {
    }
    virtual ~Mepoo_IntegrationTest()
    {
    }

    // just to prevent the "hides overloaded virtual function" warning
    virtual void SetUp()
    {
        internal::CaptureStderr();
    }

    virtual void TearDown()
    {
        publisherPort->stopOffer();
        subscriberPort->unsubscribe();

        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    enum class configType
    {
        /// Default RouDi Config
        DEFAULT,
        /// Custom defined Mempool Config
        CUSTOM,
    };

    iox::RouDiConfig_t createRouDiConfig(MemPoolInfoContainer& memPoolTestContainer,
                                         std::vector<TestMemPoolConfig>& testMempoolConfig,
                                         const configType defaultconf = configType::DEFAULT)
    {
        if (defaultconf == configType::CUSTOM)
        {
            iox::mepoo::MePooConfig mempoolConfig;

            uint64_t mempoolSize{0};
            // create actual config
            for (size_t i = 0; i < testMempoolConfig.size() && i < memPoolTestContainer.capacity(); ++i)
            {
                iox::roudi::MemPoolInfo mempoolInfo;
                mempoolInfo.m_minFreeChunks = testMempoolConfig[i].chunkCount;
                mempoolInfo.m_usedChunks = 0;
                mempoolInfo.m_chunkSize = testMempoolConfig[i].chunkSize;
                mempoolInfo.m_numChunks = testMempoolConfig[i].chunkCount;
                memPoolTestContainer.push_back(mempoolInfo);

                mempoolConfig.m_mempoolConfig.push_back(
                    {testMempoolConfig[i].chunkSize, testMempoolConfig[i].chunkCount});
                mempoolSize += static_cast<uint64_t>(testMempoolConfig[i].chunkSize) * testMempoolConfig[i].chunkCount;
            }

            auto currentGroup = iox::posix::PosixGroup::getGroupOfCurrentProcess();
            iox::RouDiConfig_t roudiConfig;
            roudiConfig.m_sharedMemorySegments.push_back(
                {currentGroup.getName(), currentGroup.getName(), mempoolConfig});
            return roudiConfig;
        }
        else
        {
            return iox::RouDiConfig_t().setDefaults();
        }
    }

    void SetUp(MemPoolInfoContainer& memPoolTestContainer,
               std::vector<TestMemPoolConfig>& testMempoolConfig,
               const configType defaultconf = configType::DEFAULT)
    {
        auto config = createRouDiConfig(memPoolTestContainer, testMempoolConfig, defaultconf);
        m_roudiEnv = iox::cxx::optional<RouDiEnvironment>(config);

        ASSERT_THAT(m_roudiEnv.has_value(), Eq(true));

        iox::capro::ServiceDescription m_service_description{99, 1, 20};

        auto& senderRuntime = iox::runtime::PoshRuntime::initRuntime("sender");
        publisherPort.emplace(senderRuntime.getMiddlewarePublisher(m_service_description));

        auto& receiverRuntime = iox::runtime::PoshRuntime::initRuntime("receiver");
        subscriberPort.emplace(receiverRuntime.getMiddlewareSubscriber(m_service_description));
    }

    void SetUpRouDiOnly(MemPoolInfoContainer& memPoolTestContainer,
                        std::vector<TestMemPoolConfig>& testMempoolConfig,
                        const configType defaultconf = configType::DEFAULT)
    {
        auto config = createRouDiConfig(memPoolTestContainer, testMempoolConfig, defaultconf);
        m_roudiEnv = iox::cxx::optional<RouDiEnvironment>(config);

        ASSERT_THAT(m_roudiEnv.has_value(), Eq(true));
    }

    void PrintTiming(iox::units::Duration start)
    {
        auto totalMillisconds = start.toMilliseconds();
        auto milliseconds = totalMillisconds % 1000U;
        auto totalSeconds = totalMillisconds / 1000U;
        auto seconds = totalSeconds % 60U;
        auto minutes = totalSeconds / 60U;
        std::cerr << "RouDi startup took " << minutes << " minutes " << seconds << " seconds and " << milliseconds
                  << " milliseconds" << std::endl;
    }

    template <uint32_t size>
    struct MemPoolTestTopic
    {
        char testtopic[size] = {0};
    };

    enum class Log
    {
        On,
        Off
    };

    bool compareMemPoolInfo(MemPoolInfoContainer& first, MemPoolInfoContainer& second, Log doLog = Log::On)
    {
        // check containersize is equal
        if (first.size() != second.size())
        {
            return false;
        }

        size_t index = 0;
        for (auto& info : first)
        {
            // check only for mempool which are not used by the introspection; sendreceivesample takes care to not use
            // this mempools
            bool isInIntrospectionMempools =
                std::find(m_introspectionChunkSizes.begin(), m_introspectionChunkSizes.end(), info.m_chunkSize)
                != m_introspectionChunkSizes.end();
            if (!isInIntrospectionMempools)
            {
                if (info.m_chunkSize != second[index].m_chunkSize)
                {
                    if (doLog == Log::On)
                    {
                        std::cerr << "chunksize must: " << info.m_chunkSize
                                  << " is not equal to chunksize is: " << second[index].m_chunkSize << std::endl;
                    }
                    return false;
                }
                if (info.m_minFreeChunks != second[index].m_minFreeChunks)
                {
                    if (doLog == Log::On)
                    {
                        std::cerr << "m_minFreeChunks must: " << info.m_minFreeChunks
                                  << " is not equal to m_minFreeChunks is: " << second[index].m_minFreeChunks
                                  << std::endl;
                    }
                    return false;
                }
                if (info.m_numChunks != second[index].m_numChunks)
                {
                    if (doLog == Log::On)
                    {
                        std::cerr << "m_numChunks must: " << info.m_numChunks
                                  << " is not equal to m_numChunks is: " << second[index].m_numChunks << std::endl;
                    }
                    return false;
                }
                if (info.m_usedChunks != second[index].m_usedChunks)
                {
                    if (doLog == Log::On)
                    {
                        std::cerr << "m_usedChunks must: " << info.m_usedChunks
                                  << " is not equal to m_usedChunks is: " << second[index].m_usedChunks << std::endl;
                    }
                    return false;
                }
            }
            index++;
        }
        return true;
    }

    void getMempoolInfoFromIntrospection(MemPoolInfoContainer& mempoolInfo)
    {
        auto currentUser = iox::posix::PosixUser::getUserOfCurrentProcess();
        auto memoryManager = m_roudiEnv->m_roudiApp->m_mempoolIntrospection.m_segmentManager
                                 ->getSegmentInformationWithWriteAccessForUser(currentUser.getName())
                                 .m_memoryManager;
        ASSERT_TRUE(memoryManager.has_value());
        m_roudiEnv->m_roudiApp->m_mempoolIntrospection.copyMemPoolInfo(memoryManager.value().get(), mempoolInfo);

        // internally, the chunks are adjusted to the additional management information;
        // this needs to be subtracted to be able to compare to the configured sizes
        for (auto& mempool : mempoolInfo)
        {
            if (mempool.m_chunkSize != 0)
            {
                mempool.m_chunkSize = mempool.m_chunkSize - static_cast<uint32_t>(sizeof(iox::mepoo::ChunkHeader));
            }
        }
    }

    template <uint32_t size>
    uint32_t indexOfMempool(std::vector<TestMemPoolConfig>& testMempoolConfig) const
    {
        using Topic = MemPoolTestTopic<size>;
        constexpr auto topicSize = sizeof(Topic);

        uint32_t index = 0;
        for (const auto& mempoolConfig : testMempoolConfig)
        {
            if (topicSize <= mempoolConfig.chunkSize)
            {
                break;
            }
            index++;
        }
        return index;
    }

    template <uint32_t size>
    bool sendreceivesample(const int& times)
    {
        using Topic = MemPoolTestTopic<size>;
        constexpr auto TOPIC_SIZE = sizeof(Topic);
        constexpr auto TOPIC_ALIGNMENT = alignof(Topic);

        if (!(publisherPort->isOffered()))
        {
            publisherPort->offer();
        }

        if (subscriberPort->getSubscriptionState() != iox::SubscribeState::SUBSCRIBED)
        {
            subscriberPort->subscribe();
        }

        m_roudiEnv->InterOpWait();

        for (int idx = 0; idx < times; ++idx)
        {
            publisherPort
                ->tryAllocateChunk(
                    TOPIC_SIZE, TOPIC_ALIGNMENT, iox::CHUNK_NO_USER_HEADER_SIZE, iox::CHUNK_NO_USER_HEADER_ALIGNMENT)
                .and_then([&](auto sample) {
                    new (sample->userPayload()) Topic;
                    publisherPort->sendChunk(sample);
                    m_roudiEnv->InterOpWait();
                });
        }

        return true;
    }

    static constexpr uint32_t DefaultNumChunks = 10;

    std::vector<TestMemPoolConfig> defaultMemPoolConfig()
    {
        constexpr int MempoolCount = 6;
        std::vector<TestMemPoolConfig> defaultConfig;
        uint32_t power = 5;
        for (int i = 0; i < MempoolCount; ++i)
        {
            defaultConfig.push_back({1u << power, DefaultNumChunks});
            ++power;
        }

        return defaultConfig;
    }

    std::vector<uint32_t> m_introspectionChunkSizes;

    MePooConfig memconf;

    iox::cxx::optional<iox::popo::PublisherPortUser> publisherPort;
    iox::cxx::optional<iox::popo::SubscriberPortUser> subscriberPort;

    iox::cxx::optional<RouDiEnvironment> m_roudiEnv;
};


TEST_F(Mepoo_IntegrationTest, MempoolConfigCheck)
{
    MemPoolInfoContainer memPoolTestContainer;

    auto testMempoolConfig = defaultMemPoolConfig();

    SetUp(memPoolTestContainer, testMempoolConfig, configType::CUSTOM);

    constexpr int samplesize1 = 200;
    const int repetition1 = 1;
    ASSERT_TRUE(sendreceivesample<samplesize1>(repetition1));
    auto mempolIndex1 = indexOfMempool<samplesize1>(testMempoolConfig);
    memPoolTestContainer[mempolIndex1].m_usedChunks = repetition1;
    memPoolTestContainer[mempolIndex1].m_minFreeChunks =
        memPoolTestContainer[mempolIndex1].m_minFreeChunks - repetition1;

    m_roudiEnv->InterOpWait();

    constexpr int samplesize2 = 450;
    const int repetition2 = 3;
    ASSERT_TRUE(sendreceivesample<samplesize2>(repetition2));
    auto mempolIndex2 = indexOfMempool<samplesize2>(testMempoolConfig);
    memPoolTestContainer[mempolIndex2].m_usedChunks = repetition2;
    memPoolTestContainer[mempolIndex2].m_minFreeChunks =
        memPoolTestContainer[mempolIndex2].m_minFreeChunks - repetition2;

    m_roudiEnv->InterOpWait();


    // get mempoolconfig from introspection
    MemPoolInfoContainer memPoolInfoContainer;
    EXPECT_THAT(compareMemPoolInfo(memPoolInfoContainer, memPoolTestContainer, Log::Off), Eq(false));
    getMempoolInfoFromIntrospection(memPoolInfoContainer);

    EXPECT_THAT(compareMemPoolInfo(memPoolInfoContainer, memPoolTestContainer), Eq(true));
}

TEST_F(Mepoo_IntegrationTest, WrongSampleSize)
{
    MemPoolInfoContainer memPoolTestContainer;
    auto testMempoolConfig = defaultMemPoolConfig();
    SetUp(memPoolTestContainer, testMempoolConfig, configType::CUSTOM);
    constexpr int samplesize3 = 2048;
    const int repetition3 = 1;
    auto errorHandlerCalled{false};
    iox::Error receivedError{iox::Error::kNO_ERROR};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled,
         &receivedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
            receivedError = error;
        });

    sendreceivesample<samplesize3>(repetition3);

    EXPECT_TRUE(errorHandlerCalled);
    ASSERT_THAT(receivedError, Eq(iox::Error::kMEPOO__MEMPOOL_GETCHUNK_CHUNK_IS_TOO_LARGE));
}

TEST_F(Mepoo_IntegrationTest, SampleOverflow)
{
    MemPoolInfoContainer memPoolTestContainer;
    auto testMempoolConfig = defaultMemPoolConfig();
    SetUp(memPoolTestContainer, testMempoolConfig, configType::CUSTOM);
    constexpr int samplesize1 = 200;
    const int repetition1 = 2 * DefaultNumChunks;
    auto errorHandlerCalled{false};
    iox::Error receivedError{iox::Error::kNO_ERROR};
    auto errorHandlerGuard = iox::ErrorHandler::SetTemporaryErrorHandler(
        [&errorHandlerCalled,
         &receivedError](const iox::Error error, const std::function<void()>, const iox::ErrorLevel) {
            errorHandlerCalled = true;
            receivedError = error;
        });

    sendreceivesample<samplesize1>(repetition1);

    EXPECT_TRUE(errorHandlerCalled);
    ASSERT_THAT(receivedError, Eq(iox::Error::kMEPOO__MEMPOOL_GETCHUNK_POOL_IS_RUNNING_OUT_OF_CHUNKS));
}

TIMING_TEST_F(Mepoo_IntegrationTest, MempoolCreationTimeDefaultConfig, Repeat(5), [&] {
    MemPoolInfoContainer memPoolTestContainer;
    auto testMempoolConfig = defaultMemPoolConfig();

    auto start = Timer::now();

    SetUp(memPoolTestContainer, testMempoolConfig, configType::DEFAULT);

    auto stop = Timer::now();

    // Calc the difference
    iox::units::Duration timediff = stop.value() - start.value();

    PrintTiming(timediff);

    /// Currently we expect that the RouDi is ready at least after 2 seconds
    auto maxtime = 2000_ms;
    EXPECT_THAT(timediff, Le(maxtime));
});
