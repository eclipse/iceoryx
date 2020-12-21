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
#include "iceoryx_dds/internal/log/logging.hpp"
#include "MempoolPubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>


iox::dds::FastDDSDataReader::FastDDSDataReader(
        iox::capro::IdString_t serviceId,
        iox::capro::IdString_t instanceId,
        iox::capro::IdString_t eventId) noexcept
    : m_serviceId(serviceId)
    , m_instanceId(instanceId)
    , m_eventId(eventId)
    , m_type(new Mempool::ChunkPubSubType())
    , m_listener(nullptr)
{
    m_listener.m_fastddsDataReader = this;
    LogDebug() << m_logName << "Created FastDDSDataReader";
}

iox::dds::FastDDSDataReader::~FastDDSDataReader()
{
    if (m_dataReader != nullptr)
    {
        m_subscriber->delete_datareader(m_dataReader);
    }
    if (m_subscriber != nullptr)
    {
        m_participant->delete_subscriber(m_subscriber);
    }
    if (m_topic != nullptr)
    {
        m_participant->delete_topic(m_topic);
    }
    if (m_participant != nullptr)
    {
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(m_participant);
    }
    LogDebug() << m_logName << "Destroyed FastDDSDataReader";
}

void iox::dds::FastDDSDataReader::connect() noexcept
{
    if (!m_isConnected.load(std::memory_order_relaxed))
    {
        // Create the DomainParticipant
        eprosima::fastdds::dds::DomainParticipantQos participantQos;
        m_participant =
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        // Register the type
        m_type.register_type(m_participant);

        // Create topic
        auto topicString =
            "/" + std::string(m_serviceId) + "/" + std::string(m_instanceId) + "/" + std::string(m_eventId);
        m_topic = m_participant->create_topic(topicString, "Mempool::Chunk", eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);

        // Create subscriber
        m_subscriber = m_participant->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);

        // Create datareader
        eprosima::fastdds::dds::DataReaderQos dataReaderQos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
        dataReaderQos.history().kind = eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS;
        dataReaderQos.endpoint().history_memory_policy =
            eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
        m_dataReader = m_subscriber->create_datareader(m_topic, dataReaderQos, &m_listener);

        // Save state
        m_isConnected.store(true, std::memory_order_relaxed);
        LogDebug() << m_logName << "Connected to topic: " << topicString;
    }
}

iox::cxx::optional<uint32_t> iox::dds::FastDDSDataReader::peekNextSize()
{
    // Get information about the next unread sample
    Mempool::Chunk data{0, nullptr};
    eprosima::fastdds::dds::SampleInfo nextInfo;
    if (eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK ==
        m_dataReader->read_next_sample(&data ,&nextInfo))
    {
        if (data.size != 0)
        {
            return iox::cxx::optional<uint32_t>(static_cast<uint64_t>(data.size));
        }
    }

    // no valid samples available
    return iox::cxx::nullopt_t();
}

bool iox::dds::FastDDSDataReader::hasNewSamples()
{
    eprosima::fastdds::dds::SampleInfo nextInfo;
    return (m_dataReader->get_first_untaken_info(&nextInfo) == eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
}

iox::cxx::expected<iox::dds::DataReaderError> iox::dds::FastDDSDataReader::takeNext(
        uint8_t* const buffer,
        const uint64_t& bufferSize)
{
    // validation checks
    if (!m_isConnected.load())
    {
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::NOT_CONNECTED);
    }
    if (buffer == nullptr)
    {
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::INVALID_RECV_BUFFER);
    }

    // take next sample and copy into buffer
    Mempool::Chunk data{static_cast<uint32_t>(bufferSize), buffer};
    eprosima::fastdds::dds::SampleInfo nextInfo;
    eprosima::fastrtps::types::ReturnCode_t ret = m_dataReader->take_next_sample(&data, &nextInfo);
    if (ret == eprosima::fastrtps::types::ReturnCode_t::RETCODE_NO_DATA)
    {
        // no samples available
        return iox::cxx::success<>();
    }
    else if (ret != eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK)
    {
        LogError() << m_logName << "take_next_sample returned and error";
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::INVALID_DATA);
    }

    // valid size
    auto sampleSize = data.size;
    if (sampleSize == 0)
    {
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::INVALID_DATA);
    }
    if (bufferSize < sampleSize)
    {
        // provided buffer is too small.
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::RECV_BUFFER_TOO_SMALL);
    }

    // copy data into the provided buffer
    auto bytes = data.payload;
    std::copy(bytes, bytes + sampleSize, const_cast<uint8_t*>(buffer));

    return iox::cxx::success<>();
}

