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
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_POSH_POPO_PORTS_CLIENT_SERVER_PORT_TYPES_HPP
#define IOX_POSH_POPO_PORTS_CLIENT_SERVER_PORT_TYPES_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/locking_policy.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_pointer.hpp"

#include <cstdint>

namespace iox
{
namespace popo
{
struct ClientChunkDistributorConfig
{
    static constexpr uint32_t MAX_QUEUES = 1;
    static constexpr uint64_t MAX_HISTORY_CAPACITY = 1; // could be 0, but problem for the container then
};

struct ServerChunkDistributorConfig
{
    static constexpr uint32_t MAX_QUEUES = MAX_CLIENTS_PER_SERVER;
    static constexpr uint64_t MAX_HISTORY_CAPACITY = 1; // could be 0, but problem for the container then
};

struct ClientChunkQueueConfig
{
    static constexpr uint64_t MAX_QUEUE_CAPACITY = MAX_RESPONSE_QUEUE_CAPACITY;
};

struct ServerChunkQueueConfig
{
    static constexpr uint64_t MAX_QUEUE_CAPACITY = MAX_REQUEST_QUEUE_CAPACITY;
};

using ClientChunkQueueData_t = ChunkQueueData<ClientChunkQueueConfig, ThreadSafePolicy>;

using ServerChunkQueueData_t = ChunkQueueData<ServerChunkQueueConfig, ThreadSafePolicy>;

using ClientChunkDistributorData_t =
    ChunkDistributorData<ClientChunkDistributorConfig, ThreadSafePolicy, ChunkQueuePusher<ServerChunkQueueData_t>>;

using ServerChunkDistributorData_t =
    ChunkDistributorData<ServerChunkDistributorConfig, ThreadSafePolicy, ChunkQueuePusher<ClientChunkQueueData_t>>;

using ClientChunkReceiverData_t = ChunkReceiverData<MAX_RESPONSES_PROCESSED_SIMULTANEOUSLY, ClientChunkQueueData_t>;

using ServerChunkReceiverData_t = ChunkReceiverData<MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY, ServerChunkQueueData_t>;

using ClientChunkSenderData_t = ChunkSenderData<MAX_REQUESTS_ALLOCATED_SIMULTANEOUSLY, ClientChunkDistributorData_t>;

using ServerChunkSenderData_t = ChunkSenderData<MAX_RESPONSES_ALLOCATED_SIMULTANEOUSLY, ServerChunkDistributorData_t>;

class RPCBaseHeader
{
  public:
    explicit RPCBaseHeader(cxx::not_null<ClientChunkQueueData_t* const> chunkQueueDataPtr, const int64_t sequenceNumber)
        : m_clientQueueDataPtr(chunkQueueDataPtr)
        , m_sequenceNumber(sequenceNumber)
    {
    }

    RPCBaseHeader(const RPCBaseHeader& other) = delete;
    RPCBaseHeader& operator=(const RPCBaseHeader&) = delete;
    RPCBaseHeader(RPCBaseHeader&& rhs) = default;
    RPCBaseHeader& operator=(RPCBaseHeader&& rhs) = default;
    virtual ~RPCBaseHeader() = default;

    int64_t getSequenceNumber() const noexcept
    {
        return m_sequenceNumber;
    }

  protected:
    rp::RelativePointer<ClientChunkQueueData_t> m_clientQueueDataPtr;
    int64_t m_sequenceNumber{0};
};

class RequestHeader : public RPCBaseHeader
{
  public:
    explicit RequestHeader(cxx::not_null<ClientChunkQueueData_t* const> chunkQueueDataPtr) noexcept
        : RPCBaseHeader(chunkQueueDataPtr, 0)
    {
    }

    RequestHeader(const RequestHeader& other) = delete;
    RequestHeader& operator=(const RequestHeader&) = delete;
    RequestHeader(RequestHeader&& rhs) = default;
    RequestHeader& operator=(RequestHeader&& rhs) = default;
    virtual ~RequestHeader() = default;

    void setSequenceNumber(const int64_t sequenceNumber) noexcept
    {
        this->m_sequenceNumber = sequenceNumber;
    }

    void setFireAndForget(const bool fireAndForget) noexcept
    {
        m_isFireAndForget = fireAndForget;
    }

    mepoo::ChunkHeader* getChunkHeader() const noexcept
    {
        /// todo
        return nullptr;
    }
    void* getUserPayload() noexcept
    {
        /// todo
        return nullptr;
    }

  private:
    bool m_isFireAndForget{false};
};

class ResponseHeader : public RPCBaseHeader
{
  public:
    ResponseHeader(cxx::not_null<ClientChunkQueueData_t* const> chunkQueueDataPtr,
                   const int64_t sequenceNumber) noexcept
        : RPCBaseHeader(chunkQueueDataPtr, sequenceNumber)
    {
    }

    ResponseHeader(const ResponseHeader& other) = delete;
    ResponseHeader& operator=(const ResponseHeader&) = delete;
    ResponseHeader(ResponseHeader&& rhs) = default;
    ResponseHeader& operator=(ResponseHeader&& rhs) = default;
    virtual ~ResponseHeader() = default;

    void setServerError(bool serverError) noexcept
    {
        m_hasServerError = serverError;
    }

    bool hasServerError() const noexcept
    {
        return m_hasServerError;
    }

    const mepoo::ChunkHeader* getChunkHeader() const noexcept
    {
        /// todo
        return nullptr;
    }
    const void* getUserPayload() const noexcept
    {
        /// todo
        return nullptr;
    }

  private:
    bool m_hasServerError{false};
};


} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_PORTS_CLIENT_SERVER_PORT_TYPES_HPP
