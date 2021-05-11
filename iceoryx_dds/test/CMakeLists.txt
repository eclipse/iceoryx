# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
cmake_minimum_required(VERSION 3.5)
project(test_iox_to_dds VERSION 0)

find_package(iceoryx_utils_testing REQUIRED)
find_package(iceoryx_posh_testing REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(PROJECT_PREFIX "dds_gateway")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_PREFIX}/test)

file(GLOB_RECURSE MODULETESTS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/moduletests/*.cpp")

set(TEST_LINK_LIBS
    ${CODE_COVERAGE_LIBS}
    GTest::gtest
    GTest::gmock
    iceoryx_utils::iceoryx_utils
    iceoryx_utils_testing::iceoryx_utils_testing
    iceoryx_posh::iceoryx_posh
    iceoryx_posh_testing::iceoryx_posh_testing
    iceoryx_dds::iceoryx_dds
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(TEST_CXX_FLAGS ${ICEORYX_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS} -Wno-unused -Wno-pedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS} -Wno-unused -Wno-pedantic)
endif()

if(LINUX)
    set(TEST_LINK_LIBS ${TEST_LINK_LIBS} dl)
elseif(QNX)
elseif(APPLE)
    set(TEST_LINK_LIBS ${TEST_LINK_LIBS} dl)
elseif(WIN32)
endif()

# unittests
add_executable(${PROJECT_PREFIX}_moduletests ${MODULETESTS_SRC})
set_property(TARGET ${PROJECT_PREFIX}_moduletests PROPERTY CXX_STANDARD ${ICEORYX_CXX_STANDARD})
target_include_directories(${PROJECT_PREFIX}_moduletests PRIVATE .)
target_compile_options(${PROJECT_PREFIX}_moduletests PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(${PROJECT_PREFIX}_moduletests ${TEST_LINK_LIBS})
