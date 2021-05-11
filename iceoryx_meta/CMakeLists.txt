# Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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
project(iceoryx_meta)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

include(${CMAKE_CURRENT_SOURCE_DIR}/build_options.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/install_help_and_config.cmake)


# ===== Core
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_utils ${CMAKE_BINARY_DIR}/utils)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_posh ${CMAKE_BINARY_DIR}/posh)

# ===== Doxygen example
if(BUILD_DOC)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../doc/aspice_swe3_4 ${CMAKE_BINARY_DIR}/doc)
    return()
endif()


# ===== Extras
if(INTROSPECTION)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tools/introspection ${CMAKE_BINARY_DIR}/iceoryx_introspection)
endif()

# ===== Gateways
if(DDS_GATEWAY)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/cyclonedds ${CMAKE_BINARY_DIR}/dependencies/cyclonedds/prebuild)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_dds ${CMAKE_BINARY_DIR}/iceoryx_dds)
endif()

# ===== Language binding for C
if(BINDING_C)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_binding_c ${CMAKE_BINARY_DIR}/iceoryx_binding_c)
endif()

# ===== Examples
if(EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/icehello ${CMAKE_BINARY_DIR}/iceoryx_examples/icehello)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/icedelivery ${CMAKE_BINARY_DIR}/iceoryx_examples/icedelivery)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/iceoptions ${CMAKE_BINARY_DIR}/iceoryx_examples/iceoptions)
    if(BINDING_C)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/callbacks_in_c ${CMAKE_BINARY_DIR}/iceoryx_examples/callbacks_in_c)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/icedelivery_in_c ${CMAKE_BINARY_DIR}/iceoryx_examples/icedelivery_in_c)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/waitset_in_c ${CMAKE_BINARY_DIR}/iceoryx_examples/waitset_in_c)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/iceperf ${CMAKE_BINARY_DIR}/iceoryx_examples/iceperf)
    endif()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/waitset ${CMAKE_BINARY_DIR}/iceoryx_examples/waitset)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/callbacks ${CMAKE_BINARY_DIR}/iceoryx_examples/callbacks)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/singleprocess ${CMAKE_BINARY_DIR}/iceoryx_examples/singleprocess)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/ice_access_control ${CMAKE_BINARY_DIR}/iceoryx_examples/ice_access_control)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/complexdata ${CMAKE_BINARY_DIR}/iceoryx_examples/complexdata)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../iceoryx_examples/user_header ${CMAKE_BINARY_DIR}/iceoryx_examples/user_header)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/package/package.cmake)

show_config_options()

message("")
message("       Build Properties")
message("          project name..............: " ${CMAKE_PROJECT_NAME})
message("          c compiler................: " ${CMAKE_C_COMPILER})
message("          c++ compiler..............: " ${CMAKE_CXX_COMPILER})
message("")

