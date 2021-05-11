// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "mocks/mepoo_memory_manager_mock.hpp"
#include "mocks/publisher_mock.hpp"
#include "iceoryx_utils/testing/timing_test.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

#include "iceoryx_posh/internal/roudi/introspection/mempool_introspection.hpp"

#include "iceoryx_posh/internal/mepoo/segment_manager.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

class CallChecker
{
  public:
    MOCK_METHOD0(offer, void(void));
};

CallChecker& callChecker()
{
    static CallChecker cc;
    return cc;
}

class MockPublisherPortUserAccess : public MockPublisherPortUser
{
  public:
    void offer()
    {
        callChecker().offer();
    }
};

class SegmentMock
{
  public:
    SegmentMock() = default;
    SegmentMock(const SegmentMock&){};

    MePooMemoryManager_MOCK& getMemoryManager()
    {
        return memoryManager;
    }

    iox::posix::PosixGroup getWriterGroup() const
    {
        return iox::posix::PosixGroup::getGroupOfCurrentProcess();
    }

    iox::posix::PosixGroup getReaderGroup() const
    {
        return iox::posix::PosixGroup::getGroupOfCurrentProcess();
    }

  private:
    MePooMemoryManager_MOCK memoryManager;
};

class SegmentManagerMock
{
  public:
    iox::cxx::vector<SegmentMock, iox::MAX_SHM_SEGMENTS> m_segmentContainer;
};


class MemPoolIntrospectionAccess
    : public iox::roudi::MemPoolIntrospection<MePooMemoryManager_MOCK, SegmentManagerMock, MockPublisherPortUserAccess>
{
  public:
    MemPoolIntrospectionAccess(MePooMemoryManager_MOCK& memoryManager,
                               SegmentManagerMock& segmentManager,
                               MockPublisherPortUserAccess&& publisherPort)
        : iox::roudi::MemPoolIntrospection<MePooMemoryManager_MOCK, SegmentManagerMock, MockPublisherPortUserAccess>(
            memoryManager, segmentManager, std::move(publisherPort))
    {
    }
    MockPublisherPortUserAccess& getPublisherPort()
    {
        return this->m_publisherPort;
    }

    using iox::roudi::MemPoolIntrospection<MePooMemoryManager_MOCK, SegmentManagerMock, MockPublisherPortUserAccess>::
        send;
};

class MemPoolIntrospection_test : public Test
{
  public:
    using MemPoolInfoContainer = iox::roudi::MemPoolInfoContainer;
    using MemPoolInfo = iox::mepoo::MemPoolInfo;
    using Topic = iox::roudi::MemPoolIntrospectionInfoContainer;

    MemPoolIntrospection_test()
    {
    }

    ~MemPoolIntrospection_test()
    {
    }

    virtual void SetUp()
    {
        internal::CaptureStdout();
        SegmentMock segmentMock;
        m_segmentManager_mock.m_segmentContainer.push_back(segmentMock);
    }

    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStdout();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    template <typename MemPoolInfoStruct>
    void initMemPoolInfo(int index, MemPoolInfoStruct& info)
    {
        info.m_chunkSize = index * 100 + 10;
        info.m_minFreeChunks = index * 100 + 45;
        info.m_numChunks = index * 100 + 50;
        info.m_usedChunks = index * 100 + 3;
    }

    // initializes the mempool info with a defined pattern
    void initMemPoolInfoContainer(MemPoolInfoContainer& memPoolInfoContainer)
    {
        int index = 0;
        for (auto& info : memPoolInfoContainer)
        {
            initMemPoolInfo(index, info);
            index++;
        }
    }

    // compares mempool info structs and returns true if they are equal
    template <typename MemPoolInfoType1, typename MemPoolInfoType2>
    bool compareMemPoolInfo(MemPoolInfoType1& first, MemPoolInfoType2& second)
    {
        int index = 0;
        for (auto& info : first)
        {
            if (info.m_chunkSize != second[index].m_chunkSize)
            {
                return false;
            }
            if (info.m_minFreeChunks != second[index].m_minFreeChunks)
            {
                return false;
            }
            if (info.m_numChunks != second[index].m_numChunks)
            {
                return false;
            }
            if (info.m_usedChunks != second[index].m_usedChunks)
            {
                return false;
            }
            index++;
        }

        return true;
    }

