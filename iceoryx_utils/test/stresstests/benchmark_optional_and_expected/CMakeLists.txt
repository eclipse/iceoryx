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

# Build icedelivery example
cmake_minimum_required(VERSION 3.5)
project(benchmark_optional_and_expected)

include(GNUInstallDirs)

find_package(iceoryx_utils CONFIG REQUIRED)
find_package(Threads REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_utils::iceoryx_utils CXX_STANDARD)
if ( NOT ICEORYX_CXX_STANDARD )
    include(IceoryxPlatform)
endif ( NOT ICEORYX_CXX_STANDARD )

add_executable(iox-bm-optional-and-expected ./benchmark_optional_and_expected.cpp)
target_link_libraries(iox-bm-optional-and-expected
    iceoryx_utils::iceoryx_utils
    Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(TEST_CXX_FLAGS ${ICEORYX_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(TEST_CXX_FLAGS PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})
endif()

target_compile_options(iox-bm-optional-and-expected PRIVATE ${TEST_CXX_FLAGS})

set_target_properties(iox-bm-optional-and-expected PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)

install(
    TARGETS iox-bm-optional-and-expected
    RUNTIME DESTINATION bin
)
