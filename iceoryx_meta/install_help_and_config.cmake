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

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/../doc/website/FAQ.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../doc/website/getting-started/overview.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../doc/website/advanced/installation-guide-for-contributors.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE
        ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICE.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../CHANGELOG.md
  DESTINATION share/doc/iceoryx
  COMPONENT dev)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../doc/3rd_party_licenses
  DESTINATION share/doc/iceoryx
  COMPONENT dev)
