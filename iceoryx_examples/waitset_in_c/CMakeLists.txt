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
cmake_minimum_required(VERSION 3.5)
project(example_waitset_in_c)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

if("-Wno-noexcept-type" IN_LIST ICEORYX_WARNINGS)
    list(REMOVE_ITEM ICEORYX_WARNINGS "-Wno-noexcept-type")
endif()

add_executable(iox-c-waitset-publisher ./ice_c_waitset_publisher.c)
target_link_libraries(iox-c-waitset-publisher
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iox-c-waitset-publisher PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-c-waitset-gateway ./ice_c_waitset_gateway.c)
target_link_libraries(iox-c-waitset-gateway
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iox-c-waitset-gateway PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-c-waitset-grouping ./ice_c_waitset_grouping.c)
target_link_libraries(iox-c-waitset-grouping
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iox-c-waitset-grouping PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-c-waitset-individual ./ice_c_waitset_individual.c)
target_link_libraries(iox-c-waitset-individual
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iox-c-waitset-individual PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-c-waitset-sync ./ice_c_waitset_sync.c)
target_link_libraries(iox-c-waitset-sync
    iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(iox-c-waitset-sync PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

set_source_files_properties(
    ./ice_c_waitset_sync.c
    ./ice_c_waitset_individual.c
    ./ice_c_waitset_grouping.c
    ./ice_c_waitset_gateway.c
    ./ice_c_waitset_publisher.c
  PROPERTIES 
    LANGUAGE C
)

set_target_properties(
    iox-c-waitset-sync
    iox-c-waitset-individual
    iox-c-waitset-grouping
    iox-c-waitset-gateway
    iox-c-waitset-publisher
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

install(
    TARGETS iox-c-waitset-publisher iox-c-waitset-gateway iox-c-waitset-grouping iox-c-waitset-individual iox-c-waitset-sync
    RUNTIME DESTINATION bin
)
