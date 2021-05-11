# Contributing to Eclipse iceoryx

Thanks for your interest in this project.

## Project description

In domains like automotive, robotics or gaming, a huge amount of data must be
transferred between different parts of the system. If these parts are actually
different processes on a POSIX based operating system like Linux, this huge
amount of data has to be transferred via an inter-process-communication (IPC)
mechanism. Find more infos on the [Eclipse site](https://projects.eclipse.org/projects/technology.iceoryx).

## Eclipse Contributor Agreement

Before your contribution can be accepted by the project team, contributors must
electronically sign the Eclipse Contributor Agreement ([ECA](http://www.eclipse.org/legal/ECA.php)).

Commits that are provided by non-committers must have a Signed-off-by field in
the footer indicating that the author is aware of the terms by which the
contribution has been provided to the project. The non-committer must
additionally have an Eclipse Foundation account and must have a signed Eclipse
Contributor Agreement (ECA) on file.

For more information, please see the [Eclipse Committer Handbook](https://www.eclipse.org/projects/handbook/#resources-commit).

## Contact

Contact the project developers via the project's "dev" list.

* iceoryx-dev@eclipse.org

## Feature request and bugs

We love pull requests! The next sections try to cover most of the relevant questions. For larger contributions or
architectural changes, we'd kindly ask you to either:

* Raise the proposed changes during a [developer meetup](https://github.com/eclipse-iceoryx/iceoryx/wiki/Developer-meetup)

or

* Create a design document and raise it in a separate pull request beforehand

If you would like to report a bug or propose a new feature, please raise an issue before raising a pull request.
Please have a quick search upfront if a similar issue already exists. An
[release board](https://github.com/eclipse-iceoryx/iceoryx/projects) is used to prioritize the issues for a specific release.
This makes it easier to track the work-in-progress. If you have troubles getting an issue assigned to you please
contact the maintainers via [Gitter](https://gitter.im/eclipse/iceoryx).

Please make sure you have:

1. Signed the [Eclipse Contributor Agreement](http://www.eclipse.org/legal/ECA.php)
2. Created an issue before creating a branch, e.g. `Super duper feature` with issue number `123`
3. All branches have the following naming format: `iox-#[issue]-branch-name` e.g. `iox-#123-super-duper-feature`
4. All commits have the following naming format: `iox-#[issue] commit message` e.g. `iox-#123 implemented super-duper feature`
5. All commits have been signed with `git commit -s`
6. You open your pull request towards the base branch `master`
7. Link the pull request to the according Github issue and set the label accordingly

## Branching strategy

`master`

* Main development branch
* Open for external contributions

`release_x.y`

* Branch for stablising a certain release
* Write access limited to maintainers
* Fine-tuning of external contribution e.g. running Axivion SCA
* Finish any missing implementations regarding the quality levels

As depicted below, after the release branch has been created the stabilisation phase will begin. After finishing the release, a git tag will be created to point to `HEAD` of the release branch. Follow-up releases will be branched off from the git tag.

```
o---o---o---o---o  master
     \
      \      v1.0.0      v1.0.1
       \        |           |
        o---o---o---o---o---o---o  release_1.0
                         \
                          \      v1.1.0
                           \        |
                            o---o---o  release_1.1
```

## Coding style

We love the [C++ core guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). If in doubt please try
to follow them as well as our unwritten conventions in the existing parts of the code base.
Please format your code with the provided [clang-format](https://clang.llvm.org/docs/ClangFormat.html) and
[clang-tidy](https://clang.llvm.org/extra/clang-tidy/) before raising a pull request. Lots of IDEs do read the
clang-format file these days.

We created some convenient rules to highlight some bits that you might not be used to in other FOSS projects. They are
helpful to build embedded systems for safety fields like automotive or avionics. It is possible that not the whole
codebase follows these rules, things are work in progress.

1) **No heap is allowed**, static memory management hugely decreases the complexity of your software (e.g. cxx::vector
    without heap)
2) **No exceptions are allowed**, all function and methods need to have `noexcept` in their signature
3) **No undefined behavior**, zero-cost abstract is not feasible in high safety environments
4) **Use C++14**
5) **[Rule of Five](https://en.wikipedia.org/wiki/Rule_of_three_(C%2B%2B_programming))**, if there is a non-default
    destructor needed, the rule of five has to be applied
6) **[STL](https://en.wikipedia.org/wiki/Standard_Template_Library)**, we aim to be compatible towards the STL, but
    our code may contain additions which are not compatible with the STL (e.g. `iox::cxx::vector::emplace_back()`
    does return a bool)
7) **Always use `iox::log::Logger`**, instead of `printf()`
8) **Always use `iox::ErrorHandler()`**, when an error occurs that cannot or shall not be propagated via an 
    `iox::cxx::expected`, the `iox::ErrorHandler()` shall be used; exceptions are not allowed

See [error-handling.md](https://github.com/eclipse-iceoryx/iceoryx/blob/master/doc/design/error-handling.md) for additional 
information about logging and error handling.

### Naming conventions

* File names with `lower_snake_case`: `my_thing.hpp`
* Structs, classes and enum classes in `UpperCamelCase`: `class MyClass{}`
* Methods and variables in `lowerCamelCase`: `uint16_t myVariable`
* Compile time constants, also enum values in `UPPER_SNAKE_CASE`: `static constexpr uint16_t MY_CONSTANT`
* Class members start with `m_`: `m_myMember`
    * Public members of structs and classes do not have the `m_` prefix
* Namespaces in `lower_snake_case` : `my_namespace`
* Aliases have a `_t` postfix : `using FooString_t = iox::cxx::string<100>;`

### Doxygen

Please use [doxygen](http://www.doxygen.nl/) to document your code.

The following doxygen comments are required for public API headers:

```cpp
    /// @brief short description
    /// @param[in] / [out] / [in,out] name description
    /// @return description
```

A good example for code formatting and doxygen structure is at [swe_docu_guidelines.md (WIP)](./doc/aspice_swe3_4/swe_docu_guidelines.md)

## Folder structure

The folder structure boils down to:

* iceoryx_COMPONENT
  * cmake: All cmakes files go here, needed for `find_pkg()`
  * doc: Manuals and documentation
  * include: public headers with stable API
    * internal: public headers with unstable API, which might change quite frequently
  * source: implementation files
  * test: unit and integration tests
  * CMakeLists.txt: Build the component separately
* examples_iceoryx: Examples described in [iceoryx_examples](./iceoryx_examples/README.md)

All new code should follow the folder structure.

### How to add a new example

1. Add the example in the ["List of all examples"](./iceoryx_examples/README.md)
2. Create a new file in `doc/website/getting-started/examples/foobar.md`. This file shall only set the title and include the readme from `./iceoryx_examples/foobar/README.md`
3. Add an `add_subdirectory` directive into `iceoryx_meta/CMakeLists.txt` in the `if(EXAMPLES)` section.
4. Consider using [geoffrey](https://github.com/elBoberido/geoffrey#geoffrey---syncs-source-code-to-markdown-code-blocks) for syncing code in code blocks with the respective source files
5. Add integration test for example
6. [Record an asciicast](./tools/website/how-to-record-asciicast.md) and embed image link

## Testing

We use [Google test](https://github.com/google/googletest) for our unit and integration tests. We require compatibility
with the version 1.8.1.

Have a look at our [best practice guidelines](./doc/website/advanced/best-practice-for-testing.md) for writing tests and
[installation guide for contributors](./doc/website/advanced/installation-guide-for-contributors.md#build-and-run-tests) for building them.

### Unit tests (aka module tests)

Unit tests are black box tests that test the public interface of a class. They are required for all new code.

### Integration tests

Integration tests are composition of more than one class and test their interaction. They are optional for new code.

## Coverage Scan

To ensure that the provided testcode covers the productive code you can do a coverage scan with gcov. The reporting is done with lcov and htmlgen.
You will need to install the following packages:

```bash
sudo apt install lcov
```

In iceoryx we have multiple testlevels for testcoverage: 'unit', 'integration', 'component' and ’all’ for all testlevels together. You can create reports for these different testlevels or for all tests. Coverage is done with gcc.
The coverage scan applies to Quality level 3 and partly level 2 with branch coverage.

For having a coverage report iceoryx needs to be compiled with coverage flags and the tests needs to be executed.
You can do this with one command in iceroyx folder like this:

```bash
./tools/iceoryx_build_test.sh clean build-all -c <testlevel>
```

Optionally you can use build-all option to get coverage for extensions like DDS or C-Binding.
The -c flag indicates that you want to have a coverage report and you can pass there the needed testlevel. Per default the testlevel is set to 'all'.
example:

```bash
./tools/iceoryx_build_test.sh debug build-all -c unit
```

**NOTE**
Iceoryx needs to be built as static library for working with gcov flags. The script does it automatically.

The flag `-c unit` is for having only reports for unit-tests. In the script `tools/gcov/lcov_generate.sh` is the initial scan, filtering and report generation automatically done.

All reports are stored locally in build/lcov as html report (index.html). In Github, we are using [codecov](https://about.codecov.io) for a general reporting of the code coverage.
Codecov gives a brief overview of the code coverage and also indicates in Pull-Requests if newly added code is not covered by tests.
If you want to download the detailed html reports from the Pull-Requests or master build you can do it by the following way:

1. Open the "Checks" view in the PR
2. Open the "Details" link for the check `iceoryx-coverage-doxygen-ubuntu` in `Test Coverage + Doxygen Documentation`
3. On the right side you find a menu button `Artifacts` which shows `lcov-report` as download link

## Legal & Compliance

### Safety & security

The iceoryx maintainers aim for [ASIL-D](https://en.wikipedia.org/wiki/Automotive_Safety_Integrity_Level#ASIL_D)
compliance. The [ISO26262](https://en.wikipedia.org/wiki/ISO_26262) is also a good read-up if you want to learn more
about automotive safety. A nice introduction [video](https://www.youtube.com/watch?v=F4GzsA00s5I) was presented on
CppCon 2019.

If you want to report a vulnerability, please use the [Eclipse process](https://www.eclipse.org/security/).

#### Static code analysis

The iceoryx maintainers have a partnership with [Axivion](https://www.axivion.com/en/) and use their
[Axivion Suite](https://www.axivion.com/en/products/static-code-analysis/) to run a static-code analysis.

Github [labels](https://github.com/eclipse-iceoryx/iceoryx/labels) are used to group issues into the rulesets:

| Ruleset name | Github issue label | Priority |
|---|---|---|
| [Adaptive AUTOSAR](https://www.autosar.org/fileadmin/user_upload/standards/adaptive/17-03/AUTOSAR_RS_CPP14Guidelines.pdf) C++14 | AUTOSAR | :star::star::star: |
| [SEI CERT C++](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682) 2016 Coding Standard | CERT | :star::star: |
| [MISRA](https://www.misra.org.uk/) C++ 2008 | MISRA | :star: |

The enabled rules can be found [here](./tools/axivion/axivion_config.json). It is possible that not the whole codebase
follows these rules, things are work in progress. But this is where we want to go.

If one of the rules is not followed, a rationale is added in the following manner:

With a comment in the same line:

```cpp
    *mynullptr = foo; // PRQA S 4242 # Short description why
```

With a comment one line above (with the number after the warning number, next ’n’ lines are inclusive)

```cpp
    // PRQA S 4242 1 # Short description why
    *mynullptr = foo;
```

Scan results of the `master` branch are available on a [Axivion dashboard](https://iceoryx-axivion.apex.ai/). Please
contact one of the maintainers, if you're interested in getting access.

Don't be afraid if you don't have Axivion available. As we want to make it easy for developers to contribute,
please raise a pull request and one of the maintainers will provided you access to the [dashboard](https://iceoryx-axivion.apex.ai/).

As an alternative it is also possible to use Perforce's
[Helix QAC++ 2019.2](https://www.perforce.com/products/helix-qac) to perform a static-code analysis.

### Header

Each source file needs to have this header:

```cpp
    // Copyright (c) [YEAR OF INITIAL CONTRIBUTION] - [YEAR LAST CONTRIBUTION] by [CONTRIBUTOR]. All rights reserved.
    //
    // Licensed under the Apache License, Version 2.0 (the "License");
    // you may not use this file except in compliance with the License.
    // You may obtain a copy of the License at
    //
    //     http://www.apache.org/licenses/LICENSE-2.0
    //
    // Unless required by applicable law or agreed to in writing, software
    // distributed under the License is distributed on an "AS IS" BASIS,
    // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    // See the License for the specific language governing permissions and
    // limitations under the License.
    //
    // SPDX-License-Identifier: Apache-2.0
```

Note: The date is either a year or a range of years with the first and last years of the range separated by a dash. For example: "2004" (initial and last contribution in the same year) or "2000 - 2004". The first year is when the contents of the file were first created and the last year is when the contents were last modified. The years of contribution should be ordered in chronological order, thus the last date in the list should be the year of the most recent contribution. If there is a gap between contributions of one or more calendar years, use a comma to separate the disconnected contribution periods (e.g. "2000 - 2004, 2006").

Example:

```cpp
    // Copyright (c) 2019 - 2020, 2022 by Acme Corporation. All rights reserved.
    // Copyright (c) 2020 - 2022 by Jane Doe <jane@example.com>. All rights reserved.
    //
    // Licensed under the Apache License, Version 2.0 (the "License");
    // you may not use this file except in compliance with the License.
    // You may obtain a copy of the License at
    //
    //     http://www.apache.org/licenses/LICENSE-2.0
    //
    // Unless required by applicable law or agreed to in writing, software
    // distributed under the License is distributed on an "AS IS" BASIS,
    // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    // See the License for the specific language governing permissions and
    // limitations under the License.
    //
    // SPDX-License-Identifier: Apache-2.0
```

**_NOTE:_**  For scripts or CMake files you can use the respective comment syntax like `#` for the header.

## Quality levels

The CMake targets are developed according to the
[ROS quality levels](https://github.com/ros-infrastructure/rep/blob/master/rep-2004.rst).
Despite developing some of the targets according to automotive standards like ISO26262, the code base standalone
does NOT legitimize the usage in a safety-critical system. All requirements of a lower quality level are included in
higher quality levels e.g. quality level 4 is included in quality level 3.

### Quality level 5

This quality level is the default quality level. It is meant for examples and helper tools.

* Derived from [ROS quality level 5](https://www.ros.org/reps/rep-2004.html#quality-level-5)
  * Reviewed by two approver
  * No compiler warnings
  * License and copyright statement available
  * No version policy required
  * No unit tests required

### Quality level 4

* Derived from [ROS quality level 4](https://www.ros.org/reps/rep-2004.html#quality-level-4)
  * Basic unit tests are required
  * Builds and runs on Windows, MacOS, Linux and QNX

### Quality level 3

* Derived from [ROS quality level 3](https://www.ros.org/reps/rep-2004.html#quality-level-3)
  * Doxygen and documentation required
  * Test specification required
  * Version policy required

### Quality level 2

This quality level is meant for all targets that need tier 1 support in ROS 2.

* Derived from [ROS quality level 2](https://www.ros.org/reps/rep-2004.html#quality-level-2)
  * Must have a [quality declaration document](https://www.ros.org/reps/rep-2004.html#quality-declaration-template)

### Quality level 1

* Derived from [ROS quality level 1](https://www.ros.org/reps/rep-2004.html#quality-level-1)
  * Version policy for stable API and ABI required
  * [ASPICE](https://beza1e1.tuxen.de/aspice.html) SWE.6 tests available
  * Performance tests and regression policy required
  * Static code analysis warnings in Axivion addressed
  * Enforcing the code style is required
  * Unit tests have full statement and branch coverage

### Quality level 1+

This quality level goes beyond the ROS quality levels and contains extensions.

* Code coverage according to [MC/DC](https://en.wikipedia.org/wiki/Modified_condition/decision_coverage) available

## Training material recommended for contributors

* Effective C++ by Scott Meyers
* [Unit Testing and the Arrange, Act and Assert (AAA) Pattern](https://medium.com/@pjbgf/title-testing-code-ocd-and-the-aaa-pattern-df453975ab80) by Paulo Gomes
* The C++ Standard Library by Nicolai M. Josuttis
* Modern C++ Programming with Test-Driven Development: Code Better, Sleep Better by Jeff Langr
* Modern C++ Design: Generic Programming and Design Patterns by Andrei Alexandrescu
* Exceptional C++ by Herb Sutter
* C++ Concurrency in Action by Anthony Williams
