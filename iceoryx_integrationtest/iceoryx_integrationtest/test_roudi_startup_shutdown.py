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
import launch.actions
from launch_ros.substitutions import ExecutableInPackage

import launch_testing
import launch_testing.actions
from launch_testing.asserts import assertSequentialStdout

import pytest

# @brief Test goal: "Integrationtest for testing correct RouDi startup and shutdown"
# @pre setup ROS2 launch executable for RouDi (debug mode)
# @post check if all applications return exitcode 0 (success) after test run
@pytest.mark.launch_test
def generate_test_description():

    colcon_prefix_path = os.environ.get('COLCON_PREFIX_PATH', '')
    proc_env = os.environ.copy()

    roudi_executable = os.path.join(
        colcon_prefix_path,
        'iceoryx_posh/bin/',
        'iox-roudi'
    )
    roudi_process = launch.actions.ExecuteProcess(
        cmd=[roudi_executable, '-l', 'debug'],
        env=proc_env, output='screen',
        sigterm_timeout='20')

    return launch.LaunchDescription([
        roudi_process,
        launch_testing.actions.ReadyToTest()
    ]), {'roudi_process': roudi_process}


class TestRouDiProcess(unittest.TestCase):
    def test_roudi_ready(self, proc_output):
        proc_output.assertWaitFor(
            'RouDi is ready for clients', timeout=45, stream='stdout')


@launch_testing.post_shutdown_test()
class TestRouDiProcessOutput(unittest.TestCase):
    def test_exit_code(self, proc_info):
        launch_testing.asserts.assertExitCodes(proc_info)

    def test_full_output(self, proc_output, roudi_process):
        with assertSequentialStdout(proc_output, roudi_process) as cm:
            cm.assertInStdout('Log level set to:')
            cm.assertInStdout('[ Reserving shared memory successful ]')
            cm.assertInStdout('Registered memory segment')
            cm.assertInStdout('[ Reserving shared memory successful ]')
            cm.assertInStdout('Roudi registered payload data segment')
            cm.assertInStdout('RouDi is ready for clients')

            if os.name != 'nt':
                cm.assertInStdout("Joining 'Mon+Discover' thread...")
                cm.assertInStdout("...'Mon+Discover' thread joined.")
                cm.assertInStdout("Joining 'IPC-msg-process' thread...")
                cm.assertInStdout("...'IPC-msg-process' thread joined.")

    def test_out_of_order(self, proc_output, roudi_process):
        with assertSequentialStdout(proc_output, roudi_process) as cm:
            cm.assertInStdout('Log level set to:')
            cm.assertInStdout('[ Reserving shared memory successful ]')
            cm.assertInStdout('Registered memory segment')
            cm.assertInStdout('[ Reserving shared memory successful ]')
            cm.assertInStdout('Roudi registered payload data segment')
            cm.assertInStdout('RouDi is ready for clients')
