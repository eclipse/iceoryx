// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_utils/platform/semaphore.hpp"
#include "iceoryx_utils/platform/time.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <thread>

iox_sem_t::iox_sem_t() noexcept
{
}

iox_sem_t::iox_sem_t(iox_sem_t&& rhs) noexcept
{
    *this = std::move(rhs);
}

iox_sem_t::~iox_sem_t()
{
}

iox_sem_t& iox_sem_t::operator=(iox_sem_t&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_value.store(rhs.m_value.load());
        m_handle = rhs.m_handle;
        m_hasPosixHandle = rhs.m_hasPosixHandle;

        rhs.m_value.store(0);
    }
    return *this;
}

int iox_sem_getvalue(iox_sem_t* sem, int* sval)
{
    *sval = sem->m_value.load(std::memory_order_relaxed);
    return 0;
}

int iox_sem_post(iox_sem_t* sem)
{
    int retVal{0};
    if (sem->m_hasPosixHandle)
    {
        retVal = sem_post(sem->m_handle.posix);
        if (retVal == 0)
        {
            sem->m_value.fetch_add(1, std::memory_order_relaxed);
        }
    }
    else
    {
        pthread_mutex_lock(&sem->m_handle.condition.mtx);
        sem->m_value.fetch_add(1, std::memory_order_relaxed);
        pthread_mutex_unlock(&sem->m_handle.condition.mtx);

        pthread_cond_signal(&sem->m_handle.condition.variable);
    }
    return retVal;
}

