// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_dds/dds/fastdds_data_reader.hpp"
#include "iceoryx_dds/dds/fastdds_data_writer.hpp"
#include "test.hpp"

#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <chrono>

using namespace ::testing;
using ::testing::_;

namespace iox
{
namespace dds
{
// ======================================== Helpers ======================================== //
using TestDataReader = FastDDSDataReader;
using TestDataWriter = FastDDSDataWriter;

// ======================================== Fixture ======================================== //
class FastDDSTest : public Test
{
  public:
    void SetUp(){};
    void TearDown(){};
};

// ======================================== Tests ======================================== //
TEST_F(FastDDSTest, DoesNotAttemptToReadWhenDisconnected)
{
    // ===== Setup
    uint64_t bufferSize = 1024;
    uint8_t buffer[bufferSize];

    // ===== Test
    TestDataReader reader{"", "", ""};

    auto takeResult = reader.take(buffer, bufferSize, iox::cxx::nullopt);
    EXPECT_EQ(true, takeResult.has_error());
    EXPECT_EQ(iox::dds::DataReaderError::NOT_CONNECTED, takeResult.get_error());

    auto takeNextResult = reader.takeNext(buffer, bufferSize);
    EXPECT_EQ(true, takeNextResult.has_error());
    EXPECT_EQ(iox::dds::DataReaderError::NOT_CONNECTED, takeResult.get_error());
}

TEST_F(FastDDSTest, ReturnsErrorWhenAttemptingToReadIntoANullBuffer)
{
    // ===== Setup
    uint64_t bufferSize = 0;
    uint8_t* buffer = nullptr;

    // ===== Test
    TestDataReader reader{"", "", ""};
    reader.connect();

    auto takeResult = reader.take(buffer, bufferSize, iox::cxx::nullopt);
    EXPECT_EQ(true, takeResult.has_error());
    EXPECT_EQ(iox::dds::DataReaderError::INVALID_RECV_BUFFER, takeResult.get_error());

    auto takeNextResult = reader.takeNext(buffer, bufferSize);
    EXPECT_EQ(true, takeNextResult.has_error());
    EXPECT_EQ(iox::dds::DataReaderError::INVALID_RECV_BUFFER, takeNextResult.get_error());
}

TEST_F(FastDDSTest, SendAndTakeNext)
{
    uint64_t bufferSize = 1024;

    /* Create reader */
    TestDataReader reader{"test", "test", "test"};
    reader.connect();

    /* Create writer */
    TestDataWriter writer{"test", "test", "test"};
    writer.connect();

    /* Wait for discovery */
    writer.waitForReaderDiscovery(1);
    reader.waitForWriterDiscovery(1);

    /* Setup send buffer */
    uint8_t sendBuffer[bufferSize];
    // Populate send buffer with random values
    std::srand(static_cast<uint>(std::time(nullptr)));
    for (uint64_t i = 0; i < bufferSize; i++)
    {
        sendBuffer[i] = static_cast<uint8_t>(std::rand() % 255);
    }

    /* Send data */
    writer.write(sendBuffer, bufferSize);

    /* Wait for delivery */
    reader.waitForData();

    /* Setup receive buffer */
    uint8_t receiveBuffer[bufferSize];
    for (uint64_t i = 0; i < bufferSize; i++)
    {
        receiveBuffer[i] = 0;
    }

    /* Take data */
    auto result = reader.takeNext(receiveBuffer, bufferSize);

    /* Check correctness */
    EXPECT_EQ(false, result.has_error());
    for (uint64_t i = 0; i < bufferSize; i++)
    {
        ASSERT_EQ(sendBuffer[i], receiveBuffer[i]);
    }
}

TEST_F(FastDDSTest, SendAndTake)
{
    uint64_t bufferSize = 1024;

    /* Create reader */
    TestDataReader reader{"test", "test", "test"};
    reader.connect();

    /* Create writer */
    TestDataWriter writer{"test", "test", "test"};
    writer.connect();

    /* Wait for discovery */
    writer.waitForReaderDiscovery(1);
    reader.waitForWriterDiscovery(1);

    /* Setup send buffers */
    uint8_t sendBuffer1[bufferSize];
    // Populate send buffer with random values
    std::srand(static_cast<uint>(std::time(nullptr)));
    for (uint64_t i = 0; i < bufferSize; i++)
    {
        sendBuffer1[i] = static_cast<uint8_t>(std::rand() % 255);
    }

    uint8_t sendBuffer2[bufferSize];
    for (uint64_t i = 0; i < bufferSize; i++)
    {
        sendBuffer2[i] = static_cast<uint8_t>(std::rand() % 255);
    }

    /* Send data */
    writer.write(sendBuffer1, bufferSize);
    writer.write(sendBuffer2, bufferSize);

    /* Wait for delivery */
    reader.waitForData();

    /* Setup receive buffer */
    uint8_t receiveBuffer[bufferSize*2];
    for (uint64_t i = 0; i < bufferSize*2; i++)
    {
        receiveBuffer[i] = 0;
    }

    /* Take data */
    auto result = reader.take(receiveBuffer, bufferSize*2, 2);

    /* Check correctness */
    EXPECT_EQ(false, result.has_error());
    for (uint64_t i = 0; i < bufferSize*2; i++)
    {
        if (i < bufferSize)
        {
            ASSERT_EQ(sendBuffer1[i], receiveBuffer[i]);
        }
        else
        {
            ASSERT_EQ(sendBuffer2[i - bufferSize], receiveBuffer[i]);
        }
    }
}

} // namespace dds
} // namespace iox
