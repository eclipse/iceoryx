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

# Build user_header example
cmake_minimum_required(VERSION 3.5)
project(example_user_header)

include(GNUInstallDirs)

find_package(iceoryx_posh REQUIRED)
find_package(iceoryx_utils REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

## C++ typed API publisher
add_executable(iox-cpp-user-header-publisher
               publisher_cxx_api.cpp
)
target_link_libraries(iox-cpp-user-header-publisher
                      PRIVATE
                      iceoryx_utils::iceoryx_utils
                      iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-user-header-publisher
                       PRIVATE
                       ${ICEORYX_WARNINGS}
                       ${ICEORYX_SANITIZER_FLAGS}
)

## C++ untyped API publisher
add_executable(iox-cpp-user-header-untyped-publisher
               publisher_untyped_cxx_api.cpp
)
target_link_libraries(iox-cpp-user-header-untyped-publisher
                      PRIVATE
                      iceoryx_utils::iceoryx_utils
                      iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-user-header-untyped-publisher
                       PRIVATE
                       ${ICEORYX_WARNINGS}
                       ${ICEORYX_SANITIZER_FLAGS}
)

## C++ typed API subscriber
add_executable(iox-cpp-user-header-subscriber
               subscriber_cxx_api.cpp
)
target_link_libraries(iox-cpp-user-header-subscriber
                      PRIVATE
                      iceoryx_utils::iceoryx_utils
                      iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-user-header-subscriber
                       PRIVATE
                       ${ICEORYX_WARNINGS}
                       ${ICEORYX_SANITIZER_FLAGS}
)

## C++ untyped API subscriber
add_executable(iox-cpp-user-header-untyped-subscriber
               subscriber_untyped_cxx_api.cpp
)
target_link_libraries(iox-cpp-user-header-untyped-subscriber
                      PRIVATE
                      iceoryx_utils::iceoryx_utils
                      iceoryx_posh::iceoryx_posh
)
target_compile_options(iox-cpp-user-header-untyped-subscriber
                       PRIVATE
                       ${ICEORYX_WARNINGS}
                       ${ICEORYX_SANITIZER_FLAGS}
)

## additional properties
set_target_properties(iox-cpp-user-header-publisher
                      iox-cpp-user-header-subscriber
                      iox-cpp-user-header-untyped-publisher
                      iox-cpp-user-header-untyped-subscriber
                      PROPERTIES
                      CXX_STANDARD_REQUIRED ON
                      CXX_STANDARD ${ICEORYX_CXX_STANDARD}
                      POSITION_INDEPENDENT_CODE ON
)

install(TARGETS
        iox-cpp-user-header-publisher
        iox-cpp-user-header-subscriber
        iox-cpp-user-header-untyped-publisher
        iox-cpp-user-header-untyped-subscriber
        RUNTIME DESTINATION bin
)

find_package(iceoryx_binding_c QUIET)
if(iceoryx_binding_c_FOUND)

    set_source_files_properties(
        publisher_c_api.c
        subscriber_c_api.c
        PROPERTIES
        LANGUAGE C
        C_STANDARD_REQUIRED ON
        C_STANDARD 11
    )

    if("-Wno-noexcept-type" IN_LIST ICEORYX_WARNINGS)
        list(REMOVE_ITEM ICEORYX_WARNINGS "-Wno-noexcept-type")
    endif()

    ## C API publisher
    add_executable(iox-c-user-header-publisher
                publisher_c_api.c
    )
    target_link_libraries(iox-c-user-header-publisher
                        PRIVATE
                        iceoryx_binding_c::iceoryx_binding_c
    )
    target_compile_options(iox-c-user-header-publisher
                        PRIVATE
                        ${ICEORYX_WARNINGS}
                        ${ICEORYX_SANITIZER_FLAGS}
    )

    ## C API subscriber
    add_executable(iox-c-user-header-subscriber
                subscriber_c_api.c
    )
    target_link_libraries(iox-c-user-header-subscriber
                        PRIVATE
                        iceoryx_binding_c::iceoryx_binding_c
    )
    target_compile_options(iox-c-user-header-subscriber
                        PRIVATE
                        ${ICEORYX_WARNINGS}
                        ${ICEORYX_SANITIZER_FLAGS}
    )

    set_target_properties(iox-c-user-header-publisher
                        iox-c-user-header-subscriber
                        PROPERTIES
                        POSITION_INDEPENDENT_CODE ON
    )

    install(TARGETS
            iox-c-user-header-publisher
            iox-c-user-header-subscriber
            RUNTIME DESTINATION bin
    )
endif()
