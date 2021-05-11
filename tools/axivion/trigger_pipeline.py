#!/usr/bin/env python3

# Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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


import os
import requests
import sys
import time


def main():
    data = {
        'token': os.environ['AXIVION_TRIGGER_TOKEN'],
        'ref': os.environ.get('AXIVION_REF_NAME', 'master'),
    }
    commit_sha = os.environ['GITHUB_SHA']
    r = requests.post(
        f'https://gitlab.com/api/v4/projects/24081973/trigger/pipeline?variables[ICEORYX_SHA]={commit_sha}',
        json=data)

    if r.status_code != 201:
        print(f'ERROR: Pipeline trigger failed: {r.status_code}', file=sys.stderr)
        sys.exit(1)

    pipeline_id = r.json()['id']
    print(pipeline_id)


if __name__ == "__main__":
     main()
