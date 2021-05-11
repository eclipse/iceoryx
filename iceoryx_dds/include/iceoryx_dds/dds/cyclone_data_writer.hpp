// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_DDS_DDS_CYCLONE_DATA_WRITER_HPP
#define IOX_DDS_DDS_CYCLONE_DATA_WRITER_HPP

#include "iceoryx_dds/dds/data_writer.hpp"

#include <Mempool_DCPS.hpp>
#include <dds/dds.hpp>

namespace iox
{
namespace dds
{
///
/// @brief Implementation of the DataWriter abstraction using the cyclonedds implementation.
///
class CycloneDataWriter : public iox::dds::DataWriter
{
  public:
    CycloneDataWriter() = delete;
    CycloneDataWriter(const capro::IdString_t serviceId,
                      const capro::IdString_t instanceId,
                      const capro::IdString_t eventId) noexcept;
    virtual ~CycloneDataWriter();
    CycloneDataWriter(const CycloneDataWriter&) = delete;
    CycloneDataWriter& operator=(const CycloneDataWriter&) = delete;
    // Required for vector
    CycloneDataWriter(CycloneDataWriter&&) = default;
    CycloneDataWriter& operator=(CycloneDataWriter&&) = default;

    void connect() noexcept override;
    void write(iox::dds::IoxChunkDatagramHeader datagramHeader,
               const uint8_t* const userHeaderBytes,
               const uint8_t* const userPayloadBytes) noexcept override;
    capro::IdString_t getServiceId() const noexcept override;
    capro::IdString_t getInstanceId() const noexcept override;
    capro::IdString_t getEventId() const noexcept override;

  private:
    capro::IdString_t m_serviceId{""};
    capro::IdString_t m_instanceId{""};
    capro::IdString_t m_eventId{""};

    ::dds::pub::Publisher m_publisher = ::dds::core::null;
    ::dds::topic::Topic<Mempool::Chunk> m_topic = ::dds::core::null;
    ::dds::pub::DataWriter<Mempool::Chunk> m_writer = ::dds::core::null;
};

} // namespace dds
} // namespace iox

#endif // IOX_DDS_DDS_CYCLONE_DATA_WRITER_HPP
