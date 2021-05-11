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

# Build ice_access_control example
cmake_minimum_required(VERSION 3.5)
project(example_ice_access_control)

option(TOML_CONFIG "TOML support for RouDi with dynamic configuration" OFF)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
find_package(iceoryx_utils CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

add_executable(iox-cpp-radar iox_radar_app.cpp)

target_link_libraries(iox-cpp-radar
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-radar PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-display iox_display_app.cpp)

target_link_libraries(iox-cpp-display
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-display PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-cheeky iox_cheeky_app.cpp)

target_link_libraries(iox-cpp-cheeky
    iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-cheeky PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-cpp-roudi-static-segments
    roudi_main_static_segements.cpp
)

target_link_libraries(iox-cpp-roudi-static-segments
    PRIVATE
    iceoryx_utils::iceoryx_utils
    iceoryx_posh::iceoryx_posh_roudi
)

target_compile_options(iox-cpp-roudi-static-segments PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

set_target_properties(iox-cpp-radar iox-cpp-display iox-cpp-cheeky iox-cpp-roudi-static-segments PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)


install(
    TARGETS iox-cpp-radar iox-cpp-display iox-cpp-cheeky iox-cpp-roudi-static-segments
    RUNTIME DESTINATION bin
)
