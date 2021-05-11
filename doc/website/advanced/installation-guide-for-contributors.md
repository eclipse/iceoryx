# Installation guide for contributors

## :material-test-tube: Build and run tests

While developing on iceoryx you want to know if your changes are breaking existing functions or if your newly written tests are passing.
For that purpose, we are generating CMake targets that are executing the tests. First, we need to build them:

```bash
cmake -Bbuild -Hiceoryx_meta -DBUILD_TEST=ON
cmake --build build
```

CMake is automatically installing GoogleTest as a local dependency and build the tests against it. Please note that if you want to build tests for extensions like the DDS-Gateway you need to enable this extension as well in the CMake build. To build tests for all extensions simply add `-DBUILD_ALL` to the CMake command.

!!! hint
    Before creating a Pull-Request, you should check your code for compiler warnings. For that purpose is the `-DBUILD_STRICT` CMake option available which treats compiler warnings as errors. This flag is enabled on the GitHub CI for building Pull-Requests.

Now let's execute tests:

```bash
cd iceoryx/build
make all_tests
```

Some of the tests are timing critical and need a stable environment. We call them timing tests and have them in separate targets available:

```bash
make timing_module_tests
make timing_integration_tests
```

In iceoryx we distinguish between different test levels. The most important are: Module tests and Integration tests.
Module tests are basically Unit-tests where the focus is on class level with black-box testing.
In integration tests multiple classes (e.g. mepoo config) are tested together.
The source code of the tests is placed into the folder `test` within the different iceoryx components. You can find there at least a folder `moduletests` and sometimes `integrationtests`.

If you now want to create a new test you can place the sourcefile directly into the right folder. CMake will automatically detect the new file when doing a clean build and will add it to a executable. There is no need to add a gtest main function because we already provide it.
For every test level are executables created, for example `posh_moduletests`. They are placed into the corresponding build folder (e.g. `iceoryx/build/posh/test/posh_moduletests`).

If you want to execute only individual test cases, you can use these executables together with a filter command.
Let's assume you want to execute only `ServiceDescription_test` from posh_moduletests:

```bash
./build/posh/test/posh_moduletests --gtest_filter="ServiceDescription_test*"
```

## :fontawesome-solid-pump-soap: Use Sanitizer Scan

Due to the fact that iceoryx works a lot with system memory, it should be ensured that errors like memory leaks are not introduced.
To prevent this, we use the clang toolchain which offers several tools for scanning the codebase. One of them is the [Address-Sanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) which checks for example on dangling pointers.

The below-listed sanitizers are enabled at the moment.

- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) (ASan) is a fast memory error detector.
!!! note
    AddressSanitizer exits on the first detected error, which means there could be more errors in the codebase when this error is reported.
- [LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html) (LSan) is a run-time memory leak detector. In iceoryx, it runs as part of the AddressSanitizer.
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) (UBSan) is a fast undefined behavior detector. iceoryx uses default behavior i.e. `print a verbose error report and continue execution`

With the `iceoryx_build_test.sh` script you can do the scan on your own. Additionally, the scans are running on the CI in every Pull-Request.
As a prerequisite, you need to install the clang compiler:

```bash
sudo apt install clang
```

Then you need to compile iceoryx with the sanitizer flags:

```bash
./tools/iceoryx_build_test.sh build-strict build-all sanitize clang clean
```

Now we can run the tests with enabled sanitizer options:

```bash
cd build && ./tools/run_tests.sh
```

If errors occur, an error report is shown with a stack trace to find the place where the leak occurs. If the leak comes from an external dependency or shall be handled later then it is possible to set a function on a suppression list.
This should be only rarely used and only in coordination with an iceoryx maintainer.

!!! note
    iceoryx needs to be built as a static library for working with sanitizer flags. The script does it automatically.
    Except when you want to use the ${ICEORYX_WARNINGS} then you have to call `findpackage(iceoryx_utils)`

## :material-library: iceoryx library build

The iceoryx build consists of several libraries which have dependencies on each other. The goal is to have self-encapsulated library packages available where the end-user can easily find it with the CMake command `find-package(...)`.
In the default case, the iceoryx libraries are installed by `make install` into `/usr/lib` which requires root access. As an alternative you can install the libs into a custom folder by setting `-DCMAKE_INSTALL_PREFIX=/custom/install/path` as build-flag for the CMake file in iceoryx_meta.

As a starting point for the CMake build, iceoryx_meta collects all libraries (utils, posh etc.) and extensions (binding_c, dds) together. The provided build script `iceoryx_build_test.sh` in the `tools` folder uses iceoryx_meta.

Per default, iceoryx is built as static lib for better usability.
Additionally, we offer to build as shared library because it is a cleaner solution for resolving dependency issues and it reduces the linker time.
This is done by the flag `BUILD_SHARED_LIBS` which is set to OFF per default. If you want to have shared libraries, just pass `-DBUILD_SHARED_LIBS=ON` to CMake or use `build-shared` as a flag in the build script.

If iceoryx builds shared libraries you have to copy them into a custom path and need to set the LD_LIBRARY_PATH to the custom path (e.g. build/install/prefix).

```bash
export LD_LIBRARY_PATH=/your/path/to/iceoryx/libs
```

or you can set it directly:

```bash
LD_LIBRARY_PATH=/your/path/to/lib iox-roudi
```

If you want to share iceoryx to other users, you can create a debian package. This can be done by using: `./tools/iceoryx_build_test.sh package` where it will be deployed into the `build_package` folder.

!!! note
    The CMake libraries export their dependencies for easier integration. This means that you do not need to do a `findpackage()` to all the dependencies. For example, you don't need to call `findpackage(iceoryx_utils)` when you have it done for iceoryx_posh. It includes it already.
