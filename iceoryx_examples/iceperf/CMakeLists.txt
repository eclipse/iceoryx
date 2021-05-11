# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
# Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

# Build iceperf example
cmake_minimum_required(VERSION 3.5)
project(example_iceperf)

option(TOML_CONFIG "TOML support for RouDi with dynamic configuration" ON)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

add_executable(iceperf-bench-leader main_leader.cpp iceperf_leader.cpp base.cpp iceoryx.cpp iceoryx_c.cpp uds.cpp mq.cpp)

target_link_libraries(iceperf-bench-leader
    iceoryx_posh::iceoryx_posh
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iceperf-bench-leader PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})
if(CMAKE_SYSTEM_NAME MATCHES QNX)
    target_link_libraries(iceperf-bench-leader socket)
endif()

add_executable(iceperf-bench-follower main_follower.cpp iceperf_follower.cpp base.cpp iceoryx.cpp iceoryx_c.cpp uds.cpp mq.cpp)

target_link_libraries(iceperf-bench-follower
    iceoryx_posh::iceoryx_posh
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iceperf-bench-follower PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})
if(CMAKE_SYSTEM_NAME MATCHES QNX)
    target_link_libraries(iceperf-bench-follower socket)
endif()

add_executable(iceperf-roudi
    roudi_main_static_config.cpp
)

target_link_libraries(iceperf-roudi
    PRIVATE
    iceoryx_utils::iceoryx_utils
    iceoryx_posh::iceoryx_posh_roudi
)

target_compile_options(iceperf-roudi PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

set_target_properties(iceperf-bench-leader iceperf-bench-follower iceperf-roudi PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

install(
    TARGETS iceperf-bench-leader iceperf-bench-leader iceperf-roudi
    RUNTIME DESTINATION bin
)
