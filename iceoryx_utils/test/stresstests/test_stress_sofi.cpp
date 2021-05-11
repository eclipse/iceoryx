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

#include "iceoryx_utils/internal/concurrent/sofi.hpp"

#include "iceoryx_utils/testing/test.hpp"

#include <cstdint>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <thread>

using namespace testing;

using SoFiData = int64_t;
constexpr SoFiData INVALID_SOFI_DATA{-1};

constexpr int64_t STRESS_TIME_HOURS{0};
constexpr int64_t STRESS_TIME_MINUTES{0};
constexpr int64_t STRESS_TIME_SECONDS{2};
constexpr std::chrono::milliseconds STRESS_TIME{
    ((STRESS_TIME_HOURS * 60 + STRESS_TIME_MINUTES) * 60 + STRESS_TIME_SECONDS) * 1000};

class SoFiStress : public Test
{
  public:
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }


    /// @brief Sets the CPU affinity for a thread
    ///
    /// @param cpu is the CPU the thread shall use
    /// @param nativeHandle is the native handle of the c++11 std::thread
    /// @return bool: if true, setting the affinity was successfull
    bool setCpuAffinity(unsigned int cpu, std::thread::native_handle_type nativeHandle)
    {
#ifdef __linux__
        // Create a cpu_set_t object representing a set of CPUs. Clear it and mark only cpu as set.
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        auto retVal = pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
        if (retVal != 0)
        {
            std::cout << "Error calling pthread_setaffinity_np: " << retVal << "; errno: " << errno << std::endl;
            return false;
        }
#else
        static_cast<void>(cpu);          // fix unused variable warning
        static_cast<void>(nativeHandle); // fix unused variable warning
#endif

        return true;
    }
};

/// @brief This tests a slow pusher and fast popper.
///
/// In this case, we have an empty SoFi where continuously a pop is performed.
/// From time to time there is a push.
/// The test covers the situation when there is a push into an empty SoFi while there is a simultaneous pop.
///
/// Consecutive values (starting with 0) are pushed into the SoFi, so the popped out values should also be consecutive.
///
/// push and pop thread should run with the same priority to have an equal chance to interrupt each other.
///
/// @note This test doesn't check for the correct memory ordering of the FIFO, but assumes that the used memory ordering
/// is correct and tests the algorithm in general, e.g. if a load/store is used instead of a compare_exchange
TEST_F(SoFiStress, SimultaneouslyPushAndPopOnEmptySoFi)
{
    iox::concurrent::SoFi<SoFiData, 10> sofi;

    SoFiData popCounter{0};
    SoFiData tryPopCounter{0};
    SoFiData pushCounter{0};
    std::atomic<bool> allowPush{false};
    std::atomic<bool> isPushing{false};
    std::atomic<bool> stopPushThread{false};
    std::atomic<bool> stopPopThread{false};

    auto popThread = std::thread([&] {
        allowPush = true;
        SoFiData valOut{INVALID_SOFI_DATA};
        while (!stopPopThread)
        {
            valOut = INVALID_SOFI_DATA;
            if (sofi.pop(valOut))
            {
                // pop SoFi and do tests if successful
                // if we do not get an expected value, perform the test for logging and stop the threads
                if (popCounter != valOut)
                {
                    // there should be only consecutive values
                    EXPECT_THAT(popCounter, Eq(valOut));
                    stopPushThread = true;
                    stopPopThread = true;
                }
                popCounter++;
                while (isPushing)
                {
                    // busy waiting is useless, let the other thread continue it's work
                    std::this_thread::yield();
                }

                allowPush = true;
            }
            else if (valOut >= 0)
            {
                EXPECT_THAT(valOut, Lt(0)) << "SoFi told us to be empty, but returned a value!";
                stopPushThread = true;
                stopPopThread = true;
            }
            tryPopCounter++;
        }
    });

    auto pushThread = std::thread([&] {
        SoFiData valOut{INVALID_SOFI_DATA};
        while (!stopPushThread)
        {
            // we try to trigger a push into an empty SoFi, so wait until the pop thread tells us the SoFi is empty
            if (!allowPush)
            {
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                continue;
            }

            // allowPush is also set in the pop thread, so we need to block the access in the pop thread
            isPushing = true;
            valOut = INVALID_SOFI_DATA;
            auto pushResult = sofi.push(pushCounter, valOut);
            pushCounter++;
            allowPush = false;
            isPushing = false;

            // if we do not get an expected value, perform the test for logging and stop the threads
            if (pushResult == false || valOut >= 0)
            {
                EXPECT_THAT(pushResult, Eq(true)) << "Pushing is slower than popping! No overflow should occur!";
                EXPECT_THAT(valOut, Lt(0)) << "Pushing is slower than popping! No value should be returned!";
                stopPushThread = true;
                stopPopThread = true;
            }

            std::this_thread::yield(); // allow other threads to run -> slows this thread down
        }
        stopPopThread = true;
    });

    if (std::thread::hardware_concurrency() > 1)
    {
        EXPECT_TRUE(setCpuAffinity(0, pushThread.native_handle())) << "Could not run thread on specified CPU!";
        EXPECT_TRUE(setCpuAffinity(2, popThread.native_handle())) << "Could not run thread on specified CPU!";
    }

    // let the games begin ... stress empty SoFi pop while pushing
    std::this_thread::sleep_for(STRESS_TIME);

    stopPushThread = true; // stop the push thread -> this will also stop the pop thread

    pushThread.join();
    popThread.join();

    // after stopping the threads, there might still be values in the SoFi; get them out and check for validity
    SoFiData valOut{INVALID_SOFI_DATA};
    while (sofi.pop(valOut))
    {
        if (valOut != popCounter)
        {
            EXPECT_THAT(valOut, Eq(popCounter)) << "There was a data loss!";
            break;
        }
        valOut = INVALID_SOFI_DATA;
        popCounter++;
    }

    EXPECT_THAT(pushCounter / 1000, Gt(STRESS_TIME.count())) << "There should be at least 1000 pushes per millisecond!";
    EXPECT_THAT(tryPopCounter / 4, Gt(popCounter))
        << "There should be at least 4 times as many trys to pop as actual pops!";
    EXPECT_THAT(pushCounter, Eq(popCounter)) << "Push and Pop Counter should be Equal after the Test!";

    std::cout << "try pop counter: " << tryPopCounter << std::endl;
    std::cout << "pop counter    : " << pushCounter << std::endl;
}

