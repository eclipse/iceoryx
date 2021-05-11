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
set(test_iceoryx_posh_VERSION 0)
project(test_iceoryx_posh VERSION ${test_iceoryx_posh_VERSION})

find_package(iceoryx_utils_testing REQUIRED)
find_package(iceoryx_posh_testing REQUIRED)

if(BUILD_TEST AND NOT GTest_FOUND)
    find_package(GTest CONFIG REQUIRED)
endif(BUILD_TEST AND NOT GTest_FOUND)

set(PROJECT_PREFIX "posh")

# Set path for input files in test
SET(POSH_TEST_INPUT_FILES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/input/)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/moduletests/test_input_path.hpp.in
               ${CMAKE_BINARY_DIR}/generated/iceoryx/tests/posh/moduletests/test_input_path.hpp)


file(GLOB_RECURSE MODULETESTS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/moduletests/*.cpp")
file(GLOB_RECURSE INTEGRATIONTESTS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/integrationtests/*.cpp")
file(GLOB_RECURSE COMPONENTTESTS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/componenttests/*.cpp")
file(GLOB_RECURSE MOCKS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mocks/*.cpp")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_PREFIX}/test)

set(TEST_LINK_LIBS
    ${CODE_COVERAGE_LIBS}
    GTest::gtest
    GTest::gmock
    iceoryx_utils::iceoryx_utils
    iceoryx_utils::iceoryx_platform
    iceoryx_utils_testing::iceoryx_utils_testing
    iceoryx_posh::iceoryx_posh
    iceoryx_posh::iceoryx_posh_config
    iceoryx_posh::iceoryx_posh_gateway
    iceoryx_posh::iceoryx_posh_roudi
    iceoryx_posh_testing::iceoryx_posh_testing
)

if(LINUX)
    set(TEST_LINK_LIBS ${TEST_LINK_LIBS} dl )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(TEST_CXX_FLAGS ${ICEORYX_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS} -Wno-pedantic -Wno-conversion)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS} -Wno-pedantic -Wno-conversion)
endif()

# unittests
add_executable(${PROJECT_PREFIX}_moduletests ${MODULETESTS_SRC} ${MOCKS_SRC})
target_include_directories(${PROJECT_PREFIX}_moduletests PRIVATE . ${CMAKE_BINARY_DIR}/generated)
# TODO: fix conversion warnings
target_compile_options(${PROJECT_PREFIX}_moduletests PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(${PROJECT_PREFIX}_moduletests ${TEST_LINK_LIBS})
set_target_properties(${PROJECT_PREFIX}_moduletests PROPERTIES 
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

# integration tests
add_executable( ${PROJECT_PREFIX}_integrationtests ${INTEGRATIONTESTS_SRC} ${TESTUTILS_SRC} ${MOCKS_SRC})
target_compile_options(${PROJECT_PREFIX}_integrationtests PRIVATE ${TEST_CXX_FLAGS})
target_include_directories(${PROJECT_PREFIX}_integrationtests PRIVATE .)
target_link_libraries(${PROJECT_PREFIX}_integrationtests ${TEST_LINK_LIBS})
set_target_properties(${PROJECT_PREFIX}_integrationtests PROPERTIES 
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)
