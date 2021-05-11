#!/bin/bash

# Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

# Necessary tools:
# mkdocs-awesome-pages-plugin, v2.5.0
# mkdocs-material, v7.0.7+insiders.2.5.0
# markdown-include, v0.6.0
# mkdocs, v1.1.2
# Doxygen, 1.8.17
# doxybook2, v1.3.1
# mike, v1.0.0

set -e

WORKSPACE=$(git rev-parse --show-toplevel)
BASENAME=$(basename $WORKSPACE)
WEBREPO="git@github.com:eclipse-iceoryx/iceoryx-web.git"
TYPE=${1:-local} #`local` starts a local webserver to inspect the results, `publish` pushes the generated doc to iceoryx_web
VERSION=$2
BRANCH=$3

if [ "$BASENAME" != "iceoryx" ]; then
    echo "Git folder must be named iceoryx!"
    exit 1
fi

cd $WORKSPACE

git checkout $BRANCH

# Generate doxygen
cmake -Bbuild -Hiceoryx_meta -DBUILD_DOC=ON
cd $WORKSPACE/build
make -j8 doxygen_iceoryx_posh doxygen_iceoryx_utils doxygen_iceoryx_binding_c doxygen_iceoryx_dds doxygen_iceoryx_introspection

cd $WORKSPACE

PACKAGES="utils posh c-binding DDS-gateway introspection"

# Generate doxybook2 config files, to have correct links in doxygen docu
mkdir -p $WORKSPACE/tools/website/generated/

for PACKAGE in ${PACKAGES}  ; do
    FILE=$WORKSPACE/tools/website/generated/doxybook2-$PACKAGE.json
    rm -f $FILE
    echo "{" >> $FILE
    if [ "$TYPE" == "local" ]; then
        echo "\"baseUrl\": \"/API-reference/$PACKAGE/\","  >> $FILE
    else
        echo "\"baseUrl\": \"/$VERSION/API-reference/$PACKAGE/\","  >> $FILE
    fi
    echo "\"indexInFolders\": false,"  >> $FILE
    echo "\"linkSuffix\": \"/\","  >> $FILE
    echo "\"mainPageInRoot\": false"  >> $FILE
    echo "}"  >> $FILE
done

# Generate markdown from doxygen
mkdir -p $WORKSPACE/doc/website/API-reference/utils
doxybook2 --input $WORKSPACE/build/doc/iceoryx_utils/xml/ --output $WORKSPACE/doc/website/API-reference/utils --config $WORKSPACE/tools/website/generated/doxybook2-utils.json

mkdir -p $WORKSPACE/doc/website/API-reference/posh
doxybook2 --input $WORKSPACE/build/doc/iceoryx_posh/xml/ --output $WORKSPACE/doc/website/API-reference/posh --config $WORKSPACE/tools/website/generated/doxybook2-posh.json

mkdir -p $WORKSPACE/doc/website/API-reference/c-binding
doxybook2 --input $WORKSPACE/build/doc/iceoryx_binding_c/xml/ --output $WORKSPACE/doc/website/API-reference/c-binding --config $WORKSPACE/tools/website/generated/doxybook2-c-binding.json

mkdir -p $WORKSPACE/doc/website/API-reference/DDS-gateway
doxybook2 --input $WORKSPACE/build/doc/iceoryx_dds/xml/ --output $WORKSPACE/doc/website/API-reference/DDS-gateway --config $WORKSPACE/tools/website/generated/doxybook2-DDS-gateway.json

mkdir -p $WORKSPACE/doc/website/API-reference/introspection
doxybook2 --input $WORKSPACE/build/doc/iceoryx_introspection/xml/ --output $WORKSPACE/doc/website/API-reference/introspection --config $WORKSPACE/tools/website/generated/doxybook2-introspection.json

# Remove index files

FILES="index_classes.md index_examples.md index_files.md index_modules.md index_namespaces.md index_pages.md index_groups.md"

for PACKAGE in ${PACKAGES}  ; do
    for FILE in ${FILES}  ; do
        rm -f $WORKSPACE/doc/website/API-reference/$PACKAGE/$FILE
    done
done


if [ "$TYPE" == "local" ]; then
    echo "starting local webserver"
    mkdocs serve --config-file ../iceoryx/mkdocs.yml
fi

if [ "$TYPE" == "publish" ]; then
    # Generate HTML and push to GitHub pages
    if [ ! -d "$WORKSPACE/../iceoryx-web" ]; then
        cd $WORKSPACE/../
        git clone $WEBREPO
    fi
    cd $WORKSPACE/../iceoryx-web
    mike deploy --branch main --config-file ../iceoryx/mkdocs.yml --push --update-aliases $VERSION latest
fi
