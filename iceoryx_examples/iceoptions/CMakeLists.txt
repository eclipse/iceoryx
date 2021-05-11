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

# Build iceoptions example
cmake_minimum_required(VERSION 3.5)
project(example_iceoptions)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

add_executable(iox-cpp-publisher-with-options ./iox_publisher_with_options.cpp)
target_link_libraries(iox-cpp-publisher-with-options
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-publisher-with-options PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-subscriber-with-options ./iox_subscriber_with_options.cpp)
target_link_libraries(iox-cpp-subscriber-with-options
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-subscriber-with-options PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})


set_target_properties(iox-cpp-publisher-with-options iox-cpp-subscriber-with-options
                      PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

# ========================================================== //

install(TARGETS iox-cpp-publisher-with-options iox-cpp-subscriber-with-options
        RUNTIME DESTINATION bin)
