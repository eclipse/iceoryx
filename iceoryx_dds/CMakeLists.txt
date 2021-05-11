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

project(iceoryx_dds VERSION ${IOX_VERSION_STRING})

if(NOT cpptoml_FOUND)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpptoml/ ${CMAKE_BINARY_DIR}/dependencies/dds_gateway/cpptoml/prebuild)

    find_package(cpptoml REQUIRED)
endif()

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
    NAMESPACE iceoryx_dds
    PROJECT_PREFIX ${PREFIX}
)

#
########## feature flags ##########
#
option(USE_CYCLONE_DDS "Bind to CycloneDDS implementation" ON)

if(USE_CYCLONE_DDS)
    message(INFO " Using CycloneDDS stack")
    find_package(CycloneDDS CONFIG REQUIRED)
    find_package(CycloneDDS-CXX CONFIG REQUIRED)
endif()

#
########## build building-block library ##########
#
add_library(iceoryx_dds
    source/iceoryx_dds/log/logging.cpp
)
add_library(${PROJECT_NAMESPACE}::iceoryx_dds ALIAS iceoryx_dds)

set_target_properties(iceoryx_utils PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(iceoryx_dds
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dependencies/install/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
)
target_link_libraries(iceoryx_dds
    PRIVATE
    iceoryx_posh::iceoryx_posh
    iceoryx_posh::iceoryx_posh_config
    iceoryx_posh::iceoryx_posh_gateway
    iceoryx_utils::iceoryx_utils
)

if(USE_CYCLONE_DDS)
    target_sources(iceoryx_dds
        PRIVATE
        source/iceoryx_dds/dds/cyclone_context.cpp
        source/iceoryx_dds/dds/cyclone_data_reader.cpp
        source/iceoryx_dds/dds/cyclone_data_writer.cpp
        source/iceoryx_dds/dds/iox_chunk_datagram_header.cpp
    )

    # Generate IDL at configure time
    set(IDLPP_GENERATE_DIR ${CMAKE_BINARY_DIR}/iceoryx_dds_messages)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/idlpp-cxx-generate.cmake.in ${IDLPP_GENERATE_DIR}/CMakeLists.txt COPYONLY)
    execute_process(COMMAND ${CMAKE_COMMAND} "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" -DMESSAGE_DEFINITION_DIR=${CMAKE_CURRENT_SOURCE_DIR}/msg/ "${IDLPP_GENERATE_DIR}"
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${IDLPP_GENERATE_DIR} )
    if(result)
        message(FATAL_ERROR "Unable to configure IDL.")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${IDLPP_GENERATE_DIR})
    if(result)
        message(FATAL_ERROR "Unable to generate IDL.")
    endif()

    target_compile_definitions(iceoryx_dds PUBLIC -DUSE_CYCLONE_DDS)
    target_include_directories(iceoryx_dds
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/iceoryx_dds_messages>
    )
    target_link_libraries(iceoryx_dds
        PUBLIC
        CycloneDDS-CXX::ddscxx
        ${IDLPP_GENERATE_DIR}/libiceoryx_dds_messages.a
    )
endif()

target_compile_options(iceoryx_dds PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

#
########## build gateway apps ##########
#
add_executable(iox-gw-iceoryx2dds
    source/iceoryx2dds_app/main.cpp
)
target_link_libraries(iox-gw-iceoryx2dds
    iceoryx_posh::iceoryx_posh
    iceoryx_posh::iceoryx_posh_gateway
    iceoryx_posh::iceoryx_posh_config
    ${PROJECT_NAMESPACE}::iceoryx_dds
)

target_compile_options(iox-gw-iceoryx2dds PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-gw-dds2iceoryx 
    source/dds2iceoryx_app/main.cpp
)
target_link_libraries(iox-gw-dds2iceoryx 
    iceoryx_posh::iceoryx_posh
    iceoryx_posh::iceoryx_posh_gateway
    iceoryx_posh::iceoryx_posh_config
    ${PROJECT_NAMESPACE}::iceoryx_dds
)

target_compile_options(iox-gw-dds2iceoryx PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

set_target_properties(iox-gw-iceoryx2dds iox-gw-dds2iceoryx iceoryx_dds 
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
)

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
    TARGETS iceoryx_dds iox-gw-iceoryx2dds iox-gw-dds2iceoryx
    INCLUDE_DIRECTORY include/
)

