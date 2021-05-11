// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#ifndef IOX_BINDING_C_CPP2C_PUBLISHER_H
#define IOX_BINDING_C_CPP2C_PUBLISHER_H

#include "iceoryx_posh/internal/popo/ports/publisher_port_user.hpp"

struct cpp2c_Publisher
{
    cpp2c_Publisher() = default;
    cpp2c_Publisher(const cpp2c_Publisher&) = delete;
    cpp2c_Publisher(cpp2c_Publisher&& rhs) noexcept;
    ~cpp2c_Publisher();

    cpp2c_Publisher& operator=(const cpp2c_Publisher&) = delete;
    cpp2c_Publisher& operator=(cpp2c_Publisher&& rhs) noexcept;

    iox::popo::PublisherPortData* m_portData{nullptr};
};

#endif
