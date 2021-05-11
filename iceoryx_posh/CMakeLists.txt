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

set(IOX_VERSION_STRING "1.0.0")

project(iceoryx_posh VERSION ${IOX_VERSION_STRING})

find_package(Threads REQUIRED)
find_package(iceoryx_utils REQUIRED)

option(TOML_CONFIG "TOML support for RouDi with dynamic configuration" ON)
option(ONE_TO_MANY_ONLY "Restricts communication to 1:n pattern" OFF)

if(TOML_CONFIG)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpptoml/ ${CMAKE_BINARY_DIR}/dependencies/posh/cpptoml/prebuild)

    find_package(cpptoml REQUIRED)
endif()

include(cmake/iceoryxversions.cmake)
include(IceoryxPackageHelper)
include(IceoryxPlatform)

include(cmake/iceoryx_posh_deployment.cmake)

if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME MATCHES Darwin)
    option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)
endif()


if(CLANG_TIDY)
    find_program(
        CLANG_TIDY_EXE
        NAMES "clang-tidy"
    )

    if(CLANG_TIDY_EXE)
        set(PERFORM_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else(CLANG_TIDY_EXE)
        message(WARNING "clang-tidy activated but unable to find clang-tidy executable")
    endif()
endif()

#
########## set variables for library export ##########
#
setup_package_name_and_create_files(
    NAME ${PROJECT_NAME}
    NAMESPACE iceoryx_posh
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
########## posh core lib ##########
#
add_library(iceoryx_posh
    source/log/posh_logging.cpp
    source/capro/capro_message.cpp
    source/capro/service_description.cpp
    source/mepoo/chunk_header.cpp
    source/mepoo/chunk_management.cpp
    source/mepoo/chunk_settings.cpp
    source/mepoo/mepoo_config.cpp
    source/mepoo/segment_config.cpp
    source/mepoo/memory_manager.cpp
    source/mepoo/mem_pool.cpp
    source/mepoo/shared_chunk.cpp
    source/mepoo/shm_safe_unmanaged_chunk.cpp
    source/mepoo/segment_manager.cpp
    source/mepoo/mepoo_segment.cpp
    source/mepoo/memory_info.cpp
    source/popo/ports/interface_port.cpp
    source/popo/ports/interface_port_data.cpp
    source/popo/ports/application_port.cpp
    source/popo/ports/application_port_data.cpp
    source/popo/ports/base_port_data.cpp
    source/popo/ports/base_port.cpp
    source/popo/ports/publisher_port_data.cpp
    source/popo/ports/publisher_port_user.cpp
    source/popo/ports/publisher_port_roudi.cpp
    source/popo/ports/subscriber_port_user.cpp
    source/popo/ports/subscriber_port_roudi.cpp
    source/popo/ports/subscriber_port_single_producer.cpp
    source/popo/ports/subscriber_port_multi_producer.cpp
    source/popo/ports/subscriber_port_data.cpp
    source/popo/ports/client_port_data.cpp
    source/popo/ports/client_port_roudi.cpp
    source/popo/ports/client_port_user.cpp
    source/popo/ports/server_port_data.cpp
    source/popo/ports/server_port_roudi.cpp
    source/popo/ports/server_port_user.cpp
    source/popo/building_blocks/condition_listener.cpp
    source/popo/building_blocks/condition_notifier.cpp
    source/popo/building_blocks/condition_variable_data.cpp
    source/popo/building_blocks/locking_policy.cpp
    source/popo/building_blocks/typed_unique_id.cpp
    source/popo/listener.cpp
    source/popo/notification_info.cpp
    source/popo/trigger.cpp
    source/popo/trigger_handle.cpp
    source/popo/user_trigger.cpp
    source/version/version_info.cpp
    source/runtime/ipc_interface_base.cpp
    source/runtime/ipc_interface_user.cpp
    source/runtime/ipc_interface_creator.cpp
    source/runtime/ipc_runtime_interface.cpp
    source/runtime/ipc_message.cpp
    source/runtime/port_config_info.cpp
    source/runtime/posh_runtime.cpp
    source/runtime/posh_runtime_single_process.cpp
    source/runtime/node.cpp
    source/runtime/node_data.cpp
    source/runtime/node_property.cpp
    source/runtime/shared_memory_user.cpp
)

add_library(${PROJECT_NAMESPACE}::iceoryx_posh ALIAS iceoryx_posh)

set_target_properties(iceoryx_posh PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(PERFORM_CLANG_TIDY)
    set_target_properties(
        iceoryx_posh PROPERTIES CXX_CLANG_TIDY "${PERFORM_CLANG_TIDY}"
    )
endif(PERFORM_CLANG_TIDY)

target_include_directories(iceoryx_posh
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/iceoryx/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
    PRIVATE
    source/capro
    source/log
    source/mepoo
    source/popo
    source/version
    source/runtime
)
target_link_libraries(iceoryx_posh
    PUBLIC
    iceoryx_utils::iceoryx_utils
    PRIVATE
    ${ICEORYX_SANITIZER_FLAGS}
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_options(iceoryx_posh PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

if(LINUX)
    target_link_libraries(iceoryx_posh PRIVATE rt )
elseif(QNX)
elseif(APPLE)
elseif(WIN32)
else()
    message(WARNING "Could not detect supported platform, but I'm feeling lucky today." )
endif()

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  DESTINATION share/doc/iceoryx_posh
  COMPONENT dev)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/3rd_party_licenses
  DESTINATION share/doc/iceoryx_posh
  COMPONENT dev)


#
########## posh lib for gateway support ##########
#
add_library(iceoryx_posh_gateway
    source/gateway/gateway_base.cpp
)
add_library(${PROJECT_NAMESPACE}::iceoryx_posh_gateway ALIAS iceoryx_posh_gateway)

set_target_properties(iceoryx_posh_gateway PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(iceoryx_posh_gateway
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/iceoryx/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dependencies/install/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
)

target_link_libraries(iceoryx_posh_gateway
    PUBLIC
    iceoryx_utils::iceoryx_utils
    iceoryx_posh::iceoryx_posh
    PRIVATE
    ${ICEORYX_SANITIZER_FLAGS}
)

target_compile_options(iceoryx_posh_gateway PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})


#
########## posh roudi lib ##########
#
add_library(iceoryx_posh_roudi
    source/roudi/application/iceoryx_roudi_app.cpp
    source/roudi/application/roudi_app.cpp
    source/roudi/memory/memory_block.cpp
    source/roudi/memory/memory_provider.cpp
    source/roudi/memory/mempool_collection_memory_block.cpp
    source/roudi/memory/mempool_segment_manager_memory_block.cpp
    source/roudi/memory/port_pool_memory_block.cpp
    source/roudi/memory/posix_shm_memory_provider.cpp
    source/roudi/memory/default_roudi_memory.cpp
    source/roudi/memory/roudi_memory_manager.cpp
    source/roudi/memory/iceoryx_roudi_memory_manager.cpp
    source/roudi/port_manager.cpp
    source/roudi/port_pool.cpp
    source/roudi/roudi.cpp
    source/roudi/process.cpp
    source/roudi/process_manager.cpp
    source/roudi/service_registry.cpp
    source/roudi/iceoryx_roudi_components.cpp
    source/roudi/roudi_cmd_line_parser.cpp
    source/roudi/roudi_cmd_line_parser_config_file_option.cpp
    source/roudi/roudi_config.cpp
    )

add_library(${PROJECT_NAMESPACE}::iceoryx_posh_roudi ALIAS iceoryx_posh_roudi)

set_target_properties(iceoryx_posh_roudi PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(PERFORM_CLANG_TIDY)
    set_target_properties(
        iceoryx_posh_roudi PROPERTIES CXX_CLANG_TIDY "${PERFORM_CLANG_TIDY}"
    )
endif()

target_include_directories(iceoryx_posh_roudi PRIVATE
    source/runtime
    source/version
    source/popo
    source/capro
    source/roudi
)

target_link_libraries(iceoryx_posh_roudi
    PUBLIC
    ${PROJECT_NAMESPACE}::iceoryx_posh     # because of roudi.hpp
    PRIVATE
    ${ICEORYX_SANITIZER_FLAGS}
    iceoryx_utils::iceoryx_utils
)

if(CMAKE_SYSTEM_NAME MATCHES QNX)
    target_link_libraries(iceoryx_posh_roudi PRIVATE socket)
endif(CMAKE_SYSTEM_NAME MATCHES QNX)

target_compile_options(iceoryx_posh_roudi PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})


if(TOML_CONFIG)
    #
    ######### posh config lib ##########
    #
    add_library(iceoryx_posh_config
        source/log/posh_config_logging.cpp
        source/gateway/gateway_config.cpp
        source/gateway/toml_gateway_config_parser.cpp
        source/roudi/roudi_config_toml_file_provider.cpp
    )
    add_library(${PROJECT_NAMESPACE}::iceoryx_posh_config ALIAS iceoryx_posh_config)

    set_target_properties(iceoryx_posh_config PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        POSITION_INDEPENDENT_CODE ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    target_include_directories(iceoryx_posh_config PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/iceoryx/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dependencies/install/include>
        $<INSTALL_INTERFACE:include/${PREFIX}>
    )

    target_link_libraries(iceoryx_posh_config PRIVATE
        iceoryx_posh::iceoryx_posh
        iceoryx_utils::iceoryx_utils
        iceoryx_posh::iceoryx_posh_roudi
        cpptoml
        ${ICEORYX_SANITIZER_FLAGS}
    )

    target_compile_options(iceoryx_posh_config PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/etc/iceoryx/roudi_config_example.toml
        DESTINATION etc/
        COMPONENT dev)
#
######### posh roudi daemon ##########
#

    add_executable(iox-roudi
        source/roudi/application/roudi_main.cpp
    )

    set_target_properties(iox-roudi PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        POSITION_INDEPENDENT_CODE ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    target_link_libraries(iox-roudi
        PRIVATE
        iceoryx_utils::iceoryx_utils
        ${PROJECT_NAMESPACE}::iceoryx_posh_roudi
        ${PROJECT_NAMESPACE}::iceoryx_posh_config
    )

    target_include_directories(iox-roudi PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dependencies/install/include>
        $<INSTALL_INTERFACE:include/${PREFIX}>
    )

    target_compile_options(iox-roudi PRIVATE ${ICEORYX_WARNINGS})

    if(PERFORM_CLANG_TIDY)
        set_target_properties(
            iox-roudi PROPERTIES CXX_CLANG_TIDY "${PERFORM_CLANG_TIDY}"
        )
    endif()

endif()

#
########## exporting library ##########
#
if(TOML_CONFIG)
    set(ROUDI_EXPORT iceoryx_posh_config iox-roudi)
endif()

setup_install_directories_and_export_package(
    TARGETS iceoryx_posh iceoryx_posh_roudi iceoryx_posh_gateway ${ROUDI_EXPORT}
    INCLUDE_DIRECTORY include/
)

install(
    FILES ${CMAKE_BINARY_DIR}/generated/iceoryx/include/${PROJECT_NAME}/iceoryx_posh_deployment.hpp
    DESTINATION include/${PREFIX}/${PROJECT_NAME}/
    COMPONENT dev
)

if(ROUDI_ENVIRONMENT OR BUILD_TEST)
    #
    ######### posh roudi environment ##########
    #
    setup_package_name_and_create_files(
        NAME ${PROJECT_NAME}_roudi_environment
        NAMESPACE iceoryx_posh_testing
        PROJECT_PREFIX ${PREFIX}
    )

    add_library(iceoryx_posh_testing
        STATIC
        testing/roudi_environment/runtime_test_interface.cpp
        testing/roudi_environment/roudi_environment.cpp
    )

    add_library(iceoryx_posh_testing::iceoryx_posh_testing ALIAS iceoryx_posh_testing)
    set_target_properties(iceoryx_posh_testing PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        POSITION_INDEPENDENT_CODE ON
    )

    target_include_directories(iceoryx_posh_testing
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/testing/include>
        $<INSTALL_INTERFACE:include/${PREFIX}>
    )

    target_link_libraries(iceoryx_posh_testing
        PRIVATE
        iceoryx_posh::iceoryx_posh
        iceoryx_utils::iceoryx_utils
        iceoryx_posh::iceoryx_posh_roudi
    )

    target_compile_options(iceoryx_posh_testing PRIVATE ${ICEORYX_WARNINGS})

    setup_install_directories_and_export_package(
        TARGETS iceoryx_posh_testing
        INCLUDE_DIRECTORY testing/include/
    )

    #
    ########## find_package in source tree ##########
    #
    set(${PROJECT_NAME}_testing_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake
    CACHE FILEPATH
    "${PROJECT_NAME}_testingConfig.cmake to make find_package(${PROJECT_NAME}_testing) work in source tree!"
    FORCE
    )
endif()

if(BUILD_TEST)
    add_subdirectory(test)
endif()
