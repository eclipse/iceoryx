# Installation

All iceoryx libraries are deployed as independent CMake packages. Posh is using functions from utils and is depending on it. You are able to build posh and utils and integrate them into existing CMake projects.

## Prerequisites

### :octicons-package-dependencies-16: Dependencies

- 64-bit hardware (e.g. x86_64 or aarch64; 32-bit hardware might work, but is not supported)
- [CMake](https://cmake.org), 3.10 or later
- One of the following compilers:
    - [GCC](https://gcc.gnu.org), 7.4 or later (5.4 currently supported too)
    - [Clang](https://clang.llvm.org), 9.0 or later
    - [MSVC](https://visualstudio.microsoft.com/de/), part of Visual Studio 2019 or later
- [libacl](http://download.savannah.gnu.org/releases/acl/), 2.2 or later. Only for Linux & QNX.
- optional, [ncurses](https://invisible-island.net/ncurses/), 6.2 or later. Required by introspection tool (only for Linux, QNX and MacOS).

#### Optional, Cyclone DDS Gateway

The Cyclone DDS gateway depends currently on [Cyclone DDS](https://github.com/eclipse-cyclonedds/cyclonedds).
When building it with the CMake option `-DDDS_GATEWAY=ON` it will be automatically installed as a dependency.
Furthermore, you have to install:

- [Apache Maven](http://maven.apache.org/download.cgi), 3.5 or later
- [OpenJDK](http://jdk.java.net/11/), 11.0 or later. Alternatively, Java JDK version 8 or later

!!! hint
    If you are behind a corporate firewall you may have to adjust the proxy settings of maven in `/etc/maven/settings.xml`. See: [Maven Proxy Configuration](https://maven.apache.org/settings.html#proxies)

### :material-apple: Mac OS

Before installing iceoryx you need to install XCode and git. Optionally, ncurses library is required for
the introspection client. To install ncurses locally into your build folder follow these steps

```bash
cd iceoryx
ICEORYX_DIR=$PWD
mkdir -p build
cd build
git clone https://github.com/mirror/ncurses.git
cd ncurses
git checkout v6.2
./configure  --prefix=$ICEORYX_DIR/build/dependencies/ --exec-prefix=$ICEORYX_DIR/build/dependencies/ --with-termlib
make -j12
make install
```

### :fontawesome-brands-linux: Linux

Although we strive to be fully POSIX-compliant, we recommend using Ubuntu 18.04 and at least GCC 7.5.0 for development.

You will need to install the following packages:

```bash
sudo apt install gcc g++ cmake libacl1-dev libncurses5-dev pkg-config
```

Additionally, there is an optional dependency to the [cpptoml](https://github.com/skystrife/cpptoml) library, which is used to parse the RouDi config file containing mempool configuration.

### :fontawesome-brands-blackberry: QNX

QNX SDP 7.0 and 7.1 are supported (shipping with gcc 5.4 and gcc 8.3 respectively).

The easiest way to build iceoryx on QNX is by using the build script and providing a toolchain file.
We provide generic QNX SDP 7.0 toolchain files for ARM_64 and X86_64 in `./tools/toolchains/qnx` ([Direct Link](https://github.com/eclipse-iceoryx/iceoryx/tree/master/tools/toolchains/qnx)).

ARM_64:

```bash
./tools/iceoryx_build_test.sh -t /home/user/toolchains/qnx/qnx_sdp70_aarch64le.cmake
```

X86_64:

```bash
./tools/iceoryx_build_test.sh -t /home/user/toolchains/qnx/qnx_sdp70_x86_64.cmake
```

!!! attention
    Please ensure that the folder `/var/lock` exist and the filesystem supports file locking.

### :fontawesome-brands-windows: Windows

In case you do not have a Windows installation, Microsoft provides free developer images from [here](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/).

Additionally, [CMake](https://cmake.org/download/) and [git](https://gitforwindows.org/) are required. The option to add CMake to the system PATH for all users should be set when it is installed.

If the developer image from Microsoft is used, Visual Studio Community 2019 is already installed, else it can be found [here](https://visualstudio.microsoft.com/de/downloads/).

To be able to compile iceoryx, the `Desktop development with C++` Workload must be installed. This is done by running `VisualStudioInstaller` and selecting the `Modify` button on `Visual Studio Community 2019`.

Either `VS Code` or `Developer Command Prompt` can be used to build iceoryx with CMake. Maybe one or two restarts are required to let CMake find the compiler.

Alternatively, `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat` can be executed in any shell to setup all the paths for compilation.

## :material-triangle: Build with CMake

!!! note
    Building with CMake is the preferred way, for more complex actions like a coverage scan
    is a script available (see chapter below).

The `CMakeLists.txt` from `iceoryx_meta` can be used to easily develop iceoryx with an IDE.

1. Clone the repository

    ```bash
    git clone https://github.com/eclipse-iceoryx/iceoryx.git
    ```

2. Generate the necessary build files

    ```bash
    cd iceoryx
    cmake -Bbuild -Hiceoryx_meta
    # when you have installed external dependencies like ncurses you have to add them
    # to your prefix path
    cmake -Bbuild -Hiceoryx_meta -DCMAKE_PREFIX_PATH=$(PWD)/build/dependencies/
    ```

    !!! tip
        To build all iceoryx components add `-DBUILD_ALL` to the CMake command. For Windows it is currently recommended to use the `cmake -Bbuild -Hiceoryx_meta -DBUILD_TEST=ON -DINTROSPECTION=OFF -DBINDING_C=ON -DEXAMPLES=ON` instead

3. Compile the source code

    ```bash
    cmake --build build
    ```

    !!! tip
        You can speed up the build by appending `-j 4` where 4 stands for the number of parallel build processes.
        You can choose more or less depending on your available CPU cores on your machine.

4. Install to system

    Mac:

    ```bash
    cmake --build build --target install
    ```

    Linux:

    ```bash
    sudo cmake --build build --target install
    ```

    !!! tip
        The installation directory is usually left at its default, which is `/usr/local`

    !!! note
        iceoryx is built in release mode as static library with `-O3` optimization by default. If you want to enable debug symbols please set `CMAKE_BUILD_TYPE=Deb`.

### Build options

Please take a look at the CMake file [build_options.cmake](https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_meta/build_options.cmake)
to get an overview of the available build options for enabling additional features.

## :material-powershell: Build with script

As an alternative, we provide a build-test script which we use to integrate iceoryx into our infrastructure.
The intention of the script goes beyond building iceoryx, it is also used for the code coverage scan or the address-sanitizer runs on the CI.
The script currently works for Linux and QNX only, it is planned to offer a multi-platform solution.

 1. Clone the repository

    ```bash
    git clone https://github.com/eclipse-iceoryx/iceoryx.git
    ```

 2. Build everything

    ```bash
    cd iceoryx
    ./tools/iceoryx_build_test.sh build-all
    ```

    !!! note
        The build script is installing the header files and binaries into `build/install/prefix`.

You can use the `help` argument for getting an overview of the available options:

```bash
./tools/iceoryx_build_test.sh help
```

!!! tip
    The examples can be built with `-DEXAMPLES=ON` with iceoryx_meta or by providing the `examples` argument to the build script.

## :material-robot: Build with colcon

Alternatively, iceoryx can be built with [colcon](https://colcon.readthedocs.io/en/released/user/installation.html#using-debian-packages) to provide a smooth integration for ROS 2 developers.
To build the iceoryx_integrationtest package one requires a minimal [ROS 2 installation](https://docs.ros.org/en/foxy/Installation/Linux-Install-Debians.html).

Install required ROS 2 packages:

```bash
sudo apt install ros-foxy-ros-testing ros-foxy-ros-base
source /opt/ros/foxy/setup.bash
```

build with colcon:

```bash
mkdir -p iceoryx_ws/src
cd $_
git clone https://github.com/eclipse-iceoryx/iceoryx.git
cd ..
colcon build
```

!!! note
    If you don't want to install ROS 2, you can skip the iceoryx_integrationtest package by calling:

    ```bash
    colcon build --packages-skip iceoryx_integrationtest
    ```

This build method makes the most sense in combination with [rmw_iceoryx](https://github.com/ros2/rmw_iceoryx.git)
