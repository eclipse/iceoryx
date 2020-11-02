// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_UTILS_POSIX_WRAPPER_MUTEX_HPP
#define IOX_UTILS_POSIX_WRAPPER_MUTEX_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/platform/pthread.hpp"

#if defined(__QNX__) || defined(__APPLE__)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

namespace iox
{
namespace posix
{
/// @brief Wrapper for a interprocess pthread based mutex which does not use
///         exceptions!
/// @code
///     #include "iceoryx_utils/internal/posix_wrapper/mutex.hpp"
///
///     int main() {
///         cxx::optional<posix::mutex> myMutex = posix::mutex::CreateMutex(iox::posix::mutex::Recursive::OFF,
///         iox::posix::mutex::Robust::OFF);
///
///         // always verify if the mutex could be created since we aren't
///         // throwing exceptions
///         if ( myMutex.has_value() ) {
///             myMutex->lock();
///             // ... do stuff
///             myMutex->unlock();
///         }
///
///         {
///             // we need to use the dereferencing operator since myMutex is
///             // a mutex wrapped in an optional
///             std::lock_guard<posix::mutex> lock(*myMutex);
///             // ...
///         }
///
///     }
class mutex
{
  public:
    enum class Recursive
    {
        ON,
        OFF
    };

    enum class Robust
    {
        ON,
        OFF
    };

    /// @brief The construction of the mutex can fail, which will lead to a call to std::terminate, which is alright
    /// for the moment since we are intending to get rid of the mutex sooner or later.
    /// @param[in] recursive Sets the recursive attribute of the mutex. If recursive is ON, the same thread, which has
    /// already locked the mutex, can lock the mutex again without getting blocked.
    /// @param[in] robust If robust is set ON, a process or thread can exit while having the lock without causing an
    /// invalid mutex. In the next lock call to this mutex the system recognizes the mutex is locked by a dead process
    /// or thread and allows the mutex to be restored.
    mutex(const Recursive recursive, const Robust robust);

    ~mutex();

    /// @brief all copy and move assignment methods need to be deleted otherwise  undefined behavior or race conditions
    /// will occur if you copy or move mutex when its possible that they are locked or will be locked.
    mutex(const mutex&) = delete;
    mutex(mutex&&) = delete;
    mutex& operator=(const mutex&) = delete;
    mutex& operator=(mutex&&) = delete;

    /// @brief Locks the mutex object and returns true if the underlying c function did not returned any error. If the
    /// mutex is already locked the method is blocking till the mutex can be locked.
    bool lock();

    /// @brief Unlocks the mutex object and returns true if the underlying c
    ///         function did not returned any error.
    ///        IMPORTANT! Unlocking and unlocked mutex is undefined behavior
    ///         and the underlying c function will report success in this case!
    bool unlock();

    /// @brief  Tries to lock the mutex object. If it is not possible to lock the mutex object try_lock will return an
    /// error. If the c function fails it will return false, otherwise true.
    bool try_lock();

    /// @brief  Returns the native handle which then can be used in pthread_mutex_** calls. Required when a
    /// pthread_mutex_** call is not abstracted with this wrapper.
    pthread_mutex_t get_native_handle() const noexcept;

  private:
    pthread_mutex_t m_handle;
};
} // namespace posix
} // namespace iox

#endif // IOX_UTILS_POSIX_WRAPPER_MUTEX_HPP
