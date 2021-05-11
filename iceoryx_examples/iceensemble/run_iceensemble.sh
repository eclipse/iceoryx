#!/bin/bash

CONFIGDIR="`pwd`"
WORKSPACE=$(git rev-parse --show-toplevel)
SESSION=iceensemble
tmux="tmux -2 -q"

$tmux kill-server

$tmux has-session -t $SESSION
if [ $? -eq 0 ]; then
       echo "Session $SESSION already exists. Attaching to session."
       $tmux attach -t $SESSION
       exit 0;
fi

command -v tmux >/dev/null 2>&1 || { echo >&2 "tmux is not installed but required. Trying to install it..."; sudo apt-get install tmux; }

$tmux new-session -d -s $SESSION
$tmux new-window -a -t $SESSION $WORKSPACE/build/iox-roudi

$tmux split-window -t 0 -h $WORKSPACE/build/iceoryx_examples/icehello/iox-cpp-publisher-helloworld
$tmux split-window -t 1 -h $WORKSPACE/build/iceoryx_examples/icedelivery/iox-cpp-publisher
$tmux split-window -t 0 -h $WORKSPACE/build/iceoryx_examples/icedelivery/iox-cpp-publisher-untyped
$tmux split-window -t 0 -v $WORKSPACE/build/iceoryx_examples/iceoptions/iox-cpp-publisher-with-options
$tmux split-window -t 2 -v $WORKSPACE/build/iceoryx_examples/icedelivery_in_c/iox-c-publisher
$tmux split-window -t 4 -v $WORKSPACE/build/iceoryx_examples/icedelivery/iox-cpp-subscriber
$tmux split-window -t 6 -v $WORKSPACE/build/iceoryx_examples/iceoptions/iox-cpp-subscriber-with-options

$tmux attach -t $SESSION
