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

# Build complexdata example
cmake_minimum_required(VERSION 3.5)
project(example_complexdata)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

add_executable(iox-cpp-publisher-vector ./iox_publisher_vector.cpp)
target_link_libraries(iox-cpp-publisher-vector
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-publisher-vector PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-subscriber-vector ./iox_subscriber_vector.cpp)
target_link_libraries(iox-cpp-subscriber-vector
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-subscriber-vector PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-publisher-complexdata ./iox_publisher_complexdata.cpp)
target_link_libraries(iox-cpp-publisher-complexdata
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-publisher-complexdata PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-subscriber-complexdata ./iox_subscriber_complexdata.cpp)
target_link_libraries(iox-cpp-subscriber-complexdata
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-subscriber-complexdata PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

set_target_properties(iox-cpp-subscriber-complexdata iox-cpp-subscriber-vector
                      iox-cpp-publisher-complexdata iox-cpp-publisher-vector
                      PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

# ========================================================== //

install(TARGETS iox-cpp-publisher-complexdata iox-cpp-publisher-vector
        iox-cpp-subscriber-complexdata iox-cpp-subscriber-vector
        RUNTIME DESTINATION bin)

