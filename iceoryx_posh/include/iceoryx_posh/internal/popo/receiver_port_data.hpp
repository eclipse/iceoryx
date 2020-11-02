// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_RECEIVER_PORT_DATA_HPP
#define IOX_POSH_POPO_RECEIVER_PORT_DATA_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/popo/delivery_fifo.hpp"
#include "iceoryx_posh/internal/popo/ports/base_port_data.hpp"
#include "iceoryx_posh/internal/popo/used_chunk_list.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_ptr.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

#include "iceoryx_utils/platform/platform_correction.hpp"

namespace iox
{
namespace popo
{
struct ReceiverPortData : public BasePortData
{
    using mutex_t = posix::mutex; // std::mutex
    using MemoryInfo = iox::mepoo::MemoryInfo;

    ReceiverPortData() noexcept;
    ReceiverPortData(const capro::ServiceDescription& serviceDescription,
                     const std::string& applicationName,
                     const MemoryInfo& memoryInfo = MemoryInfo()) noexcept;

    // Written by application, read by RouDi
    std::atomic_bool m_subscribeRequested{false};
    // Written by RouDi, read by application
    std::atomic<SubscribeState> m_subscriptionState{SubscribeState::NOT_SUBSCRIBED};

    DeliveryFiFo m_deliveryFiFo;
    static constexpr uint32_t DELIVERED_LIST_SIZE = 2u * MAX_SUBSCRIBER_QUEUE_CAPACITY;
    UsedChunkList<DELIVERED_LIST_SIZE> m_deliveredChunkList;

    // event callback related
    mutable std::atomic_bool m_chunkSendCallbackActive{false};
    mutable mutex_t m_chunkSendCallbackMutex{mutex_t::Recursive::OFF, mutex_t::Robust::ON};
    iox::relative_ptr<posix::Semaphore> m_chunkSendSemaphore{nullptr};

    // offer semaphore that is stored in shared memory
    iox_sem_t m_shmSemaphoreHandle;
    posix::Semaphore::result_t m_shmSemaphore = posix::Semaphore::create();

    bool m_notifyOverflow{false};
    std::atomic<uint64_t> m_overflowCounter{0u};

    MemoryInfo m_memoryInfo;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_RECEIVER_PORT_DATA_HPP
