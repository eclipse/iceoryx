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
#ifndef IOX_EXAMPLES_ICEPERF_ICEORYX_C_HPP
#define IOX_EXAMPLES_ICEPERF_ICEORYX_C_HPP

#include "base.hpp"
#include "iceoryx_posh/capro/service_description.hpp"

extern "C" {
#include "iceoryx_binding_c/publisher.h"
#include "iceoryx_binding_c/subscriber.h"
}

class IceoryxC : public IcePerfBase
{
  public:
    IceoryxC(const iox::capro::IdString_t& publisherName, const iox::capro::IdString_t& subscriberName) noexcept;
    ~IceoryxC();
    void initLeader() noexcept override;
    void initFollower() noexcept override;
    void shutdown() noexcept override;

  private:
    void init() noexcept;
    void sendPerfTopic(const uint32_t payloadSizeInBytes, const RunFlag runFlag) noexcept override;
    PerfTopic receivePerfTopic() noexcept override;

    iox_pub_storage_t m_publisherStorage;
    iox_sub_storage_t m_subscriberStorage;
    iox_pub_t m_publisher;
    iox_sub_t m_subscriber;
};

#endif // IOX_EXAMPLES_ICEPERF_ICEORYX_HPP
