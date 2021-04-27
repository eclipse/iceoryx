# Copyright (c) 2019 by Cornel Izbasa. All rights reserved.
# Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
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


# ======================================================================================================================== #

FROM ubuntu:bionic as builder
ARG REVISION
ARG B_ICEORYX_BUILD
ENV ICEORYX_BUILD=$B_ICEORYX_BUILD

# Dependencies
RUN apt-get update && apt-get install -y \
        cmake \
        bison \
        libacl1-dev \
        libncurses5-dev \
        pkg-config \
        git \
        wget



ADD . /iceoryx
WORKDIR /iceoryx

RUN ./tools/iceoryx_build_test.sh build-all

# ======================================================================================================================== #

FROM ubuntu:bionic as roudi

RUN apt-get update && apt-get install -y \
        screen \
        fish

COPY --from=builder /iceoryx/build/install/prefix/lib/lib* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /iceoryx/build/dependencies/install/lib/lib* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /iceoryx/build/install/prefix/bin/* /usr/bin/
COPY --from=builder /iceoryx/build/install/prefix/etc/* /etc/
COPY --from=builder /iceoryx/tools/docker/.screenrc /root

ENTRYPOINT ["/usr/bin/iox-roudi"]
