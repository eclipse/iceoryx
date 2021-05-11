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

#ifndef IOX_EXAMPLES_ICEPERF_FOLLOWER_HPP
#define IOX_EXAMPLES_ICEPERF_FOLLOWER_HPP

#include "base.hpp"
#include "example_common.hpp"

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"

class IcePerfFollower
{
  public:
    IcePerfFollower() noexcept = default;

    int run() noexcept;

  private:
    PerfSettings getSettings(iox::popo::Subscriber<PerfSettings>& subscriber) noexcept;
    void doMeasurement(IcePerfBase& ipcTechnology) noexcept;

  private:
    PerfSettings m_settings;
};

#endif // IOX_EXAMPLES_ICEPERF_FOLLOWER_HPP
