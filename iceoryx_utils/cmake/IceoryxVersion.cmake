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


## This exports the parsed version info
## Name : parse_version
## Params: IOX_VERSION_STRING
## Output: Sets IOX_VERSION_MAIN and VERSION_SUFFIX in the parent scope
function(parse_version IOX_VERSION_STRING)
    string(REGEX MATCH "([0-9\\.]*)(-[a-z]*[0-9]*)?" _ ${IOX_VERSION_STRING})

    if(CMAKE_MATCH_COUNT EQUAL 2)
        set(IOX_VERSION_MAIN ${CMAKE_MATCH_1} PARENT_SCOPE)
        set(IOX_VERSION_SUFFIX ${CMAKE_MATCH_2} PARENT_SCOPE)
    elseif(CMAKE_MATCH_COUNT EQUAL 1)
        set(IOX_VERSION_MAIN ${CMAKE_MATCH_1} PARENT_SCOPE)
        set(IOX_VERSION_SUFFIX "" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Could not parse version string")
    endif()
endfunction()

function(adjust_version)
    set(IOX_FULL_VERSION ${PROJECT_VERSION}${IOX_VERSION_SUFFIX})
    set(PROJECT_VERSION ${IOX_FULL_VERSION} PARENT_SCOPE)
    set(${PROJECT_NAME}_VERSION ${IOX_FULL_VERSION} PARENT_SCOPE)
endfunction()
