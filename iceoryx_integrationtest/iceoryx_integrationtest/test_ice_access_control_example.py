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

# @brief Test goal: "Integrationtest for the ice_access_control example of iceoryx"
# @pre setup ROS2 launch executable for RouDi (debug mode) the example processes
# @post check if all applications return exitcode 0 (success) after test run
@pytest.mark.launch_test
def generate_test_description():

    proc_env = os.environ.copy()
    colcon_prefix_path = os.environ.get('COLCON_PREFIX_PATH', '')

    # Configure users and groups necessary to run this integration test
    subprocess.call(['sh', '$(git rev-parse --show-toplevel)/iceoryx_examples/ice_access_control/config_and_run_ice_access_control.sh config'])

    executable_list = ['iox-cpp-display', 'iox-cpp-radar', 'iox-cpp-cheeky']
    user_list = ['infotainment', 'perception', 'notallowed']

    for exec, user in zip(executable_list, user_list):
        tmp_exec = os.path.join(
            colcon_prefix_path,
            'example_waitset/bin/',
            exec)
        tmp_process = launch.actions.ExecuteProcess(
            cmd=['sudo -u ', user, ' -g iceoryx --',  tmp_exec],
            env=proc_env, output='screen')
        process_list.append(tmp_process)

    print("Process list:", process_list)

    roudi_executable = os.path.join(
        colcon_prefix_path,
        'iceoryx_ice_access_control/bin/',
        'iox-cpp-roudi-static-segments'
    )
    roudi_process = launch.actions.ExecuteProcess(
        cmd=[roudi_executable, '-l', 'debug'],
        env=proc_env, output='screen',
        sigterm_timeout='20')

    return launch.LaunchDescription([
        roudi_process,
        process_list[0],
        process_list[1],
        process_list[2],
        launch_testing.actions.ReadyToTest()
    ]), {'iox-cpp-radar': process_list[0], 'iox-cpp-display': process_list[1], 'iox-cpp-cheeky': process_list[2]}


class TestIceAccessControlExample(unittest.TestCase):
    def test_roudi_ready(self, proc_output):
        proc_output.assertWaitFor(
            'RouDi is ready for clients', timeout=45, stream='stdout')

    def test_ice_access_control_radar(self, proc_output):
        proc_output.assertWaitFor(
            'iox-cpp-radar sent value: 10', timeout=45, stream='stdout')

    def test_ice_access_control_display(self, proc_output):
        proc_output.assertWaitFor(
            'iox-cpp-display sending value: 10', timeout=45, stream='stdout')

    def test_ice_access_control_cheeky(self, proc_output):
        proc_output.assertWaitFor(
            'RouDi did not find a writable shared memory segment for the current user.', timeout=45, stream='stdout')


@ launch_testing.post_shutdown_test()
class Testice_access_controlSetExampleExitCodes(unittest.TestCase):
    def test_exit_code(self, proc_info):
        launch_testing.asserts.assertExitCodes(proc_info)
