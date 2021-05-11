# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

# configure deployment
message(STATUS "[i] <<<<<<<<<<<<< Start iceoryx_posh configuration: >>>>>>>>>>>>>")
if(ONE_TO_MANY_ONLY)
     message(STATUS "[i] Using 1:n communication only!")
     set(IOX_COMMUNICATION_POLICY OneToManyPolicy)
endif()

if(NOT IOX_COMMUNICATION_POLICY)
    message(STATUS "[i] Using m:n communication only!")
    set(IOX_COMMUNICATION_POLICY ManyToManyPolicy)
endif()

if(NOT IOX_MAX_PUBLISHERS)
    set(IOX_MAX_PUBLISHERS 512)
endif()
message(STATUS "[i] IOX_MAX_PUBLISHERS:" ${IOX_MAX_PUBLISHERS})

if(NOT IOX_MAX_SUBSCRIBERS)
    set(IOX_MAX_SUBSCRIBERS 1024)
endif()
message(STATUS "[i] IOX_MAX_SUBSCRIBERS:" ${IOX_MAX_SUBSCRIBERS})

if(NOT IOX_MAX_INTERFACE_NUMBER)
    set(IOX_MAX_INTERFACE_NUMBER 4)
endif()
message(STATUS "[i] IOX_MAX_INTERFACE_NUMBER:" ${IOX_MAX_INTERFACE_NUMBER})

if(NOT IOX_MAX_SUBSCRIBERS_PER_PUBLISHER)
    set(IOX_MAX_SUBSCRIBERS_PER_PUBLISHER 256)
endif()
message(STATUS "[i] IOX_MAX_SUBSCRIBERS_PER_PUBLISHER:" ${IOX_MAX_SUBSCRIBERS_PER_PUBLISHER})

if(NOT IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY)
    set(IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY 8)
endif()
message(STATUS "[i] IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY:" ${IOX_MAX_CHUNKS_ALLOCATED_PER_PUBLISHER_SIMULTANEOUSLY})

if(NOT IOX_MAX_PUBLISHER_HISTORY)
    set(IOX_MAX_PUBLISHER_HISTORY 16)
endif()
message(STATUS "[i] IOX_MAX_PUBLISHER_HISTORY:" ${IOX_MAX_PUBLISHER_HISTORY})

if(NOT IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY)
    set(IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY 256)
endif()
message(STATUS "[i] IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY:" ${IOX_MAX_CHUNKS_HELD_PER_SUBSCRIBER_SIMULTANEOUSLY})
message(STATUS "[i] <<<<<<<<<<<<<< End iceoryx_posh configuration: >>>>>>>>>>>>>>")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/iceoryx_posh_deployment.hpp.in"
  "${CMAKE_BINARY_DIR}/generated/iceoryx/include/iceoryx_posh/iceoryx_posh_deployment.hpp" @ONLY)
