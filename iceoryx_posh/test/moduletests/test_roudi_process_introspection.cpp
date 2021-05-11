// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/testing/timing_test.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::Return;

#include "iceoryx_posh/internal/roudi/introspection/process_introspection.hpp"

#include "iceoryx_posh/internal/popo/ports/publisher_port_data.hpp"
#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"
#include "mocks/publisher_mock.hpp"

class ProcessIntrospectionAccess : public iox::roudi::ProcessIntrospection<MockPublisherPortUser>
{
  public:
    using iox::roudi::ProcessIntrospection<MockPublisherPortUser>::send;

    iox::cxx::optional<MockPublisherPortUser>& getPublisherPort()
    {
        return this->m_publisherPort;
    }
};

class ProcessIntrospection_test : public Test
{
  public:
    using Topic = iox::roudi::ProcessIntrospectionFieldTopic;

    ProcessIntrospection_test()
    {
    }

    ~ProcessIntrospection_test()
    {
    }

    virtual void SetUp()
    {
        internal::CaptureStdout();
    }

    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStdout();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    ChunkMock<Topic>* createMemoryChunkAndSend(ProcessIntrospectionAccess& introspectionAccess)
    {
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), tryAllocateChunk(_, _, _, _))
            .WillOnce(Return(iox::cxx::expected<iox::mepoo::ChunkHeader*, iox::popo::AllocationError>::create_value(
                m_chunk.get()->chunkHeader())));

        bool chunkWasSent = false;
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), sendChunk(_))
            .WillOnce(Invoke([&](iox::mepoo::ChunkHeader* const) { chunkWasSent = true; }));

        introspectionAccess.send();

        return chunkWasSent ? m_chunk.get() : nullptr;
    }

    std::unique_ptr<ChunkMock<Topic>> m_chunk{new ChunkMock<Topic>()};
    MockPublisherPortUser m_mockPublisherPortUserIntrospection;
};

TEST_F(ProcessIntrospection_test, CTOR)
{
    {
        ProcessIntrospectionAccess introspectionAccess;
        EXPECT_THAT(introspectionAccess.getPublisherPort().has_value(), Eq(false));
    }
}

TEST_F(ProcessIntrospection_test, registerPublisherPort)
{
    {
        ProcessIntrospectionAccess introspectionAccess;
        introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection));
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), stopOffer()).Times(1);
    }
}

TEST_F(ProcessIntrospection_test, send)
{
    {
        ProcessIntrospectionAccess introspectionAccess;
        introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection));

        auto chunk = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk, Ne(nullptr));
        EXPECT_THAT(chunk->sample()->m_processList.size(), Eq(0U));
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), stopOffer()).Times(1);
    }
}

TEST_F(ProcessIntrospection_test, addRemoveProcess)
{
    {
        ProcessIntrospectionAccess introspectionAccess;
        introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection));

        const int PID = 42;
        const char PROCESS_NAME[] = "/chuck_norris";

        // invalid removal doesn't cause problems
        introspectionAccess.removeProcess(PID);
        auto chunk1 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk1, Ne(nullptr));
        EXPECT_THAT(chunk1->sample()->m_processList.size(), Eq(0U));

        // a new process should be sent
        introspectionAccess.addProcess(PID, iox::RuntimeName_t(PROCESS_NAME));
        auto chunk2 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk2, Ne(nullptr));
        EXPECT_THAT(chunk2->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk2->sample()->m_processList[0].m_pid, Eq(PID));
        EXPECT_THAT(iox::RuntimeName_t(PROCESS_NAME) == chunk2->sample()->m_processList[0].m_name, Eq(true));

        // list should be empty after removal
        introspectionAccess.removeProcess(PID);
        auto chunk3 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk3, Ne(nullptr));
        EXPECT_THAT(chunk3->sample()->m_processList.size(), Eq(0U));

        // if there isn't any change, no data are deliverd
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), sendChunk(_)).Times(0);
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), stopOffer()).Times(1);
        introspectionAccess.send();
    }
}