/// @brief This tests a fast pusher and slow popper.
///
/// In this case, we have a full SoFi where continuously a push is performed, which results in continuously overflowing.
/// From time to time there is a pop.
/// The test covers the situation when there is a pop on an overflowing SoFi while there is a simultaneous push and
/// checks
/// whether pop() and empty() works like expected.
///
/// Consecutive values (starting with 0) are pushed into the SoFi, so the overflowing and popped out values should also
/// be consecutive.
///
/// push and pop thread should run with the same priority to have an equal chance to interrupt each other.
///
/// @note This test doesn't check for the correct memory ordering of the FIFO, but assumes that the used memory ordering
/// is correct and tests the algorithm in general, e.g. if a load/store is used instead of a compare_exchange
TEST_F(SoFiStress, PopFromContinuouslyOverflowingSoFi)
{
    iox::concurrent::SoFi<SoFiData, 10> sofi;

    SoFiData pushCounter{0};
    SoFiData dataCounter{0};
    SoFiData popCounter{0};
    std::atomic<SoFiData> lastPopValue{INVALID_SOFI_DATA};
    std::atomic<bool> allowPop{false};
    std::atomic<bool> isPopping{false};
    std::atomic<bool> stopPushThread{false};
    std::atomic<bool> stopPopThread{false};

    auto pushThread = std::thread([&] {
        SoFiData valOut{INVALID_SOFI_DATA};
        while (!stopPushThread)
        {
            valOut = INVALID_SOFI_DATA;
            auto pushResult = sofi.push(pushCounter, valOut);
            pushCounter++;

            // if we do not get an expected value, perform the test for logging and stop the threads
            if (pushResult == true && valOut >= 0)
            {
                EXPECT_THAT(valOut, Lt(0)) << "There was no overflow, but we still got data!";
                stopPushThread = true;
                stopPopThread = true;
            }

            if (pushResult == false && valOut < 0)
            {
                EXPECT_THAT(valOut, Gt(INVALID_SOFI_DATA)) << "There was an overflow, but we did not get data!";
                stopPushThread = true;
                stopPopThread = true;
            }

            // for the sake of completeness
            // if "pushResult == true" and "valOut < 0" -> no error, we are pushing into an non-full SoFi

            // this is what we want, an overflowing SoFi
            if (pushResult == false && valOut >= 0)
            {
                // we had our first overflow -> allow popping
                if (dataCounter == 0)
                {
                    allowPop = true;
                }

                // there was no pop in between
                if (valOut == dataCounter)
                {
                    dataCounter++;
                }
                else // there must have been a pop in between
                {
                    while (isPopping)
                    {
                        // busy waiting is useless, let the other thread continue it's work
                        std::this_thread::yield();
                    }

                    // the popped value must match our data counter, because our data counter already didn't match with
                    // the overflow value
                    if (lastPopValue != dataCounter)
                    {
                        EXPECT_THAT(lastPopValue, Eq(dataCounter)) << "There was a data loss!";
                        stopPushThread = true;
                        stopPopThread = true;
                    }
                    lastPopValue = INVALID_SOFI_DATA;
                    dataCounter++;
                    allowPop = true;

                    // there is at most only one pop, so our overflow value must now match the incremented data counter
                    if (valOut != dataCounter)
                    {
                        EXPECT_THAT(valOut, Eq(dataCounter)) << "There was a data loss!";
                        stopPushThread = true;
                        stopPopThread = true;
                    }

                    dataCounter++;
                }
            }
        }
        stopPopThread = true;
    });

    auto popThread = std::thread([&] {
        SoFiData valOut{INVALID_SOFI_DATA};
        while (!stopPopThread)
        {
            // we try to trigger a pop from an overflowing SoFi, so wait until the push thread tells us the SoFi is
            // overflowing
            if (!allowPop)
            {
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                continue;
            }

            // SoFi should never be empty
            auto emptyResult = sofi.empty();
            if (emptyResult == true)
            {
                EXPECT_THAT(emptyResult, Eq(false)) << "SoFi is continuously overflowing and shouldn't be empty!";
                stopPushThread = true;
                stopPopThread = true;
            }

            isPopping = true;
            valOut = INVALID_SOFI_DATA;
            auto popResult = sofi.pop(valOut);
            // SoFi is continuously overflowing, so the pop should always succeed
            if (popResult)
            {
                if (valOut < 0)
                {
                    EXPECT_THAT(valOut, Gt(INVALID_SOFI_DATA))
                        << "This should not happen! SoFi promised to give us data, but we didn't get data!";
                    stopPushThread = true;
                    stopPopThread = true;
                }
                popCounter++;
                lastPopValue = valOut; // save the value for the push thread, to be able to perform the data check
                allowPop = false;
            }
            else
            {
                EXPECT_THAT(popResult, Eq(true)) << "SoFi is continuously overflowing and shouldn't be empty!";
                EXPECT_THAT(valOut, Lt(0)) << "SoFi told us to be empty, but returned a value!";
                stopPushThread = true;
                stopPopThread = true;
            }
            isPopping = false;

            std::this_thread::yield(); // allow other threads to run -> slows this thread down
            std::this_thread::yield(); // allow other threads to run -> slows this thread down
        }
    });

    if (std::thread::hardware_concurrency() > 1)
    {
        EXPECT_TRUE(setCpuAffinity(0, pushThread.native_handle())) << "Could not run thread on specified CPU!";
        EXPECT_TRUE(setCpuAffinity(2, popThread.native_handle())) << "Could not run thread on specified CPU!";
    }

    // let the games begin ... stress SoFi push overflow while popping
    std::this_thread::sleep_for(STRESS_TIME);

    stopPushThread = true; // stop the push thread -> this will also stop the pop thread

    pushThread.join();
    popThread.join();

    // after stopping the threads, there might still be values in the SoFi and an unchecked popped value; get them out
    // and check for validity
    if (lastPopValue >= 0)
    {
        EXPECT_THAT(lastPopValue, Eq(dataCounter)) << "There was a data loss!";
        dataCounter++;
    }
    auto valOut{INVALID_SOFI_DATA};
    while (sofi.pop(valOut))
    {
        if (valOut != dataCounter)
        {
            EXPECT_THAT(valOut, Eq(dataCounter)) << "There was a data loss!";
            break;
        }
        valOut = INVALID_SOFI_DATA;
        dataCounter++;
    }

    EXPECT_THAT(pushCounter / 1000, Gt(STRESS_TIME.count())) << "There should be at least 1000 pushes per millisecond!";
    EXPECT_THAT(popCounter / 100, Gt(STRESS_TIME.count())) << "There should be at least 100 pops per millisecond!";
    EXPECT_THAT(pushCounter / 4, Gt(popCounter)) << "There should be at least 4 times as many pushes as pops!";
    EXPECT_THAT(pushCounter, Eq(dataCounter)) << "Push and Data Counter should be Equal after the Test!";

    std::cout << "push counter: " << pushCounter << std::endl;
    std::cout << "pop counter : " << popCounter << std::endl;
}

