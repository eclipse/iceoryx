# Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.5)
project(iceoryx_integrationtest)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ros_testing)

  add_ros_test(iceoryx_integrationtest/test_roudi_startup_shutdown.py)
  add_ros_test(iceoryx_integrationtest/test_callback_example.py)
  add_ros_test(iceoryx_integrationtest/test_callback_in_c_example.py)
  add_ros_test(iceoryx_integrationtest/test_icehello_example.py)
  add_ros_test(iceoryx_integrationtest/test_icedelivery_example.py)
  add_ros_test(iceoryx_integrationtest/test_icedelivery_in_c_example.py)
  add_ros_test(iceoryx_integrationtest/test_singleprocess_example.py)
  add_ros_test(iceoryx_integrationtest/test_waitset_example.py)
  add_ros_test(iceoryx_integrationtest/test_waitset_in_c_example.py)
  add_ros_test(iceoryx_integrationtest/test_iceoptions_example.py)
  add_ros_test(iceoryx_integrationtest/test_complexdata_example.py)
  add_ros_test(iceoryx_integrationtest/test_user_header_example.py)

endif()

ament_package()
