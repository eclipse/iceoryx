# Copyright (c) 2019, 2020 by Robert Bosch GmbH. All rights reserved.
# Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

# Build icedelivery example
cmake_minimum_required(VERSION 3.5)
project(example_icedelivery_in_c)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

if("-Wno-noexcept-type" IN_LIST ICEORYX_WARNINGS)
    list(REMOVE_ITEM ICEORYX_WARNINGS "-Wno-noexcept-type")
endif()

add_executable(iox-c-publisher ./ice_c_publisher.c)
set_source_files_properties(./ice_c_publisher.c PROPERTIES LANGUAGE C)
target_link_libraries(iox-c-publisher
    iceoryx_binding_c::iceoryx_binding_c
)
set_target_properties(iox-c-publisher PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_options(iox-c-publisher PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-c-subscriber ./ice_c_subscriber.c)
set_source_files_properties(./ice_c_subscriber.c PROPERTIES LANGUAGE C)
target_link_libraries(iox-c-subscriber
    iceoryx_binding_c::iceoryx_binding_c
)
set_target_properties(iox-c-subscriber PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_options(iox-c-subscriber PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

install(
    TARGETS iox-c-publisher iox-c-subscriber
    RUNTIME DESTINATION bin
)
