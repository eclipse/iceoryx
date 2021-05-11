# [ASPICE](http://www.automotivespice.com/) example [WIP]

## Purpose

This document defines the documentation guidelines for SWE 3 (Detailed Design) and SWE 4 (Unit Verification) with Doxygen.


## ToDo
 - provide which doxygen tags needs to be set
 - extend documentation guideline

##  Prerequisites
The documentation is generated into .html pages and latex for pdf generation
To generate the documentation you need to install:
```bash
sudo apt install doxygen graphviz texlive-base texlive-font-utils texlive-latex-extra
```
The documentation we use plantuml for describing dynamic behavior in the code (sequence or activity diagrams).
Needed package:
```bash
sudo apt install plantuml
```

##  Howto document your code
The doxygen documentation consist of two parts, the documentation with specific tags in the code and a doxygen file which defines
the parameters for the generated files. Examples can be found in the subfolders. 

Some general rules:
 - the implementation documentation should never describe what happens, that does already the code for you. It should describe why it is implemented in the way it is.
- every doxygen comment line always begin with `///`, no C-style comment blocks with `/**` allowed
- every doxygen tag always begin with `@` e.g. `@brief comment`, no `\` for tags allowed
- documentation is done in header files only
-  never add fancy lines in your comments like this: (exception is the fileheader which is iceoryx-wide standard)
 ```cpp
    //=================================
    // My bad comment
    //=================================
    // bla bla bla
    //---------------------------------
    void badCommentedFunction();

    /// good comment: bla bla bla
    void goodCommentedFunction();
 ```

##  Tags
### Allowed Tags
A list of available doxygen tags can be found [here](https://www.doxygen.nl/manual/commands.html).
Here we provide a list with the most usual tags in iceoryx:
 - `/// @brief` short description
 - `/// @details` more text if needed
 - `/// @note` place infos here if they are not that much important but good for explanation
 - `/// @attention` give here important information to the user/developer, keep as short as possible
 - `/// @bug` can be used to mark a bug, The ticket issue number in Github should be mentioned here e.g. "iox-#123"
 - `/// @return` describe return value here
 - `/// @param[in/out]`
 - `/// @tparam <template-parameter-name>` for template parameters
 - `/// @code` and `/// @endcode` for documenting example code, this is useful for interfaces when you want to show a simple example how to use the class.
 - `/// @startuml` and `/// @enduml` for creating simple diagrams in PlantUML

### Custom Tags
In iceoryx are also custom tags used for extended documentation like errors.
 - `/// @concurrent` use this tag if the code is used in multithreading context for comments about it
 - `/// @req` used to set links to a database with requirements, see [iceoryx_component.hpp](example/iceoryx_component/include/example_component.hpp)
 - `/// @link` used to set general links additional information, see [iceoryx_component.hpp](example/iceoryx_component/include/example_component.hpp)
 - `/// @swcomponent` marks the relationship to the component
 - `/// @error` used for describing the error behavior e.g. error-handler is called
 - `/// @generatedcode` for marking code as generated

### Forbidden Tags
 - `/// @unit` , use a type safe implementation if possible!
 - `/// @min` , use a type safe implementation if possible!
 - `/// @max` , use a type safe implementation if possible!

### Doxygen Generation
For generating the documentation out of the code is CMake in combination with doxygen used.
In iceoryx_meta is a build flag `BUILD_DOC` defined which generates for you the html, xml and latex documentation. There is no need to build iceoryx beforehand.
```bash
cmake -Bbuild -Hiceoryx_meta -DBUILD_DOC=ON
cmake --build build
```
The output ist stored under `build/doc/<iceoryx_component>` where you can find the folders `html`, `xml` and `latex`.
If you want to generate the pdf files then you can use the `tools/iceoryx_build_test.sh`:
```bash
./tools/iceoryx_build_test.sh doc
```
The generated pdf files are generated into `build/doc`. Please note that iceoryx is not build

Generally, you will not find any Doxygen file in our repo because we let CMake generate it.
In [Cmake](CMakeLists.txt) is the command `doxygen_add_docs` which does the job.
There, we are also setting some parameters and the aliases for the custom tags. Aliases with an `xrefitem` create a page where all occurrences of the corresponding tag are collected.
### file header
Please see [Header](https://github.com/eclipse-iceoryx/iceoryx/blob/master/CONTRIBUTING.md#header).

### Include guards
Every header and inl file needs to have an include guard. Pragma once is not allowed.
The include guard is placed directly under the file header and needs to have the following scheme:

   * IOX_[COMPONENT_W/O_ICEORYX][SUB_COMPONENT][FILE_NAME]
   * IOX_[COMPONENT_W/O_ICEORYX][SUB_COMPONENT][SUB_COMPONENT]_[FILE_NAME]

See the header files under iceoryx/doc/aspice_swe3_4/example.

### PlantUML

if you want to make complex aspects of your implementation visible, you can write plantuml directly into the 
doxygen description of your class. An example can be found at [example_base_class.hpp](example/iceoryx_component/source/example_module/example_base_class.hpp)

Having PlantUML installed is not a must. If you want to use it you need to install it, and the variable `PLANTUML_JAR_PATH` needs to be set.
CMake will try to find the plantuml.jar in `/usr/share/plantuml`, if it is not found then it tries to use the environment variable.
You can set it by calling:
```bash
export PLANTUML_JAR_PATH=/custom/path
```
For good examples on PlantUML checkout https://plantuml.com.

### Pull-Request

In iceoryx Pull-Requests are pdf files generated and uploaded to the Github artifact store. 

If you want to download the pdf files from the Pull-Requests or master build you can do it by the following way:

 * Open the "Checks" view in the PR
 * Open the "Details" link for the check `iceoryx-coverage-doxygen-ubuntu` in `Test Coverage + Doxygen Documentation`
 * On the right side you find a menu button `Artifacts` which shows `iceoryx-pdf` as download link
