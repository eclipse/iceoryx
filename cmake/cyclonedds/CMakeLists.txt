# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
project(cyclonedds-build CXX)

include(ProcessorCount)
ProcessorCount(N)

if(NOT N EQUAL 0)
    if(((${CMAKE_VERSION} VERSION_GREATER "3.12.0") OR ${CMAKE_VERSION} VERSION_EQUAL "3.12.0"))
        set(CMAKE_BUILD_FLAGS -j ${N})
    elseif(LINUX OR QNX)
        set(CMAKE_BUILD_FLAGS -- -j ${N})
    endif()
endif()

if(WIN32)
    set(CREATE_PATH_COMMAND mkdir)
else()
    set(CREATE_PATH_COMMAND mkdir -p)
endif(WIN32)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    set(TOOLCHAIN_FILE "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

# ===== Helpers
function(fetch_and_install name)
    set(DOWNLOAD_CONFIG_DIR ${CMAKE_BINARY_DIR}/dependencies/${name}/download)
    set(SOURCE_DIR ${CMAKE_BINARY_DIR}/dependencies/${name}/src)
    set(BUILD_DIR ${CMAKE_BINARY_DIR}/dependencies/${name}/build)
    set(INSTALL_DIR ${CMAKE_BINARY_DIR}/dependencies/install)

    # Fetch source
    configure_file(${name}.cmake.in ${DOWNLOAD_CONFIG_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "${TOOLCHAIN_FILE}" "${DOWNLOAD_CONFIG_DIR}"
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DOWNLOAD_CONFIG_DIR} )
    if(result)
        message(FATAL_ERROR "CMake step [configure download] for ${name} failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build . ${CMAKE_BUILD_FLAGS}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DOWNLOAD_CONFIG_DIR} )
    if(result)
        message(FATAL_ERROR "Build step [download] for ${name} failed: ${result}")
    endif()

    # Build
    execute_process(COMMAND ${CREATE_PATH_COMMAND} "${BUILD_DIR}"
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )
    if(result)
        message(FATAL_ERROR "CMake step [create build dir] for ${name} failed: ${result}")
    endif()

    # Parse additional CMake flags
    set(ADDITIONAL_CMAKE_FLAGS "")
    foreach(flag IN LISTS ARGN)
        list(APPEND ADDITIONAL_CMAKE_FLAGS ${flag})
    endforeach()
    string( REPLACE ";" " " ADDITIONAL_CMAKE_FLAGS "${ADDITIONAL_CMAKE_FLAGS}")

    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}" "${ADDITIONAL_CMAKE_FLAGS}" "${TOOLCHAIN_FILE}" "${SOURCE_DIR}"
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${BUILD_DIR} )
    if(result)
        message(FATAL_ERROR "CMake step [configure] for ${name} failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install ${CMAKE_BUILD_FLAGS}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${BUILD_DIR} )
    if(result)
        message(FATAL_ERROR "Build step [build and install] for ${name} failed: ${result}")
    endif()
endfunction()

# ===== Install 

fetch_and_install(idlpp-cxx)
fetch_and_install(cyclonedds)
fetch_and_install(cyclonedds-cxx -DBUILD_TESTING=OFF)
