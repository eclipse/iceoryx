// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/platform/unistd.hpp"
#include "iceoryx_utils/platform/win32_errorHandling.hpp"

HandleTranslator& HandleTranslator::getInstance() noexcept
{
    static HandleTranslator globalHandleTranslator;
    return globalHandleTranslator;
}

HANDLE HandleTranslator::get(const int handle) const noexcept
{
    return m_handleList[static_cast<size_t>(handle)].windowsHandle;
}

int HandleTranslator::add(HANDLE handle) noexcept
{
    for (int64_t limit = m_handleList.size(), k = 0; k < limit; ++k)
    {
        if (m_handleList[k].windowsHandle == nullptr)
        {
            m_handleList[k].windowsHandle = handle;
            return k;
        }
    }

    m_handleList.emplace_back(handle_t{handle});
    return m_handleList.size() - 1;
}

void HandleTranslator::remove(int handle) noexcept
{
    m_handleList[static_cast<uint64_t>(handle)].windowsHandle = nullptr;
}

int ftruncate(int fildes, off_t length)
{
    return 0;
}

long sysconf(int name)
{
    return 0;
}

int iox_close(int fd)
{
    auto success = Win32Call(CloseHandle(HandleTranslator::getInstance().get(fd)));
    HandleTranslator::getInstance().remove(fd);
    if (success == 0)
    {
        return -1;
    }
    return 0;
}
