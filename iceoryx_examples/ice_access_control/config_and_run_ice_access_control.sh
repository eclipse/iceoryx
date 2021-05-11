#!/bin/bash

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

# ***NOTE***
# This shell script is for Linux-based operating systems only.
# If you're using e.g QNX refer to the manual on how to set up groups, users and permissions

WORKSPACE=$(git rev-parse --show-toplevel)
CONFIG="OFF"
RUN="OFF"
SESSION=ice_access_control
TMUX="TMUX -2 -q"

while (( "$#" )); do
  case "$1" in
    config)
        CONFIG="ON"
        shift 1
        ;;
    run)
        RUN="ON"
        shift 1
        ;;
    *)
        echo "Invalid argument '$1'"
        exit -1
        ;;
  esac
done

if [ "$CONFIG" == "ON" ] ; then
    # Create groups
    sudo groupadd -f privileged
    sudo groupadd -f unprivileged
    sudo groupadd -f iceoryx

    # Create users w/o homedir
    sudo useradd -M perception
    sudo useradd -M infotainment
    sudo useradd -M roudi
    sudo useradd -M notallowed

    # Assign users to group and disable login
    sudo usermod -L perception -a -G privileged,iceoryx -s /sbin/nologin
    sudo usermod -L infotainment -a -G unprivileged,iceoryx -s /sbin/nologin
    sudo usermod -L roudi -a -G iceoryx -s /sbin/nologin
    sudo usermod -L notallowed -a -G iceoryx -s /sbin/nologin

    # Allow RouDi to send SIGKILL to other apps
    sudo setcap cap_kill=ep $WORKSPACE/build/iceoryx_examples/ice_access_control/iox-cpp-roudi-static-segments
fi

if [ "$RUN" == "ON" ] ; then
    $TMUX kill-server

    $TMUX has-session -t $SESSION
    if [ $? -eq 0 ]; then
        echo "Session $SESSION already exists. Attaching to session."
        $TMUX attach -t $SESSION
        exit 0;
    fi

    command -v $TMUX >/dev/null 2>&1 || { echo >&2 "TMUX is not installed but required. Trying to install it..."; sudo apt-get install TMUX; }

    $TMUX new-session -d -s $SESSION

    # Start custom RouDi in 'iceoryx' group
    $TMUX new-window -a -t $SESSION 'sudo -u roudi -g iceoryx -- $WORKSPACE/build/iceoryx_examples/ice_access_control/iox-cpp-roudi-static-segments'

    # Start perception app as 'perception' user
    $TMUX split-window -t 0 -h 'sudo -u perception -g iceoryx -- $WORKSPACE/build/iceoryx_examples/ice_access_control/iox-cpp-radar'

    # Start display app as 'infotainment' user
    $TMUX split-window -t 1 -v 'sudo -u infotainment -g iceoryx -- $WORKSPACE/build/iceoryx_examples/ice_access_control/iox-cpp-display'

    # Start cheeky app as 'notallowed' user
    $TMUX split-window -t 0 -v 'sudo -u notallowed -g iceoryx -- $WORKSPACE/build/iceoryx_examples/ice_access_control/iox-cpp-cheeky'

    $TMUX attach -t $SESSION
fi
