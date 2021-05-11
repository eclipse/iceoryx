# Overview

This directory contains files related to building and running [Eclipse iceoryx](https://github.com/eclipse-iceoryx/iceoryx) using [Docker](https://www.docker.com/).

# Building the iceoryx Docker Image

To create a Docker image with the built iceoryx libraries and examples from your current sources, run the following script from the root of the repository: `./tools/docker/build_iceoryx_docker.sh`.

After the Docker image is built, you can run it in interactive mode then use one of the below options to connect to it and run the example applications.

# Running an iceoryx Docker container

A helper script is provided to launch containers for ready-built containers.
Simply run the following script from any location:
```
./tools/docker/run_iceoryx_docker.sh
```

# Connecting to the Docker container and running the example applications

With the iceoryx Docker container running RouDi, you can connect to it to play with the example applications.
There are a couple of methods available to connect to do this.

## Using `docker exec`

It is possible to bind a shell to running containers via `docker exec`. A helper script is provided with the command for convenience.
To bind a shell to the container, simply run the following script from any location:
```
./tools/docker/bind_iceoryx_docker.sh
```
You will then be dropped into a bash shell where you can access all iceoryx binaries.

All iceoryx binaries are directly available in the $PATH of the shell.
For example, to start a sender application, you can run the following from the bound bash shell:

```
root@b10b3630f6d3:/# iox-cpp-publisher-untyped
2020-12-18 09:04:12.813 [ Debug ]: Application registered management segment 0x7fa1a47c4000 with size 64244064 to id 1
2020-12-18 09:04:12.813 [ Info  ]: Application registered payload segment 0x7fa19b98a000 with size 149134400 to id 2
```

To run the corresponding receiver application, bind another shell to the container and run the following:

```
root@b10b3630f6d3:/# iox-cpp-subscriber-untyped
2020-12-18 09:04:21.692 [ Debug ]: Application registered management segment 0x7fcda1597000 with size 64244064 to id 1
2020-12-18 09:04:21.692 [ Info  ]: Application registered payload segment 0x7fcd9875d000 with size 149134400 to id 2
Got value: 10
Got value: 10
Got value: 11
Got value: 11
Got value: 12
...
```

The complete communication flow should now be observable.

There are a lot more binaries to test, detailed explanations for each can be found in [iceoryx_examples](./../../iceoryx_examples).

## Using `screen`

Another way of interacting with the daemon and examples is to use [screen](https://www.gnu.org/software/screen/) (also included in the Docker image) to start executables in different virtual screens that you attach to and detach from as your experiments may require.

This may be a better option if you are working on a remote machine and don't want to have to open multiple connections to it in separate terminals and/or if you're comfortable with using `screen`.

To start a session with screen, run the following from a bound bash shell:

```
root@b10b3630f6d3:/# screen
```

and press `[Enter]`.

Inside the virtual screen shell, you can then launch the example applications.
Again, all iceoryx applications are directly available in the $PATH.

To launch an example sender application, run the corresponding binary:

```
root@b10b3630f6d3 /# iox-cpp-publisher-untyped
2020-12-18 09:28:17.541 [ Debug ]: Application registered management segment 0x7f2f615d9000 with size 64244064 to id 1
2020-12-18 09:28:17.541 [ Info  ]: Application registered payload segment 0x7f2f5879f000 with size 149134400 to id 2
```

Press `[Ctrl]+A D` to detach from this virtual screen.

Then, the receiver application can be started in a separate virtual screen:

```
root@b10b3630f6d3:/# screen
#[Enter]
root@b10b3630f6d3 /# iox-cpp-subscriber-untyped
2020-12-18 09:29:24.082 [ Debug ]: Application registered management segment 0x7f39fb9fe000 with size 64244064 to id 1
2020-12-18 09:29:24.083 [ Info  ]: Application registered payload segment 0x7f39f2bc4000 with size 149134400 to id 2
Got value: 14
Got value: 14
Got value: 15
Got value: 15
Got value: 16
Got value: 16
...
```

You can now detach from this virtual screen with `[Ctrl]+A D`.

### Exercises with iceoryx examples using screen

As an exercise, you can return to previous screen sessions to observe their current output.
For examples, here we list all screens and return to the one running the sender:

```
root@b10b3630f6d3:/# screen -r
There are several suitable screens on:
	66.pts-0.b10b3630f6d3	(12/18/20 09:29:24)	(Detached)
	35.pts-0.b10b3630f6d3	(12/18/20 09:28:17)	(Detached)
Type "screen [-d] -r [pid.]tty.host" to resume one of them.
```

Try the following to return to the sender application (adjust the PID as per your exact `screen -r` output above):

```
screen -r 35
```

You should see again the virtual screen of the sender application. You can stop the sender with `[Ctrl]+C`, and then detach 
from this screen with `[Ctrl]+A D`, and return to the receiver screen with (adjusting the PID as per your `screen -r` output above):

```
screen -r 66
```

and you should see the receiver output again.

As an exercise, try return to the sender screen and relaunch the sender, then detach from the sender screen and attach to the receiver screen and see the output based on the new sender:

```
Got value: 104
Got value: 104
Got value: 105
Got value: 105
Got value: 106
Got value: 106
...
```
