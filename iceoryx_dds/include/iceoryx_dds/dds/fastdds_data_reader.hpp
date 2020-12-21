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

#ifndef IOX_DDS_DDS_FASTDDS_DATA_READER_HPP
#define IOX_DDS_DDS_FASTDDS_DATA_READER_HPP

#include "iceoryx_dds/dds/data_reader.hpp"

#include <atomic>
#include <condition_variable>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace iox
{
namespace dds
{
///
/// @brief Implementation of the DataReader abstraction using the Fast DDS implementation.
///
class FastDDSDataReader : public DataReader
{
  public:

    FastDDSDataReader() = delete;

    FastDDSDataReader(
        capro::IdString_t serviceId,
        capro::IdString_t instanceId,
        capro::IdString_t eventId) noexcept;

    virtual ~FastDDSDataReader();

    FastDDSDataReader(
        const FastDDSDataReader&) = delete;

    FastDDSDataReader& operator=(
        const FastDDSDataReader&) = delete;

    FastDDSDataReader(
        FastDDSDataReader&&) = delete;

    FastDDSDataReader& operator=(
        FastDDSDataReader&&) = delete;

    void connect() noexcept override;

    iox::cxx::optional<uint32_t> peekNextSize() override;

    bool hasNewSamples() override;

    iox::cxx::expected<DataReaderError> takeNext(
        uint8_t* const buffer,
        const uint64_t& bufferSize) override;

    iox::cxx::expected<uint64_t, DataReaderError>take(
        uint8_t* const buffer,
        const uint64_t& bufferSize,
        const iox::cxx::optional<uint64_t>& maxSamples) override;

    capro::IdString_t getServiceId() const noexcept override;

    capro::IdString_t getInstanceId() const noexcept override;

    capro::IdString_t getEventId() const noexcept override;

    ///
    /// @brief Block thread until there is new data, or until maxWait expires.
    /// @param[in] maxWait The maximum time to block thread (default: 10 s).
    ///
    void waitForData(
        eprosima::fastrtps::Duration_t maxWait=eprosima::fastrtps::Duration_t(10, 0));

    ///
    /// @brief Block thread until there are at least writersCount discovered DataWriters
    /// @param[in] writersCount The minimum number of discovered DataWriters (default: 1).
    ///
    void waitForWriterDiscovery(
        uint16_t writersCount=1);

  protected:

    capro::IdString_t m_serviceId{""};

    capro::IdString_t m_instanceId{""};

    capro::IdString_t m_eventId{""};

    std::atomic_bool m_isConnected{false};

    eprosima::fastdds::dds::TypeSupport m_type;

    eprosima::fastdds::dds::Topic* m_topic = nullptr;

    eprosima::fastdds::dds::DomainParticipant* m_participant = nullptr;

    eprosima::fastdds::dds::Subscriber* m_subscriber = nullptr;

    eprosima::fastdds::dds::DataReader* m_dataReader = nullptr;

    std::condition_variable m_writerDiscoveryCv;

    std::atomic<uint16_t> m_writerDiscoveryCount{0};

    std::mutex m_mutex;

    const std::string m_logName = "[FastDDSDataReader] ";

    class FastDDSDataReaderListener : public eprosima::fastdds::dds::DataReaderListener
    {
    public:

      FastDDSDataReaderListener(
          FastDDSDataReader* dataReader);

      ~FastDDSDataReaderListener() override;

      void on_subscription_matched(
          eprosima::fastdds::dds::DataReader*,
          const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

      FastDDSDataReader* m_fastddsDataReader;

    } m_listener;

};

} // namespace dds
} // namespace iox

#endif // IOX_DDS_DDS_FASTDDS_DATA_READER_HPP
