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
#ifndef IOX_POSH_ICEORYX_POSH_TYPES_HPP
#define IOX_POSH_ICEORYX_POSH_TYPES_HPP

#include "iceoryx_posh/iceoryx_posh_deployment.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/variant_queue.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/posix_wrapper/ipc_channel.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_utils/log/logstream.hpp"

#include <cstdint>

namespace iox
{
namespace popo
{
template <typename>
class TypedUniqueId;
struct BasePortData;

class PublisherPortRouDi;
class PublisherPortUser;
class SubscriberPortRouDi;
class SubscriberPortUser;
} // namespace popo
namespace posix
{
class UnixDomainSocket;
} // namespace posix

using PublisherPortRouDiType = iox::popo::PublisherPortRouDi;
using PublisherPortUserType = iox::popo::PublisherPortUser;
using SubscriberPortRouDiType = iox::popo::SubscriberPortRouDi;
using SubscriberPortUserType = iox::popo::SubscriberPortUser;
using UniquePortId = popo::TypedUniqueId<popo::BasePortData>;

using SubscriberPortType = iox::build::CommunicationPolicy;

/// @brief The socket is created in the current path if no absolute path is given hence
///      we need an absolut path so that every application knows where our sockets can
///      be found.
using IpcChannelType = iox::posix::UnixDomainSocket;

/// @todo remove MAX_RECEIVERS_PER_SENDERPORT when the new port building blocks are used
constexpr uint32_t MAX_RECEIVERS_PER_SENDERPORT = build::IOX_MAX_SUBSCRIBERS_PER_PUBLISHER;

//--------- Communication Resources Start---------------------
// Publisher
constexpr uint32_t MAX_PUBLISHERS = build::IOX_MAX_PUBLISHERS;
constexpr uint32_t MAX_SUBSCRIBERS_PER_PUBLISHER = build::IOX_MAX_SUBSCRIBERS_PER_PUBLISHER;
constexpr uint32_t MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY =
    build::IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY;
constexpr uint64_t MAX_PUBLISHER_HISTORY = build::IOX_MAX_PUBLISHER_HISTORY;
// Subscriber
constexpr uint32_t MAX_SUBSCRIBERS = build::IOX_MAX_SUBSCRIBERS;
constexpr uint32_t MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY =
    build::IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY;
constexpr uint32_t MAX_SUBSCRIBER_QUEUE_CAPACITY = MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY;
// Introspection is using the following publisherPorts, which reduced the number of ports available for the user
// 1x publisherPort mempool introspection
// 1x publisherPort process introspection
// 3x publisherPort port introspection
constexpr uint32_t PUBLISHERS_RESERVED_FOR_INTROSPECTION = 5;
/// With MAX_SUBSCRIBER_QUEUE_CAPACITY = MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY we couple the maximum number of
/// chunks a user is allowed to hold with the maximum queue capacity. This allows that a polling user can replace all
/// the held chunks in one execution with all new ones from a completely filled queue. Or the other way round, when we
/// have a contract with the user regarding how many chunks they are allowed to hold, then the queue size needs not be
/// bigger. We can provide this number of newest chunks, more the user would not be allowed to hold anyway
// Gateway
constexpr uint32_t MAX_INTERFACE_NUMBER = build::IOX_MAX_INTERFACE_NUMBER;
constexpr uint32_t MAX_INTERFACE_CAPRO_FIFO_SIZE = MAX_PUBLISHERS;
constexpr uint32_t MAX_CHANNEL_NUMBER = MAX_PUBLISHERS + MAX_SUBSCRIBERS;
constexpr uint32_t MAX_GATEWAY_SERVICES = 2 * MAX_CHANNEL_NUMBER;
// Client
constexpr uint32_t MAX_CLIENTS = build::IOX_MAX_SUBSCRIBERS; /// @todo
constexpr uint32_t MAX_REQUESTS_ALLOCATED_SIMULTANEOUSLY = 4U;
constexpr uint32_t MAX_RESPONSES_PROCESSED_SIMULTANEOUSLY = 16U;
constexpr uint32_t MAX_RESPONSE_QUEUE_CAPACITY = 16U;
// Server
constexpr uint32_t MAX_SERVERS = build::IOX_MAX_PUBLISHERS; /// @todo
constexpr uint32_t MAX_CLIENTS_PER_SERVER = 256U;
constexpr uint32_t MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY = 4U;
constexpr uint32_t MAX_RESPONSES_ALLOCATED_SIMULTANEOUSLY = MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY;
constexpr uint32_t MAX_REQUEST_QUEUE_CAPACITY = 1024;
// Waitset
namespace popo
{
using WaitSetIsConditionSatisfiedCallback = cxx::ConstMethodCallback<bool>;
}
constexpr uint32_t MAX_NUMBER_OF_CONDITION_VARIABLES = 1024U;
constexpr uint32_t MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE = 128U;
constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET = 128U;
static_assert(MAX_NUMBER_OF_ATTACHMENTS_PER_WAITSET <= MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE,
              "The WaitSet capacity is restricted by the maximum amount of notifiers per condition variable.");
// Listener
constexpr uint8_t MAX_NUMBER_OF_EVENT_VARIABLES = 128U;
constexpr uint8_t MAX_NUMBER_OF_EVENTS_PER_LISTENER = 128U;
static_assert(MAX_NUMBER_OF_EVENTS_PER_LISTENER <= MAX_NUMBER_OF_NOTIFIERS_PER_CONDITION_VARIABLE,
              "The Listener capacity is restricted by the maximum amount of notifiers per condition variable.");
//--------- Communication Resources End---------------------

constexpr uint32_t MAX_APPLICATION_CAPRO_FIFO_SIZE = 128U;

// Memory
constexpr uint32_t MAX_NUMBER_OF_MEMPOOLS = 32U;
constexpr uint32_t MAX_SHM_SEGMENTS = 100U;

constexpr uint32_t MAX_NUMBER_OF_MEMORY_PROVIDER = 8U;
constexpr uint32_t MAX_NUMBER_OF_MEMORY_BLOCKS_PER_MEMORY_PROVIDER = 64U;

constexpr uint32_t CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT{8U};
constexpr uint32_t CHUNK_NO_USER_HEADER_SIZE{0U};
constexpr uint32_t CHUNK_NO_USER_HEADER_ALIGNMENT{1U};

// Message Queue
constexpr uint32_t ROUDI_MAX_MESSAGES = 5U;
constexpr uint32_t ROUDI_MESSAGE_SIZE = 512U;
constexpr uint32_t APP_MAX_MESSAGES = 5U;
constexpr uint32_t APP_MESSAGE_SIZE = 512U;


// Processes
constexpr uint32_t MAX_PROCESS_NUMBER = 300U;
/// Maximum number of instances of a given service, which can be found.
/// This limitation is coming due to the fixed capacity of the cxx::vector (This doesn't limit the offered number of
/// instances)
constexpr uint32_t MAX_NUMBER_OF_INSTANCES = 50U;

// Nodes
constexpr uint32_t MAX_NODE_NUMBER = 1000U;
constexpr uint32_t MAX_NODE_PER_PROCESS = 50U;

constexpr uint32_t MAX_RUNTIME_NAME_LENGTH = MAX_IPC_CHANNEL_NAME_LENGTH;


static_assert(MAX_PROCESS_NUMBER * MAX_NODE_PER_PROCESS > MAX_NODE_NUMBER, "Invalid configuration for nodes");

enum class SubscribeState : uint32_t
{
    NOT_SUBSCRIBED = 0,
    SUBSCRIBE_REQUESTED,
    SUBSCRIBED,
    UNSUBSCRIBE_REQUESTED,
    WAIT_FOR_OFFER
};

enum class ConnectionState : uint32_t
{
    NOT_CONNECTED = 0,
    CONNECT_REQUESTED,
    CONNNECTED,
    DISCONNECT_REQUESTED,
    WAIT_FOR_OFFER
};

// Default properties of ChunkDistributorData
struct DefaultChunkDistributorConfig
{
    static constexpr uint32_t MAX_QUEUES = MAX_SUBSCRIBERS_PER_PUBLISHER;
    static constexpr uint64_t MAX_HISTORY_CAPACITY = MAX_PUBLISHER_HISTORY;
};

// Default properties of ChunkQueueData
struct DefaultChunkQueueConfig
{
    static constexpr uint64_t MAX_QUEUE_CAPACITY = MAX_SUBSCRIBER_QUEUE_CAPACITY;
};

// alias for cxx::string
using RuntimeName_t = cxx::string<MAX_RUNTIME_NAME_LENGTH>;
using NodeName_t = cxx::string<100>;
using ShmName_t = cxx::string<128>;

namespace capro
{
using IdString_t = cxx::string<100>;
}

/// @todo Move everything in this namespace to iceoryx_roudi_types.hpp once we move RouDi to a separate CMake target
namespace roudi
{
using ConfigFilePathString_t = cxx::string<1024>;

constexpr const char ROUDI_LOCK_NAME[] = "iox-unique-roudi";
constexpr const char IPC_CHANNEL_ROUDI_NAME[] = "roudi";

/// shared memmory segment for the iceoryx managment data
constexpr const char SHM_NAME[] = "/iceoryx_mgmt";

// Timeout
using namespace units::duration_literals;
constexpr units::Duration PROCESS_DEFAULT_KILL_DELAY = 45_s;
constexpr units::Duration PROCESS_TERMINATED_CHECK_INTERVAL = 250_ms;
constexpr units::Duration DISCOVERY_INTERVAL = 100_ms;

/// @brief Controls process alive monitoring. Upon timeout, a monitored process is removed
/// and its resources are made available. The process can then start and register itself again.
/// Contrarily, unmonitored processes can be restarted but registration will fail.
/// Once Runlevel Management is extended, it will detect absent processes. Those processes can register again.
/// ON - all processes are monitored
/// OFF - no process is monitored
enum class MonitoringMode
{
    ON,
    OFF
};

iox::log::LogStream& operator<<(iox::log::LogStream& logstream, const MonitoringMode& mode);
} // namespace roudi

namespace mepoo
{
using SequenceNumber_t = std::uint64_t;
using BaseClock_t = std::chrono::steady_clock;

// use signed integer for duration;
// there is a bug in gcc 4.8 which leads to a wrong calcutated time
// when sleep_until() is used with a timepoint in the past
using DurationNs_t = std::chrono::duration<std::int64_t, std::nano>;
using TimePointNs_t = std::chrono::time_point<BaseClock_t, DurationNs_t>;
} // namespace mepoo

namespace runtime
{
using InstanceContainer = iox::cxx::vector<capro::IdString_t, MAX_NUMBER_OF_INSTANCES>;
using namespace units::duration_literals;
constexpr units::Duration PROCESS_WAITING_FOR_ROUDI_TIMEOUT = 60_s;
constexpr units::Duration PROCESS_KEEP_ALIVE_INTERVAL = 3 * roudi::DISCOVERY_INTERVAL;  // > DISCOVERY_INTERVAL
constexpr units::Duration PROCESS_KEEP_ALIVE_TIMEOUT = 5 * PROCESS_KEEP_ALIVE_INTERVAL; // > PROCESS_KEEP_ALIVE_INTERVAL
} // namespace runtime

namespace version
{
static const uint64_t COMMIT_ID_STRING_SIZE = 12u;
using CommitIdString_t = cxx::string<COMMIT_ID_STRING_SIZE>;
static const uint64_t BUILD_DATE_STRING_SIZE = 36u;
using BuildDateString_t = cxx::string<BUILD_DATE_STRING_SIZE>;
} // namespace version

} // namespace iox

#include "iceoryx_posh/iceoryx_posh_types.inl"

#endif // IOX_POSH_ICEORYX_POSH_TYPES_HPP
