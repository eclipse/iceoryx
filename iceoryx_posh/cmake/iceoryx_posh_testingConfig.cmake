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

#
########## dummyConfig.cmake to be able to use find_package with the source tree ##########
#

if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND_PRINTED)
    message(STATUS "The package '${CMAKE_FIND_PACKAGE_NAME}' is used in source code version.")
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND_PRINTED true CACHE INTERNAL "")
endif()