/// @brief This tests a fast pusher and fast popper.
///
/// The SoFi will never be empty or or full and there are continuously simultaneous pushes and pops.
/// When the SoFi is almost full, the pusher will be slowed down until the SoFi is again half empty, then the pusher
/// runs again with full speed. When the SoFi is almost empty, the popper will be slowed down until the Sofi is again
/// half full, then the popper runs again with full speed.
///
/// Consecutive values (starting with 0) are pushed into the SoFi, so the popped out values should also be consecutive.
///
/// push and pop thread should run with the same priority to have an equal chance to interrupt each other.
///
/// @note This test doesn't check for the correct memory ordering of the FIFO, but assumes that the used memory ordering
/// is correct and tests the algorithm in general, e.g. if a load/store is used instead of a compare_exchange
TEST_F(SoFiStress, PushAndPopFromNonOverflowingNonEmptySoFi)
{
    // SoFi is quite big in this test -> put it on the heap
    using SoFi_t = iox::concurrent::SoFi<SoFiData, 1000000>;
    std::unique_ptr<SoFi_t> sofi{new SoFi_t};

    std::atomic<SoFiData> pushCounter{0};
    std::atomic<SoFiData> popCounter{0};
    bool slowDownPush{false};
    bool slowDownPop{false};
    std::atomic<bool> stopPushThread{false};
    std::atomic<bool> stopPopThread{false};

    auto pushThread = std::thread([&] {
        auto localPushCounter = pushCounter.load();
        while (!stopPushThread)
        {
            // if the SoFi is almost full, slow down
            auto fillLevel = localPushCounter - popCounter.load();
            if (fillLevel > static_cast<int64_t>(sofi->capacity()) - 10)
            {
                slowDownPush = true;
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                continue;
            }

            SoFiData valOut{INVALID_SOFI_DATA};
            auto pushResult = sofi->push(localPushCounter, valOut);

            if (!pushResult)
            {
                EXPECT_THAT(pushResult, Eq(true)) << "No overflow should occur!";
            }
            else if (valOut >= 0)
            {
                EXPECT_THAT(valOut, Lt(0)) << "There was no overflow, but we still got data!";
                stopPushThread = true;
                stopPopThread = true;
            }

            ++localPushCounter;
            pushCounter = localPushCounter;

            // we are pushing to fast, slow down until the SoFi is half empty
            if (slowDownPush)
            {
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                auto fillLevel = localPushCounter - popCounter.load();
                if (fillLevel < static_cast<int64_t>(sofi->capacity()) / 2)
                {
                    slowDownPush = false;
                }
            }
        }

        stopPopThread = true;
    });

    auto popThread = std::thread([&] {
        auto localPopCounter = popCounter.load();
        while (!stopPopThread)
        {
            // if the SoFi is almost empty, slow down
            auto fillLevel = pushCounter.load() - localPopCounter;
            if (fillLevel < 10)
            {
                slowDownPop = true;
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                continue;
            }

            SoFiData valOut{INVALID_SOFI_DATA};
            auto popResult = sofi->pop(valOut);

            if (!popResult)
            {
                EXPECT_THAT(popResult, Eq(true)) << "We shouldn't have an empty SoFi!";
            }

            // there should be only consecutive values
            if (valOut != localPopCounter)
            {
                EXPECT_THAT(valOut, Eq(localPopCounter)) << "There was a data loss!";
            }
            ++localPopCounter;
            popCounter = localPopCounter;

            // we are popping too fast, slow down until the SoFi is half full
            if (slowDownPop)
            {
                std::this_thread::yield(); // allow other threads to run -> slows this thread down
                auto fillLevel = pushCounter.load() - localPopCounter;
                if (fillLevel > static_cast<int64_t>(sofi->capacity()) / 2)
                {
                    slowDownPop = false;
                }
            }
        }
    });

    if (std::thread::hardware_concurrency() > 1)
    {
        EXPECT_TRUE(setCpuAffinity(0, pushThread.native_handle())) << "Could not run thread on specified CPU!";
        EXPECT_TRUE(setCpuAffinity(2, popThread.native_handle())) << "Could not run thread on specified CPU!";
    }

    // let the games begin ... stress SoFi push and pop
    std::this_thread::sleep_for(STRESS_TIME);

    stopPushThread = true; // stop the push thread -> this will also stop the pop thread

    pushThread.join();
    popThread.join();

    // after stopping the threads, there might still be values in the SoFi; get them out and check for validity
    SoFiData valOut{INVALID_SOFI_DATA};
    while (sofi->pop(valOut))
    {
        if (valOut != popCounter)
        {
            EXPECT_THAT(valOut, Eq(popCounter.load())) << "There was a data loss!";
            break;
        }
        valOut = INVALID_SOFI_DATA;
        popCounter++;
    }

    EXPECT_THAT(pushCounter / 1000, Gt(STRESS_TIME.count())) << "There should be at least 1000 pushes per millisecond!";
    EXPECT_THAT(pushCounter, Eq(popCounter.load())) << "Push and Pop Counter should be Equal after the Test!";

    std::cout << "push & pop counter: " << pushCounter << std::endl;
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
