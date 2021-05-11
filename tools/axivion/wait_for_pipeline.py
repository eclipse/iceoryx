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

TOKEN = {
    'PRIVATE-TOKEN': os.environ['AXIVION_READ_API_TOKEN'],
}


def get_status(pipeline_id):
    r = requests.get(f'https://gitlab.com/api/v4/projects/24081973/pipelines/{pipeline_id}', headers=TOKEN)

    if r.status_code != 200:
        print('ERROR: Could not check the status of the pipeline.', file=sys.stderr)
        sys.exit(1)

    return r.json()['status']


def print_logs(pipeline_id):
    r = requests.get(f'https://gitlab.com/api/v4/projects/24081973/pipelines/{pipeline_id}/jobs', headers=TOKEN)

    if r.status_code != 200:
        print('ERROR: Could not get job_id from pipeline.', file=sys.stderr)
        sys.exit(1)

    # Assumes pipeline only has one job
    job_id = r.json()[0]['id']

    r = requests.get(f'https://gitlab.com/api/v4/projects/24081973/jobs/{job_id}/trace', headers=TOKEN)

    if r.status_code != 200:
        print('ERROR: Could not get job logs.', file=sys.stderr)
        sys.exit(1)

    print(r.text)


def main(pipeline_id):
    status = get_status(pipeline_id)

    while status in ['running', 'pending', 'created']:
        time.sleep(30)
        status = get_status(pipeline_id)

    print_logs(pipeline_id)

    if status in ['failed', 'canceled', 'skipped'] :
        print(f'ERROR: Pipeline {status}', file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main(sys.argv[1])
