#!/bin/bash

# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
# Copyright (c) 2020 - 2021 by Apex AI Inc. All rights reserved.
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
BUILD_FOLDER=${1:-$PWD}
COMMAND=${2:-$scan}
TEST_SCOPE=${3:-$unit}
OUTPUT_FOLDER=$BUILD_FOLDER/build/lcov

mkdir -p $OUTPUT_FOLDER

case "$2" in
    "initial")
        lcov -z
        lcov -c -i -d $BUILD_FOLDER -o $OUTPUT_FOLDER/iceoryx_init.info --no-external --rc lcov_branch_coverage=1
        ;;
    "scan")
        lcov -c -d $BUILD_FOLDER -o $OUTPUT_FOLDER/iceoryx_test.info --no-external --rc lcov_branch_coverage=1

        lcov -a $OUTPUT_FOLDER/iceoryx_init.info --add-tracefile $OUTPUT_FOLDER/iceoryx_test.info -o $OUTPUT_FOLDER/iceoryx_full.info \
        --rc lcov_branch_coverage=1

        lcov -o $OUTPUT_FOLDER/iceoryx_lcov_result_"$TEST_SCOPE".info --rc lcov_branch_coverage=1 -r $OUTPUT_FOLDER/iceoryx_full.info \
        "*/build/*" "*/test/*" "*/testing/*" "*/iceoryx_examples/*"

        genhtml $OUTPUT_FOLDER/iceoryx_lcov_result_"$TEST_SCOPE".info -o $OUTPUT_FOLDER --config-file --legend --show-details --branch-coverage
        ;;
esac