    MePooMemoryManager_MOCK m_rouDiInternalMemoryManager_mock;
    SegmentManagerMock m_segmentManager_mock;
    MockPublisherPortUserAccess m_publisherPortImpl_mock;
};

TEST_F(MemPoolIntrospection_test, CTOR)
{
    {
        EXPECT_CALL(callChecker(), offer()).Times(1);

        MemPoolIntrospectionAccess introspectionAccess(
            m_rouDiInternalMemoryManager_mock, m_segmentManager_mock, std::move(m_publisherPortImpl_mock));

        EXPECT_CALL(introspectionAccess.getPublisherPort(), stopOffer()).Times(1);
    }
}

TEST_F(MemPoolIntrospection_test, send_noSubscribers)
{
    EXPECT_CALL(callChecker(), offer()).Times(1);

    MemPoolIntrospectionAccess introspectionAccess(
        m_rouDiInternalMemoryManager_mock, m_segmentManager_mock, std::move(m_publisherPortImpl_mock));

    MemPoolInfoContainer memPoolInfoContainer;
    initMemPoolInfoContainer(memPoolInfoContainer);

    EXPECT_CALL(introspectionAccess.getPublisherPort(), tryAllocateChunk(_, _, _, _)).Times(0);

    introspectionAccess.send();
}

/// @todo test with multiple segments and also test the mempool info from RouDiInternalMemoryManager
/// @todo This test is not very useful as it is highly implementation-dependent and fails if the implementation changes.
/// Should be realized as an integration test with a roudi environment and less mocking classes instead.
TEST_F(MemPoolIntrospection_test, DISABLED_send_withSubscribers)
{
    EXPECT_CALL(callChecker(), offer()).Times(1);

    MemPoolIntrospectionAccess introspectionAccess(
        m_rouDiInternalMemoryManager_mock, m_segmentManager_mock, std::move(m_publisherPortImpl_mock));

    MemPoolInfoContainer memPoolInfoContainer;
    MemPoolInfo memPoolInfo{0, 0, 0, 0};
    initMemPoolInfoContainer(memPoolInfoContainer);

    EXPECT_CALL(m_segmentManager_mock.m_segmentContainer.front().getMemoryManager(), getMemPoolInfo(_))
        .WillRepeatedly(Invoke([&](uint32_t index) {
            initMemPoolInfo(index, memPoolInfo);
            return memPoolInfo;
        }));

    ChunkMock<Topic> chunk;
    const auto& sample = chunk.sample();

    introspectionAccess.send(); /// @todo expect call to MemPoolHandler::getMemPoolInfo

    EXPECT_CALL(m_publisherPortImpl_mock, sendChunk(_)).Times(1);
    ASSERT_EQ(sample->size(), 1u);
    EXPECT_THAT(compareMemPoolInfo(memPoolInfoContainer, chunk.sample()->front().m_mempoolInfo), Eq(true));
}

TIMING_TEST_F(MemPoolIntrospection_test, thread, Repeat(5), [&] {
    EXPECT_CALL(callChecker(), offer()).Times(1);

    MemPoolIntrospectionAccess introspectionAccess(
        m_rouDiInternalMemoryManager_mock, m_segmentManager_mock, std::move(m_publisherPortImpl_mock));

    MemPoolInfoContainer memPoolInfoContainer;
    MemPoolInfo memPoolInfo(0, 0, 0, 0);
    initMemPoolInfoContainer(memPoolInfoContainer);

    EXPECT_CALL(m_rouDiInternalMemoryManager_mock, getMemPoolInfo(_)).WillRepeatedly(Invoke([&](uint32_t index) {
        initMemPoolInfo(index, memPoolInfo);
        return memPoolInfo;
    }));
    // we use the hasSubscribers call to check how often the thread calls the send method
    EXPECT_CALL(introspectionAccess.getPublisherPort(), hasSubscribers).Times(AtLeast(4));

    using namespace iox::units::duration_literals;
    iox::units::Duration snapshotInterval(100_ms);

    introspectionAccess.setSendInterval(snapshotInterval);
    introspectionAccess.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(
        6 * snapshotInterval.toMilliseconds())); // within this time, the thread should have run 6 times
    introspectionAccess.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(
        6 * snapshotInterval.toMilliseconds())); // the thread should sleep, if not, we have 12 runs
    introspectionAccess.stop();
});
