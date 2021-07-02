# Fuzzing Iceoryx

## What is fuzzing?

Fuzzing is a method for dynamic code analysis which can be used to find several types of bugs which could also be security vulnerabilities. These vulnerabilities which could lead to a crash such as buffer overflow, use-after-free, dangling pointer and more can be found.
More information about fuzzing can for example be found [here](https://owasp.org/www-community/Fuzzing "OWASP Fuzzing").

## Why should you use fuzzing?

1. **It is easy to use** - These fuzz wrappers provide you a possibility to integrate a fuzzer very fast. It can then run for several hours or days and find bugs automatically.
2. **It is effective** - Lots of security vulnerabilities in the wild are found with fuzzers.
3. **It has a low false positive rate** - If the program crashes, then it is very likely that something in your code was wrong.

## How to build

First of all, you need to install the fuzzer you want to use. The american fuzzy lop (afl) for example can be found [here](https://github.com/google/AFL "Github AFL") and the installation manual can be found [here](https://github.com/google/AFL/blob/master/docs/INSTALL).  
For some fuzzers like the afl, the code needs to be instrumented first. For afl you can download the [LLVM](https://llvm.org/releases/download.html "LLVM"). Afterwards you need to compile the code with an afl compiler which does the instrumentation such as [afl-clang-fast](https://github.com/google/AFL/tree/master/llvm_mode).

Go to your iceoryx folder and change the compiler for example via cmake:

``` cmake -Bbuild -Hiceoryx_meta -DBUILD_TEST=True -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_CXX_COMPILER=afl-clang-fast++ ```

Afterwards you can build the project for example via:

``` cmake --build build ```

Iceoryx is now ready to be fuzzed.

## How to start testing

The fuzz-wrapper binary can be found in ``` [path_to_iceoryx]/iceoryx_eclipse/build/posh/test/posh_fuzztests ```

To start fuzzing with afl, you need two folders: One which contains examples of valid messages which can be send to the interface and one folder where the fuzzing results are stored. There is already an example folder with a valid text message to test the uds communication. It can be found within ``` [path_to_iceoryx]/iceoryx_posh/test/fuzztests/fuzz_input/ ```

The fuzz wrappers currently support three different interfaces: the unix domain socket communication, the message parsing method and the toml configuration file parser.  

1. The unix domain socket can either be tested by starting RouDi and sending messages to RouDi via uds or by directly invoking the message parsing function within RouDi. The first solution has the advantage, that the messages from uds to RouDi take the same path as normal messages from applications would do. The drawback however is that RouDi needs to be started with all the overhead which would not be necessary for this test which therefore slows the test a little bit.

2. By invoking the message parsing function directly, the method is independent from the underlying protocol such as uds. It should also be slightly faster since some functions are not invoked compared to uds fuzzing. However, a RouDi thread is also started with this approach because otherwise it wouldn't be possible to invoke the message process function within RouDi without directly modifying the code in RouDi.

3. As a third use-case, the toml configuration parser can be tested. An example of a toml file can be found here: ``` [path_to_iceoryx]/iceoryx_posh/etc/iceoryx/roudi_config_example.toml ```.
If you chose this method, you also need to set ``` -t or --toml-file <PATH_TO_FILE> ```. A file needs to be specified which can be used to write in the messages which will send to the toml configuration parser.

The interface you want to fuzz can be chosen with ``` -f or --fuzzing-api + entering the argument: uds, com or toml ```. The argument uds is used for use-case 1, com for use-case 2 and toml for use-case 3.

The way how the fuzzer sends messages to the interface can also be specified via ``` -m or --input-mode ```

1. The messages can be sent via stdin (``` -m stdin ```)
2. The messages can be sent via using the command line (``` -m cl ```) followed by ``` -i  or --command-line-input ``` with the message following as argument or ``` -c --command-line-file ``` to specify a file where the message you want to send is located. By using ``` -c or --command-line-file ```, the complete file is sent as one message and is not sent line by line.

Additionally, the log level can be chosen via ``` -l or --log-level ``` such that messages are printed for debug or not. The default is to switch logging off. 3 arguments are possible:

1. off: to disable logging (standard case)
2. fatal: log fatal messages
3. debug: log debug messages

As an example: afl can be started for uds with: ``` afl-fuzz -i [path_to_input_example] -o [path_where_output_of_fuzzer_is_written_to] [path_to_iceoryx]/build/posh/test/posh_fuzztests -f uds -m stdin ```

Or to have a more concrete example with afl-fuzz:

```shell
cd build/posh/test 

afl-fuzz -i ../../../iceoryx_posh/test/fuzztests/fuzz_input/ -o ./fuzz_output ./posh_fuzztests -f uds -m stdin

```

It is possible that afl runs out of memory or gets a timeout. To solve these problems you can increase the timeout and the memory for example with the following command:

```shell
cd build/posh/test 

afl-fuzz -m 1000000 -t 10000 -i ../../../iceoryx_posh/test/fuzztests/fuzz_input/ -o ./fuzz_output ./posh_fuzztests -f uds -m stdin

```

The outputs can then be found in fuzz_output. If AFL found a crash, a file is created in fuzz_output/crashes which contains the input leading to this crash.
