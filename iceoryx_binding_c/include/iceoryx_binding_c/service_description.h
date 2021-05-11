// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_BINDING_C_SERVICE_DESCRIPTION_H
#define IOX_BINDING_C_SERVICE_DESCRIPTION_H

#include <stdint.h>

typedef struct
{
    uint16_t serviceId;
    uint16_t instanceId;
    uint16_t eventId;

    char serviceString[100U];
    char instanceString[100U];
    char eventString[100U];
} iox_service_description_t;

#endif
