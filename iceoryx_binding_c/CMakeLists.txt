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
cmake_minimum_required(VERSION 3.7)

set(IOX_VERSION_STRING "1.0.0")



project(iceoryx_binding_c VERSION ${IOX_VERSION_STRING})

find_package(iceoryx_utils REQUIRED)
find_package(iceoryx_posh REQUIRED)


include(IceoryxPackageHelper)
include(IceoryxPlatform)

if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME MATCHES Darwin)
    option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)
endif()

#
########## set variables for export ##########
#
setup_package_name_and_create_files(
    NAME ${PROJECT_NAME}
    NAMESPACE iceoryx_binding_c
    PROJECT_PREFIX ${PREFIX}
)

#
########## find_package in source tree ##########
#
set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake
    CACHE FILEPATH
    "${PROJECT_NAME}Config.cmake to make find_package(${PROJECT_NAME}) work in source tree!"
    FORCE
)

#
########## build building-block library ##########
#
add_library(${PROJECT_NAME}
    source/c_notification_info.cpp
    source/c_listener.cpp
    source/c_node.cpp
    source/c_publisher.cpp
    source/c_runtime.cpp
    source/c_subscriber.cpp
    source/c_user_trigger.cpp
    source/c_wait_set.cpp
    source/c_chunk.cpp
    source/c2cpp_enum_translation.cpp
    source/c_log.cpp
    source/cpp2c_enum_translation.cpp
    source/cpp2c_publisher.cpp
    source/cpp2c_subscriber.cpp
    source/cpp2c_service_description_translation.cpp
)
add_library(${PROJECT_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
)

if(NOT (WIN32 OR QNX))
    target_link_libraries(${PROJECT_NAME}
      PUBLIC
        stdc++
        pthread
      PRIVATE
        iceoryx_posh::iceoryx_posh
        iceoryx_utils::iceoryx_utils
    )
else()
    target_link_libraries(${PROJECT_NAME}
      PRIVATE
        iceoryx_posh::iceoryx_posh
        iceoryx_utils::iceoryx_utils
    )
endif()

target_compile_options(${PROJECT_NAME} PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

#
########## build test executables ##########
#
if(BUILD_TEST)
    add_subdirectory(test)
endif()

#
########## export library ##########
#
setup_install_directories_and_export_package(
    TARGETS ${PROJECT_NAME}
    INCLUDE_DIRECTORY include/
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  DESTINATION share/doc/iceoryx_binding_c
  COMPONENT dev)

