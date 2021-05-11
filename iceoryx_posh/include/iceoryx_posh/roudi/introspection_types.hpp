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
#ifndef IOX_POSH_ROUDI_INTROSPECTION_TYPES_HPP
#define IOX_POSH_ROUDI_INTROSPECTION_TYPES_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

namespace iox
{
namespace roudi
{
constexpr const char INTROSPECTION_SERVICE_ID[] = "Introspection";
constexpr const char INTROSPECTION_APP_NAME[] = "introspection";
constexpr const char INTROSPECTION_NODE_NAME[] = "introspection";
const capro::ServiceDescription IntrospectionMempoolService(INTROSPECTION_SERVICE_ID, "RouDi_ID", "MemPool");
constexpr int MAX_GROUP_NAME_LENGTH = 32;

/// @brief struct for the storage of mempool usage information.
/// This data container is used by the introstpection::MemPoolInfoContainer array
/// to store information on all available memmpools.
struct MemPoolInfo
{
    uint32_t m_usedChunks{0};
    uint32_t m_minFreeChunks{0};
    uint32_t m_numChunks{0};
    uint32_t m_chunkSize{0};
    uint32_t m_chunkPayloadSize{0};
};

/// @brief container for MemPoolInfo structs of all available mempools.
using MemPoolInfoContainer = cxx::vector<MemPoolInfo, MAX_NUMBER_OF_MEMPOOLS>;

/// @brief the topic for the mempool introspection that a user can subscribe to
struct MemPoolIntrospectionInfo
{
    uint32_t m_id;
    cxx::string<MAX_GROUP_NAME_LENGTH> m_writerGroupName;
    cxx::string<MAX_GROUP_NAME_LENGTH> m_readerGroupName;
    MemPoolInfoContainer m_mempoolInfo;
};

/// @brief container for MemPoolInfo structs of all available mempools.
using MemPoolIntrospectionInfoContainer = cxx::vector<MemPoolIntrospectionInfo, MAX_SHM_SEGMENTS + 1>;

/// @brief publisher/subscriber port information consisting of a process name,a capro service description string
/// and a node name
const capro::ServiceDescription IntrospectionPortService(INTROSPECTION_SERVICE_ID, "RouDi_ID", "Port");

/// @brief container for common port data which is related to the subscriber port as well as the publisher port
struct PortData
{
    RuntimeName_t m_name;
    capro::IdString_t m_caproInstanceID;
    capro::IdString_t m_caproServiceID;
    capro::IdString_t m_caproEventMethodID;
    NodeName_t m_node;
};

using SubscriberPortData = PortData;

/// @brief container for publisher port introspection data.
struct PublisherPortData : public PortData
{
    uint64_t m_publisherPortID{0};
    iox::capro::Interfaces m_sourceInterface{iox::capro::Interfaces::INTERFACE_END};
};

/// @brief the topic for the port introspection that a user can subscribe to
struct PortIntrospectionFieldTopic
{
    cxx::vector<SubscriberPortData, MAX_SUBSCRIBERS> m_subscriberList;
    cxx::vector<PublisherPortData, MAX_PUBLISHERS> m_publisherList;
};

const capro::ServiceDescription
    IntrospectionPortThroughputService(INTROSPECTION_SERVICE_ID, "RouDi_ID", "PortThroughput");

struct PortThroughputData
{
    uint64_t m_publisherPortID{0};
    uint32_t m_sampleSize{0};
    uint32_t m_chunkSize{0};
    double m_chunksPerMinute{0};
    uint64_t m_lastSendIntervalInNanoseconds{0};
    bool m_isField{false};
};

/// @brief the topic for the port throughput that a user can subscribe to
struct PortThroughputIntrospectionFieldTopic
{
    cxx::vector<PortThroughputData, MAX_PUBLISHERS> m_throughputList;
};

const capro::ServiceDescription
    IntrospectionSubscriberPortChangingDataService(INTROSPECTION_SERVICE_ID, "RouDi_ID", "SubscriberPortsData");

struct SubscriberPortChangingData
{
    // index used to identify subscriber is same as in PortIntrospectionFieldTopic->subscriberList
    uint64_t fifoSize{0};
    uint64_t fifoCapacity{0};
    iox::SubscribeState subscriptionState{iox::SubscribeState::NOT_SUBSCRIBED};
    capro::Scope propagationScope{capro::Scope::INVALID};
};

struct SubscriberPortChangingIntrospectionFieldTopic
{
    cxx::vector<SubscriberPortChangingData, MAX_SUBSCRIBERS> subscriberPortChangingDataList;
};

const capro::ServiceDescription IntrospectionProcessService(INTROSPECTION_SERVICE_ID, "RouDi_ID", "Process");

struct ProcessIntrospectionData
{
    int m_pid{0};
    RuntimeName_t m_name;
    cxx::vector<NodeName_t, MAX_NODE_PER_PROCESS> m_nodes;
};

/// @brief the topic for the process introspection that a user can subscribe to
struct ProcessIntrospectionFieldTopic
{
    cxx::vector<ProcessIntrospectionData, MAX_PROCESS_NUMBER> m_processList;
};

} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_INTROSPECTION_TYPES_HPP