iox::cxx::expected<uint64_t, iox::dds::DataReaderError> iox::dds::FastDDSDataReader::take(
        uint8_t* const buffer,
        const uint64_t& bufferSize,
        const iox::cxx::optional<uint64_t>& maxSamples)
{
    if (!m_isConnected.load())
    {
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::NOT_CONNECTED);
    }
    if (buffer == nullptr)
    {
        return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::INVALID_RECV_BUFFER);
    }

    // get size of the sample
    auto peekResult = peekNextSize();
    if (peekResult.has_value())
    {
        uint64_t sampleSize = peekResult.value();
        if (sampleSize == 0)
        {
            return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::INVALID_DATA);
        }
        if (bufferSize < sampleSize)
        {
            // Provided buffer is too small.
            return iox::cxx::error<iox::dds::DataReaderError>(iox::dds::DataReaderError::RECV_BUFFER_TOO_SMALL);
        }

        // take maximum amount possible up to the cap: maxSamples
        auto bufferCapacity = bufferSize / sampleSize;

        auto numToTake = bufferCapacity;
        if (maxSamples.has_value())
        {
            if (bufferCapacity > maxSamples.value())
            {
                numToTake = maxSamples.value();
            }
        }

        uint64_t cursor = 0;
        for (ulong i = 0; i < numToTake; i++)
        {
            if (!takeNext(&buffer[cursor], sampleSize))
            {
                break;
            }
            cursor += sampleSize;
        }

        uint64_t numSamplesBuffered = cursor / sampleSize;
        return iox::cxx::success<uint64_t>(numSamplesBuffered);
    }
    else
    {
        return iox::cxx::success<uint64_t>(0u);
    }
}

iox::capro::IdString_t iox::dds::FastDDSDataReader::getServiceId() const noexcept
{
    return m_serviceId;
}

iox::capro::IdString_t iox::dds::FastDDSDataReader::getInstanceId() const noexcept
{
    return m_instanceId;
}

iox::capro::IdString_t iox::dds::FastDDSDataReader::getEventId() const noexcept
{
    return m_eventId;
}

void iox::dds::FastDDSDataReader::waitForData(
        eprosima::fastrtps::Duration_t maxWait)
{
    m_dataReader->wait_for_unread_message(maxWait);
}

void iox::dds::FastDDSDataReader::waitForWriterDiscovery(
        uint16_t writersCount)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    m_writerDiscoveryCv.wait(
        lk,
        [&]
        {
            return (m_writerDiscoveryCount >= writersCount);
        });

    lk.unlock();
    m_writerDiscoveryCv.notify_one();
}


iox::dds::FastDDSDataReader::FastDDSDataReaderListener::FastDDSDataReaderListener(
        FastDDSDataReader* dataReader)
    : m_fastddsDataReader(dataReader)
{
}

iox::dds::FastDDSDataReader::FastDDSDataReaderListener::~FastDDSDataReaderListener()
{
}

void iox::dds::FastDDSDataReader::FastDDSDataReaderListener::on_subscription_matched(
        eprosima::fastdds::dds::DataReader*,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
{
    switch (info.current_count_change)
    {
        case 1:
            m_fastddsDataReader->m_writerDiscoveryCount++;
            m_fastddsDataReader->m_writerDiscoveryCv.notify_one();
            break;
        case -1:
            m_fastddsDataReader->m_writerDiscoveryCount--;
            m_fastddsDataReader->m_writerDiscoveryCv.notify_one();
            break;
        default:
            break;
    }
}
