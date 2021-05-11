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

project(iceoryx_package VERSION ${IOX_VERSION_STRING})


set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "iceoryx-${iceoryx_package_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "iceoryx_${iceoryx_package_VERSION}_${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
set(CPACK_PACKAGE_CONTACT "michael.poehnl@apex.ai")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libacl1-dev,libncurses5-dev")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "iceoryx inter-process-communication (IPC) middleware")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/eclipse-iceoryx/iceoryx")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "https://projects.eclipse.org/projects/technology.iceoryx/who")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
include(CPack)
