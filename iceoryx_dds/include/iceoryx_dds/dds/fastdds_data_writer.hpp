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

#ifndef IOX_DDS_DDS_FASTDDS_DATA_WRITER_HPP
#define IOX_DDS_DDS_FASTDDS_DATA_WRITER_HPP

#include "iceoryx_dds/dds/data_writer.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace iox
{
namespace dds
{
///
/// @brief Implementation of the DataWriter abstraction using the Fast DDS implementation.
///
class FastDDSDataWriter : public iox::dds::DataWriter
{
  public:
    FastDDSDataWriter() = delete;


    FastDDSDataWriter(
        const capro::IdString_t serviceId,
        const capro::IdString_t instanceId,
        const capro::IdString_t eventId);

    virtual ~FastDDSDataWriter();

    FastDDSDataWriter(
        const FastDDSDataWriter&) = delete;

    FastDDSDataWriter& operator=(
        const FastDDSDataWriter&) = delete;

    FastDDSDataWriter(
        FastDDSDataWriter&&) = default;

    FastDDSDataWriter& operator=(
        FastDDSDataWriter&&) = default;

    void connect() noexcept override;

    void write(
        const uint8_t* const bytes,
        const uint64_t size) noexcept override;

    capro::IdString_t getServiceId() const noexcept override;

    capro::IdString_t getInstanceId() const noexcept override;

    capro::IdString_t getEventId() const noexcept override;

    ///
    /// @brief Block thread until there are at least readersCount discovered DataReaders
    /// @param[in] readersCount The minimum number of discovered DataReaders (default: 1).
    ///
    void waitForReaderDiscovery(
        uint16_t readersCount=1);

  private:

    capro::IdString_t m_serviceId{""};

    capro::IdString_t m_instanceId{""};

    capro::IdString_t m_eventId{""};

    std::atomic_bool m_isConnected{false};

    eprosima::fastdds::dds::TypeSupport m_type;

    eprosima::fastdds::dds::Topic* m_topic;

    eprosima::fastdds::dds::DomainParticipant* m_participant = nullptr;

    eprosima::fastdds::dds::Publisher* m_publisher = nullptr;

    eprosima::fastdds::dds::DataWriter* m_dataWriter = nullptr;

    std::condition_variable m_readerDiscoveryCv;

    std::atomic<uint16_t> m_readerDiscoveryCount{0};

    std::mutex m_mutex;

    const std::string m_logName = "[FastDDSDataWriter] ";

    class FastDDSDataWriterListener : public eprosima::fastdds::dds::DataWriterListener
    {
    public:

      FastDDSDataWriterListener(
          FastDDSDataWriter* dataWriter);

      ~FastDDSDataWriterListener() override;

      void on_publication_matched(
          eprosima::fastdds::dds::DataWriter*,
          const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

      FastDDSDataWriter* fastddsDataWriter;

    } m_listener;

};

} // namespace dds
} // namespace iox

#endif // IOX_DDS_DDS_FASTDDS_DATA_WRITER_HPP
