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

#include "iceoryx_dds/dds/fastdds_data_writer.hpp"
#include "iceoryx_dds/internal/log/logging.hpp"
#include "MempoolPubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <string>

iox::dds::FastDDSDataWriter::FastDDSDataWriter(
        iox::capro::IdString_t serviceId,
        iox::capro::IdString_t instanceId,
        iox::capro::IdString_t eventId)
    : m_serviceId(serviceId)
    , m_instanceId(instanceId)
    , m_eventId(eventId)
    , m_type(new Mempool::ChunkPubSubType())
    , m_listener(nullptr)
{
    m_listener.fastddsDataWriter = this;
    LogDebug() << m_logName << "Created FastDDSDataWriter";
}

iox::dds::FastDDSDataWriter::~FastDDSDataWriter()
{
    if (m_dataWriter != nullptr)
    {
        m_publisher->delete_datawriter(m_dataWriter);
    }
    if (m_topic != nullptr)
    {
        m_participant->delete_topic(m_topic);
    }
    if (m_publisher != nullptr)
    {
        m_participant->delete_publisher(m_publisher);
    }
    if (m_participant != nullptr)
    {
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(m_participant);
    }
    LogDebug() << m_logName << "Destroyed FastDDSDataWriter";
}

void iox::dds::FastDDSDataWriter::connect() noexcept
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
        auto m_topicname =
            "/" + std::string(m_serviceId) + "/" + std::string(m_instanceId) + "/" + std::string(m_eventId);
        m_topic = m_participant->create_topic(m_topicname, "Mempool::Chunk", eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);

        // Create publisher
        m_publisher = m_participant->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr);

        // Create datawriter
        eprosima::fastdds::dds::DataWriterQos dataWriterQos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
        dataWriterQos.endpoint().history_memory_policy =
            eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
        m_dataWriter = m_publisher->create_datawriter(m_topic, dataWriterQos, &m_listener);

        // Save state
        m_isConnected.store(true, std::memory_order_relaxed);
        LogDebug() << m_logName << "Connected to topic: " << m_topicname;
    }
}

void iox::dds::FastDDSDataWriter::write(
        const uint8_t* const bytes,
        const uint64_t size) noexcept
{
    try
    {
        Mempool::Chunk data{static_cast<uint32_t>(size), const_cast<uint8_t*>(bytes)};
        if (!m_dataWriter->write(&data))
        {
            LogError() << m_logName << "Cannot send data";
        }
    }
    catch(const std::exception& e)
    {
        LogError() << m_logName << "write exception: " << e.what();
    }
}

iox::capro::IdString_t iox::dds::FastDDSDataWriter::getServiceId() const noexcept
{
    return m_serviceId;
}

iox::capro::IdString_t iox::dds::FastDDSDataWriter::getInstanceId() const noexcept
{
    return m_instanceId;
}

iox::capro::IdString_t iox::dds::FastDDSDataWriter::getEventId() const noexcept
{
    return m_eventId;
}

void iox::dds::FastDDSDataWriter::waitForReaderDiscovery(
        uint16_t readersCount)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    m_readerDiscoveryCv.wait(
        lk,
        [&]
        {
            return (m_readerDiscoveryCount >= readersCount);
        });

    lk.unlock();
    m_readerDiscoveryCv.notify_one();
}

iox::dds::FastDDSDataWriter::FastDDSDataWriterListener::FastDDSDataWriterListener(
        FastDDSDataWriter* dataWriter)
    : fastddsDataWriter(dataWriter)
{
}

iox::dds::FastDDSDataWriter::FastDDSDataWriterListener::~FastDDSDataWriterListener()
{
}

void iox::dds::FastDDSDataWriter::FastDDSDataWriterListener::on_publication_matched(
        eprosima::fastdds::dds::DataWriter*,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    switch (info.current_count_change)
    {
        case 1:
            fastddsDataWriter->m_readerDiscoveryCount++;
            fastddsDataWriter->m_readerDiscoveryCv.notify_one();
            break;
        case -1:
            fastddsDataWriter->m_readerDiscoveryCount--;
            fastddsDataWriter->m_readerDiscoveryCv.notify_one();
            break;
        default:
            break;
    }
}
