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

set(IOX_VERSION_STRING "1.0.0")

project(iceoryx_introspection VERSION ${IOX_VERSION_STRING})

find_package(iceoryx_utils REQUIRED)
find_package(iceoryx_posh REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)

# cmake doesn't find ncurses under QNX; let the compiler complain if the header is missing
if(NOT QNX)
    SET(CURSES_USE_NCURSES TRUE)
    find_package(Curses REQUIRED)
    if(${CURSES_HAVE_NCURSES_H} MATCHES ".*-NOTFOUND")
        message(SEND_ERROR "ncurses header not found! Please install libncurses5-dev!")
    endif(${CURSES_HAVE_NCURSES_H} MATCHES ".*-NOTFOUND")
endif()

#
########## set variables for library export ##########
#
setup_package_name_and_create_files(
    NAME ${PROJECT_NAME}
    NAMESPACE iceoryx_introspection
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

add_library(iceoryx_introspection
    source/iceoryx_introspection_app.cpp
    source/introspection_app.cpp
)

add_library(${PROJECT_NAMESPACE}::iceoryx_introspection ALIAS iceoryx_introspection)

set_target_properties(iceoryx_introspection PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(iceoryx_introspection
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
    PRIVATE
    ${CURSES_INCLUDE_DIR}
)

target_link_libraries(iceoryx_introspection
    iceoryx_utils::iceoryx_utils
    iceoryx_posh::iceoryx_posh
    ncurses
)

target_compile_options(iceoryx_introspection PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

if(LINUX)
    set(LIB_TINFO tinfo)
elseif(WIN32)
    message(WARNING "Introspection not supported for windows." )
else()
    set(LIB_TINFO "")
endif()

target_link_libraries(iceoryx_introspection ${LIB_TINFO})

add_executable(iox-introspection-client source/introspection_main.cpp)

set_target_properties(iox-introspection-client PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(iox-introspection-client
    PRIVATE
    ${PROJECT_NAMESPACE}::iceoryx_introspection
)

target_compile_options(iox-introspection-client PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

#
########## exporting library ##########
#
setup_install_directories_and_export_package(
    TARGETS iceoryx_introspection iox-introspection-client
    INCLUDE_DIRECTORY include/
)

