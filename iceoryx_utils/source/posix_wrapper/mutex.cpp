// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/platform/platform_correction.hpp"

#include <cassert>
#include <utility>

namespace iox
{
namespace posix
{
mutex::mutex(bool f_isRecursive)
{
    pthread_mutexattr_t attr;
    bool isInitialized{true};
    isInitialized &=
        !cxx::makeSmartC(pthread_mutexattr_init, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &attr).hasErrors();
    isInitialized &= !cxx::makeSmartC(pthread_mutexattr_setpshared,
                                      cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE,
                                      {0},
                                      {},
                                      &attr,
                                      PTHREAD_PROCESS_SHARED)
                          .hasErrors();
    isInitialized &= !cxx::makeSmartC(pthread_mutexattr_settype,
                                      cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE,
                                      {0},
                                      {},
                                      &attr,
                                      f_isRecursive ? PTHREAD_MUTEX_RECURSIVE_NP : PTHREAD_MUTEX_FAST_NP)
                          .hasErrors();
    isInitialized &= !cxx::makeSmartC(pthread_mutexattr_setprotocol,
                                      cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE,
                                      {0},
                                      {},
                                      &attr,
                                      PTHREAD_PRIO_NONE)
                          .hasErrors();

    isInitialized &=
        !cxx::makeSmartC(pthread_mutex_init, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &m_handle, &attr)
             .hasErrors();

    isInitialized &=
        !cxx::makeSmartC(pthread_mutexattr_destroy, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &attr)
             .hasErrors();

    cxx::Ensures(isInitialized && "Unable to create mutex");
}

mutex::~mutex()
{
    auto destroyCall =
        cxx::makeSmartC(pthread_mutex_destroy, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &m_handle);

    cxx::Ensures(!destroyCall.hasErrors() && "Could not destroy mutex");
}

pthread_mutex_t mutex::get_native_handle() const noexcept
{
    return m_handle;
}

bool mutex::lock()
{
    return !cxx::makeSmartC(pthread_mutex_lock, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &m_handle)
                .hasErrors();
}

bool mutex::unlock()
{
    return !cxx::makeSmartC(pthread_mutex_unlock, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &m_handle)
                .hasErrors();
}

bool mutex::try_lock()
{
    return !cxx::makeSmartC(pthread_mutex_trylock, cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE, {0}, {}, &m_handle)
                .hasErrors();
}
} // namespace posix
} // namespace iox
