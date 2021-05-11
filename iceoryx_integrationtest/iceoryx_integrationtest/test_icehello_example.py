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

import os

import unittest

import launch
from launch_ros.substitutions import ExecutableInPackage
import launch_testing
import launch_testing.actions
from launch_testing.asserts import assertSequentialStdout

import pytest

# @brief Test goal: "Integrationtest for the icehello example of iceoryx"
# @pre setup ROS2 launch executables for RouDi (debug mode) and the example processes
# @post check if all applications return exitcode 0 (success) after test run
@pytest.mark.launch_test
def generate_test_description():

    proc_env = os.environ.copy()
    colcon_prefix_path = os.environ.get('COLCON_PREFIX_PATH', '')

    roudi_executable = os.path.join(
        colcon_prefix_path,
        'iceoryx_posh/bin/',
        'iox-roudi'
    )
    roudi_process = launch.actions.ExecuteProcess(
        cmd=[roudi_executable, '-l', 'debug'],
        env=proc_env, output='screen',
        sigterm_timeout='20')

    icehello_publisher_executable = os.path.join(
        colcon_prefix_path,
        'example_icehello/bin/',
        'iox-cpp-publisher-helloworld'
    )
    icehello_publisher_process = launch.actions.ExecuteProcess(
        cmd=[icehello_publisher_executable],
        env=proc_env, output='screen')

    icehello_subscriber_executable = os.path.join(
        colcon_prefix_path,
        'example_icehello/bin/',
        'iox-cpp-subscriber-helloworld'
    )
    icehello_subscriber_process = launch.actions.ExecuteProcess(
        cmd=[icehello_subscriber_executable],
        env=proc_env, output='screen')

    return launch.LaunchDescription([
        roudi_process,
        icehello_publisher_process,
        icehello_subscriber_process,
        launch_testing.actions.ReadyToTest()
    ]), {'roudi_process': roudi_process, 'icehello_publisher_process': icehello_publisher_process, 'icehello_subscriber_process': icehello_subscriber_process}

# These tests will run concurrently with the dut process. After this test is done,
# the launch system will shut down RouDi


class TestIcehelloExample(unittest.TestCase):
    def test_roudi_ready(self, proc_output):
        proc_output.assertWaitFor(
            'RouDi is ready for clients', timeout=45, stream='stdout')

    def test_icehello_data_exchange(self, proc_output):
        proc_output.assertWaitFor(
            'iox-cpp-publisher-helloworld sent value: 15', timeout=45, stream='stdout')
        proc_output.assertWaitFor(
            'iox-cpp-subscriber-helloworld got value: 15', timeout=45, stream='stdout')

# These tests run after shutdown and examine the stdout log


@launch_testing.post_shutdown_test()
class TestIcehelloExampleExitCodes(unittest.TestCase):
    def test_exit_code(self, proc_info):
        launch_testing.asserts.assertExitCodes(proc_info)