TEST_F(ProcessIntrospection_test, thread)
{
    {
        const int PID = 42;
        const char PROCESS_NAME[] = "/chuck_norris";

        ProcessIntrospectionAccess introspectionAccess;

        introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection));

        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), tryAllocateChunk(_, _, _, _))
            .WillRepeatedly(
                Return(iox::cxx::expected<iox::mepoo::ChunkHeader*, iox::popo::AllocationError>::create_value(
                    m_chunk.get()->chunkHeader())));
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), hasSubscribers()).WillRepeatedly(Return(true));
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), offer()).Times(1);
        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), sendChunk(_)).Times(Between(2, 8));

        using namespace iox::units::duration_literals;
        introspectionAccess.setSendInterval(10_ms);
        introspectionAccess.run();

        for (size_t i = 0; i < 3; ++i)
        {
            introspectionAccess.addProcess(PID, iox::RuntimeName_t(PROCESS_NAME));
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            introspectionAccess.removeProcess(PID);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }

        introspectionAccess.stop();

        for (size_t i = 0; i < 3; ++i)
        // within this time, the thread should have sent the 6 updates
        {
            introspectionAccess.stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            introspectionAccess.addProcess(PID, iox::RuntimeName_t(PROCESS_NAME));
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            introspectionAccess.removeProcess(PID);
        }
    }
}

TEST_F(ProcessIntrospection_test, addRemoveNode)
{
    {
        ProcessIntrospectionAccess introspectionAccess;

        introspectionAccess.registerPublisherPort(std::move(m_mockPublisherPortUserIntrospection));

        const int PID = 42;
        const char PROCESS_NAME[] = "/chuck_norris";
        const char NODE_1[] = "the_wrecking_crew";
        const char NODE_2[] = "the_octagon";
        const char NODE_3[] = "the_hitman";

        // invalid removal of unknown node of unknown process
        introspectionAccess.removeNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_1));
        auto chunk1 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk1, Ne(nullptr));
        EXPECT_THAT(chunk1->sample()->m_processList.size(), Eq(0U));

        // a new process
        introspectionAccess.addProcess(PID, iox::RuntimeName_t(PROCESS_NAME));

        // invalid removal of unknown node of known process
        introspectionAccess.removeNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_1));
        auto chunk2 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk2, Ne(nullptr));
        EXPECT_THAT(chunk2->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk2->sample()->m_processList[0].m_nodes.size(), Eq(0U));

        // add a node
        introspectionAccess.addNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_1));
        auto chunk3 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk3, Ne(nullptr));
        EXPECT_THAT(chunk3->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk3->sample()->m_processList[0].m_nodes.size(), Eq(1U));

        // add it again, must be ignored
        introspectionAccess.addNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_1));
        auto chunk4 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk4, Ne(nullptr));
        EXPECT_THAT(chunk4->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk4->sample()->m_processList[0].m_nodes.size(), Eq(1U));

        // add some more
        introspectionAccess.addNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_2));
        introspectionAccess.addNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_3));
        auto chunk5 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk5, Ne(nullptr));
        EXPECT_THAT(chunk5->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk5->sample()->m_processList[0].m_nodes.size(), Eq(3U));

        // remove some nodes
        introspectionAccess.removeNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_1));
        introspectionAccess.removeNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_3));
        auto chunk6 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk6, Ne(nullptr));
        EXPECT_THAT(chunk6->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk6->sample()->m_processList[0].m_nodes.size(), Eq(1U));
        EXPECT_THAT(strcmp(NODE_2, chunk6->sample()->m_processList[0].m_nodes[0].c_str()), Eq(0));

        // remove last node, list empty again
        introspectionAccess.removeNode(iox::RuntimeName_t(PROCESS_NAME), iox::NodeName_t(NODE_2));
        auto chunk7 = createMemoryChunkAndSend(introspectionAccess);
        ASSERT_THAT(chunk7, Ne(nullptr));
        EXPECT_THAT(chunk7->sample()->m_processList.size(), Eq(1U));
        EXPECT_THAT(chunk7->sample()->m_processList[0].m_nodes.size(), Eq(0U));

        EXPECT_CALL(introspectionAccess.getPublisherPort().value(), stopOffer()).Times(1);
    }
}