int iox_sem_wait(iox_sem_t* sem)
{
    int retVal{0};
    if (sem->m_hasPosixHandle)
    {
        retVal = sem_wait(sem->m_handle.posix);
        if (retVal == 0)
        {
            sem->m_value.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    else
    {
        pthread_mutex_lock(&sem->m_handle.condition.mtx);
        while (sem->m_value.load(std::memory_order_relaxed) == 0)
        {
            pthread_cond_wait(&sem->m_handle.condition.variable, &sem->m_handle.condition.mtx);
        }
        sem->m_value.fetch_sub(1, std::memory_order_relaxed);
        pthread_mutex_unlock(&sem->m_handle.condition.mtx);
        return retVal;
    }

    return retVal;
}

int iox_sem_trywait(iox_sem_t* sem)
{
    int retVal{0};
    if (sem->m_hasPosixHandle)
    {
        retVal = sem_trywait(sem->m_handle.posix);
        if (retVal == 0)
        {
            sem->m_value.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    else
    {
        pthread_mutex_lock(&sem->m_handle.condition.mtx);
        if (sem->m_value.load(std::memory_order_relaxed) > 0)
        {
            sem->m_value.fetch_sub(1, std::memory_order_relaxed);
        }
        else
        {
            errno = EAGAIN;
            retVal = -1;
        }
        pthread_mutex_unlock(&sem->m_handle.condition.mtx);
    }

    return retVal;
}

int iox_sem_timedwait(iox_sem_t* sem, const struct timespec* abs_timeout)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    static constexpr int64_t NANOSECONDS_PER_SECOND = 1000000000;
    static constexpr int64_t NANOSECONDS_PER_MICROSECOND = 1000;

    int64_t timeoutInNanoSeconds = std::max(0ll,
                                            (abs_timeout->tv_sec - tv.tv_sec) * NANOSECONDS_PER_SECOND
                                                + abs_timeout->tv_nsec - tv.tv_usec * NANOSECONDS_PER_MICROSECOND);

    if (sem->m_hasPosixHandle)
    {
        int tryWaitCall = sem_trywait(sem->m_handle.posix);
        constexpr int ETIMEDOUT_PLUS_256 = ETIMEDOUT + 256;
        if (errno == ETIMEDOUT_PLUS_256)
        {
            errno &= 0xFF;
        }
        if (tryWaitCall == -1 && errno != EAGAIN)
        {
            return -1;
        }
        else if (tryWaitCall == -1 && errno == EAGAIN && timeoutInNanoSeconds == 0)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        else if (tryWaitCall == 0)
        {
            sem->m_value.fetch_sub(1, std::memory_order_relaxed);
            return 0;
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(timeoutInNanoSeconds));

        tryWaitCall = sem_trywait(sem->m_handle.posix);
        if (errno == ETIMEDOUT_PLUS_256)
        {
            errno &= 0xFF;
        }
        errno &= 0xFF;
        if (tryWaitCall == -1 && errno == EAGAIN)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        else if (tryWaitCall == -1 && errno != EAGAIN)
        {
            return -1;
        }
        else if (tryWaitCall == 0)
        {
            sem->m_value.fetch_sub(1, std::memory_order_relaxed);
            return 0;
        }
    }
    else
    {
        pthread_mutex_lock(&sem->m_handle.condition.mtx);
        if (sem->m_value.load(std::memory_order_relaxed) == 0)
        {
            int result =
                pthread_cond_timedwait(&sem->m_handle.condition.variable, &sem->m_handle.condition.mtx, abs_timeout);
            if (result != 0)
            {
                if (result == ETIMEDOUT)
                {
                    errno = ETIMEDOUT;
                }
                pthread_mutex_unlock(&sem->m_handle.condition.mtx);
                return -1;
            }
        }
        sem->m_value.fetch_sub(1, std::memory_order_relaxed);
        pthread_mutex_unlock(&sem->m_handle.condition.mtx);
        return 0;
    }

    return -1;
}

int iox_sem_close(iox_sem_t* sem)
{
    // will only be called by named semaphores which are in our case
    // posix semaphores
    // therefor we have to call delete since we created the iox_sem_t object
    // with new in iox_sem_open
    int retVal = sem_close(sem->m_handle.posix);
    delete sem;
    return retVal;
}

int iox_sem_destroy(iox_sem_t* sem)
{
    // will only be called by unnamed semaphores which are in our
    // case dispatch semaphores
    pthread_mutex_destroy(&sem->m_handle.condition.mtx);
    pthread_cond_destroy(&sem->m_handle.condition.variable);
    // no delete necessary since the user is providing memory here
    return 0;
}

int iox_sem_init(iox_sem_t* sem, int, unsigned int value)
{
    // init mutex attribute
    pthread_mutexattr_t mutexAttr;
    if (pthread_mutexattr_init(&mutexAttr) != 0)
    {
        printf("failed to initialize mutexattr\n");
        return -1;
    }

    if (pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED) != 0)
    {
        pthread_mutexattr_destroy(&mutexAttr);
        printf("unable to set the shared process mutex attribute\n");
        return -1;
    }

    // init condition variable attribute
    pthread_condattr_t condAttr;
    if (pthread_condattr_init(&condAttr) != 0)
    {
        pthread_mutexattr_destroy(&mutexAttr);
        printf("failed to initialize condattr\n");
        return -1;
    }

    if (pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED) != 0)
    {
        pthread_condattr_destroy(&condAttr);
        pthread_mutexattr_destroy(&mutexAttr);
        printf("unable to set the shared process condition variable attribute\n");
        return -1;
    }

    if (pthread_mutex_init(&sem->m_handle.condition.mtx, &mutexAttr) != 0)
    {
        pthread_condattr_destroy(&condAttr);
        pthread_mutexattr_destroy(&mutexAttr);
        printf("failed to initialize inter process mutex\n");
        return -1;
    }

    if (pthread_cond_init(&sem->m_handle.condition.variable, &condAttr) != 0)
    {
        pthread_mutex_destroy(&sem->m_handle.condition.mtx);
        pthread_condattr_destroy(&condAttr);
        pthread_mutexattr_destroy(&mutexAttr);
        printf("failed to initialize inter process condition variable\n");
        return -1;
    }

    pthread_condattr_destroy(&condAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    sem->m_hasPosixHandle = false;
    sem->m_value.store(static_cast<int>(value), std::memory_order_relaxed);

    return 0;
}

int iox_sem_unlink(const char* name)
{
    return sem_unlink(name);
}

iox_sem_t* iox_sem_open_impl(const char* name, int oflag, ...)
{
    if (strlen(name) == 0 || name[0] == 0)
    {
        return reinterpret_cast<iox_sem_t*>(SEM_FAILED);
    }

    // sem_open creates a named semaphore which is corresponding to a file.
    // the posix version creates also something on the heap and if you would
    // like to share this semaphore you should open the named semaphore with
    // sem_open in another process. Sharing this semaphore via shared memory
    // is always wrong!
    // Hence, it is allowed to use new/delete in this case.
    iox_sem_t* sem = new iox_sem_t;

    if (oflag & (O_CREAT | O_EXCL))
    {
        va_list va;
        va_start(va, oflag);
        // mode_t is an alias for unsigned short but this causes undefined
        // behavior in va_arg since it is a promotable type - which will be
        // promoted to int
        mode_t mode = static_cast<mode_t>(va_arg(va, unsigned int));
        unsigned int value = va_arg(va, unsigned int);
        va_end(va);

        sem->m_handle.posix = sem_open(name, oflag, mode, value);
        sem->m_value.store(static_cast<int>(value), std::memory_order_relaxed);
    }
    else
    {
        sem->m_handle.posix = sem_open(name, oflag);
    }

    if (sem->m_handle.posix == SEM_FAILED)
    {
        delete sem;
        return reinterpret_cast<iox_sem_t*>(SEM_FAILED);
    }

    return sem;
}
