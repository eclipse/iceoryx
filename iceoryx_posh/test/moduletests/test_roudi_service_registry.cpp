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
//
// SPDX-License-Identifier: Apache-2.0

#include "test.hpp"

#include "iceoryx_posh/internal/roudi/service_registry.hpp"
#include "iceoryx_utils/cxx/string.hpp"

using namespace ::testing;
using ::testing::Return;

class ServiceRegistry_test : public Test
{
  public:
    using ServiceDescription = iox::capro::ServiceDescription;
    virtual void SetUp()
    {
        internal::CaptureStdout();
    }

    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStdout();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
    iox::roudi::ServiceRegistry registry;
    iox::roudi::ServiceRegistry::InstanceSet_t searchResults;

    iox::cxx::string<100> AnyInstanceString{iox::capro::AnyInstanceString};
};

TEST_F(ServiceRegistry_test, SingleAdd)
{
    registry.add("a", "b");
    registry.find(searchResults, "a", AnyInstanceString);

    EXPECT_THAT(searchResults.size(), Eq(1));
    EXPECT_THAT(searchResults[0], Eq(iox::cxx::string<100>("b")));
}

TEST_F(ServiceRegistry_test, SingleMultiAdd)
{
    registry.add("a", "b");
    registry.add("a", "c");
    registry.add("a", "d");
    registry.find(searchResults, "a", AnyInstanceString);

    EXPECT_THAT(searchResults.size(), Eq(3));

    bool hasFoundB = false;
    bool hasFoundC = false;
    bool hasFoundD = false;

    for (auto& e : searchResults)
    {
        if (e == iox::cxx::string<100>("b"))
            hasFoundB = true;
        if (e == iox::cxx::string<100>("c"))
            hasFoundC = true;
        if (e == iox::cxx::string<100>("d"))
            hasFoundD = true;
    }

    EXPECT_THAT(hasFoundB && hasFoundC && hasFoundD, Eq(true));
}

TEST_F(ServiceRegistry_test, SingleAddMultiService)
{
    registry.add("a", "b");
    registry.add("c", "d");
    registry.find(searchResults, "a", AnyInstanceString);

    EXPECT_THAT(searchResults.size(), Eq(1));
    EXPECT_THAT(searchResults[0], Eq(iox::cxx::string<100>("b")));
    searchResults.clear();

    registry.find(searchResults, "c", AnyInstanceString);
    EXPECT_THAT(searchResults.size(), Eq(1));
    EXPECT_THAT(searchResults[0], Eq(iox::cxx::string<100>("d")));
}

TEST_F(ServiceRegistry_test, FindSpecificInstance)
{
    registry.add("a", "b");
    registry.add("a", "c");
    registry.add("a", "d");
    registry.find(searchResults, "a", "c");

    EXPECT_THAT(searchResults.size(), Eq(1));
    EXPECT_THAT(searchResults[0], Eq(iox::cxx::string<100>("c")));
}

TEST_F(ServiceRegistry_test, FindSpecificNonExistingInstance)
{
    registry.add("a", "b");
    registry.add("a", "c");
    registry.add("a", "d");
    registry.find(searchResults, "a", "g");

    EXPECT_THAT(searchResults.size(), Eq(0));
}

TEST_F(ServiceRegistry_test, RemoveSingle)
{
    registry.add("a", "b");
    registry.add("a", "c");
    registry.add("a", "d");

    registry.remove("a", "c");

    registry.find(searchResults, "a", "c");
    EXPECT_THAT(searchResults.size(), Eq(0));
}

TEST_F(ServiceRegistry_test, RemoveSingleFromMultipleServices)
{
    registry.add("a", "b");
    registry.add("b", "c");
    registry.add("c", "d");

    registry.remove("b", "c");

    registry.find(searchResults, "b", "c");
    EXPECT_THAT(searchResults.size(), Eq(0));
}

TEST_F(ServiceRegistry_test, RemoveAll)
{
    registry.add("a", "b");
    registry.add("a", "c");
    registry.add("a", "d");

    registry.remove("a", "b");
    registry.remove("a", "c");
    registry.remove("a", "d");

    registry.find(searchResults, "a", AnyInstanceString);
    EXPECT_THAT(searchResults.size(), Eq(0));
}

TEST_F(ServiceRegistry_test, GetServiceMap)
{
    iox::roudi::ServiceRegistry::serviceMap_t serviceMap;

    registry.add("a", "b");
    // add same service a, instance c to check if in registry only one entry is created
    registry.add("a", "c");
    registry.add("a", "c");
    registry.add("a", "d");
    registry.add("e", "f");

    serviceMap = registry.getServiceMap();

    bool mapA = false;
    bool mapE = false;

    for (auto const& x : serviceMap)
    {
        if (x.first == iox::cxx::string<100>("a"))
        {
            ASSERT_THAT(x.second.instanceSet.size(), Eq(3));
            mapA = true;
            EXPECT_THAT(x.second.instanceSet[0], Eq(iox::cxx::string<100>("b")));
            EXPECT_THAT(x.second.instanceSet[1], Eq(iox::cxx::string<100>("c")));
            EXPECT_THAT(x.second.instanceSet[2], Eq(iox::cxx::string<100>("d")));
        }

        if (x.first == iox::cxx::string<100>("e"))
            mapE = true;
    }

    EXPECT_THAT(mapA && mapE, Eq(true));
}
