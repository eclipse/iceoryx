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

# Find GIT revisions
execute_process(COMMAND
  git describe --match=None --always --abbrev=40 --dirty
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE ICEORYX_SHA1
  OUTPUT_STRIP_TRAILING_WHITESPACE)

string(TIMESTAMP ICEORYX_BUILDDATE UTC)

set(IOX_VERSION_TWEAK "0")


if(DEFINED ${PROJECT_VERSION_TWEAK})
  set(IOX_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/iceoryx_versions.hpp.in"
  "${CMAKE_BINARY_DIR}/generated/iceoryx/include/iceoryx_versions.hpp" @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/generated/iceoryx/include/iceoryx_versions.hpp
  DESTINATION include/${PREFIX}
  COMPONENT dev)
