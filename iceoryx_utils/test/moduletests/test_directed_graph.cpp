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

#include "iceoryx_utils/internal/graphs/directed_acyclic_graph.hpp"
#include "iceoryx_utils/internal/graphs/directed_graph.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;

constexpr size_t MAX_VERTICES = 4;
constexpr size_t MAX_DEGREE = 2;

struct TestNode
{
    TestNode(uint32_t f_data, uint32_t f_index)
        : m_data(f_data)
        , m_index(f_index)
    {
    }
    uint32_t m_data;
    uint32_t m_index;

    bool operator==(const TestNode& other) const
    {
        return (m_data == other.m_data && m_index == other.m_index);
    }
};

class DirectedGraph_test : public Test
{
  public:
    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }

    DirectedGraph<TestNode, MAX_VERTICES, MAX_DEGREE> m_graph;

    TestNode node1{0, 0};
    TestNode node2{2, 1};
    TestNode node3{100, 0};
    TestNode node4{13, 42};
    TestNode node5{10000, 88};
};

TEST_F(DirectedGraph_test, addVertices)
{
    size_t numNodes = 0;

    EXPECT_EQ(m_graph.numberOfVertices(), numNodes);

    ++numNodes;
    EXPECT_TRUE(m_graph.addVertex(&node1));
    EXPECT_EQ(m_graph.numberOfVertices(), numNodes);

    ++numNodes;
    EXPECT_TRUE(m_graph.addVertex(&node2));
    EXPECT_EQ(m_graph.numberOfVertices(), numNodes);

    ++numNodes;
    EXPECT_TRUE(m_graph.addVertex(&node3));
    EXPECT_EQ(m_graph.numberOfVertices(), numNodes);

    ++numNodes;
    EXPECT_TRUE(m_graph.addVertex(&node4));
    EXPECT_EQ(m_graph.numberOfVertices(), MAX_VERTICES);

    ++numNodes;
    EXPECT_FALSE(m_graph.addVertex(&node5));
    EXPECT_EQ(m_graph.numberOfVertices(), MAX_VERTICES);
}

TEST_F(DirectedGraph_test, addEdges)
{
    m_graph.addVertex(&node1);
    m_graph.addVertex(&node2);
    m_graph.addVertex(&node3);
    m_graph.addVertex(&node4);
    m_graph.addVertex(&node5);

    size_t numEdges = 0;
    EXPECT_EQ(m_graph.numberOfEdges(), numEdges);

    EXPECT_TRUE(m_graph.addEdge(&node1, &node2));
    EXPECT_EQ(m_graph.numberOfEdges(), ++numEdges);

    EXPECT_TRUE(m_graph.addEdge(&node1, &node3));
    EXPECT_EQ(m_graph.numberOfEdges(), ++numEdges);

    EXPECT_FALSE(m_graph.addEdge(&node1, &node4));
    EXPECT_EQ(m_graph.numberOfEdges(), MAX_DEGREE);

    auto successors = m_graph.getSuccessors(&node1);

    ASSERT_EQ(successors->size(), 2);

    bool node2Found = false;
    bool node3Found = false;
    bool node4Found = false;

    for (auto node : *successors)
    {
        if (*node == node2)
        {
            node2Found = true;
        }
        if (*node == node3)
        {
            node3Found = true;
        }
        if (*node == node4)
        {
            node4Found = true;
        }
    }

    EXPECT_TRUE(node2Found);
    EXPECT_TRUE(node3Found);
    EXPECT_FALSE(node4Found);

    auto predecessors = m_graph.getPredecessors(&node2);
    ASSERT_EQ(predecessors->size(), 1);
    EXPECT_EQ((*predecessors)[0], &node1);
}

TEST_F(DirectedGraph_test, addExistingNode)
{
    m_graph.addVertex(&node1);
    EXPECT_FALSE(m_graph.addVertex(&node1));
    EXPECT_EQ(m_graph.numberOfVertices(), size_t(1));
}

TEST_F(DirectedGraph_test, addEdgeToSelf)
{
    m_graph.addVertex(&node1);
    m_graph.addVertex(&node2);

    EXPECT_FALSE(m_graph.addEdge(&node1, &node1));
    EXPECT_EQ(m_graph.numberOfEdges(), size_t(0));
}

TEST_F(DirectedGraph_test, sources)
{
    EXPECT_FALSE(m_graph.isSource(&node1));
    m_graph.addVertex(&node1);
    EXPECT_TRUE(m_graph.isSource(&node1));

    m_graph.addVertex(&node2);
    m_graph.addEdge(&node1, &node2);
    EXPECT_TRUE(m_graph.isSource(&node1));
    EXPECT_FALSE(m_graph.isSource(&node2));

    m_graph.addVertex(&node3);
    m_graph.addEdge(&node3, &node1);
    EXPECT_FALSE(m_graph.isSource(&node1));
    EXPECT_TRUE(m_graph.isSource(&node3));

    m_graph.addVertex(&node4);
    m_graph.addEdge(&node4, &node1);
    EXPECT_TRUE(m_graph.isSource(&node4));
    EXPECT_TRUE(m_graph.isSource(&node3));

    auto sources = m_graph.getSources();
    EXPECT_EQ(sources.size(), 2);
    bool node3Found = false;
    bool node4Found = false;

    for (const auto& node : sources)
    {
        if (*node == node3)
        {
            node3Found = true;
        }
        if (*node == node4)
        {
            node4Found = true;
        }
    }

    EXPECT_TRUE(node3Found);
    EXPECT_TRUE(node4Found);
}

TEST_F(DirectedGraph_test, sinks)
{
    EXPECT_FALSE(m_graph.isSink(&node1));
    m_graph.addVertex(&node1);
    EXPECT_TRUE(m_graph.isSink(&node1));

    m_graph.addVertex(&node2);
    m_graph.addEdge(&node1, &node2);
    EXPECT_FALSE(m_graph.isSink(&node1));
    EXPECT_TRUE(m_graph.isSink(&node2));

    m_graph.addVertex(&node3);
    m_graph.addEdge(&node2, &node3);
    EXPECT_FALSE(m_graph.isSink(&node2));
    EXPECT_TRUE(m_graph.isSink(&node3));

    m_graph.addVertex(&node4);
    m_graph.addEdge(&node2, &node4);
    EXPECT_TRUE(m_graph.isSink(&node3));
    EXPECT_TRUE(m_graph.isSink(&node4));

    auto sinks = m_graph.getSinks();
    EXPECT_EQ(sinks.size(), 2);
    bool node3Found = false;
    bool node4Found = false;

    for (const auto& node : sinks)
    {
        if (*node == node3)
        {
            node3Found = true;
        }
        if (*node == node4)
        {
            node4Found = true;
        }
    }

    EXPECT_TRUE(node3Found);
    EXPECT_TRUE(node4Found);
}

/// optional: typed test for all methods of the  derived class except addEdge
class DirectedAcyclicGraph_test : public Test
{
  public:
    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }

    static constexpr size_t maxVertices = 5;
    static constexpr size_t maxDegree = maxVertices - 1;
    DirectedAcyclicGraph<TestNode, maxVertices, maxDegree> m_graph;

    TestNode node1{0, 0};
    TestNode node2{2, 1};
    TestNode node3{100, 0};
    TestNode node4{13, 42};
    TestNode node5{10000, 88};
};

TEST_F(DirectedAcyclicGraph_test, addEdges)
{
    m_graph.addVertex(&node1);
    m_graph.addVertex(&node2);
    m_graph.addVertex(&node3);
    m_graph.addVertex(&node4);
    m_graph.addVertex(&node5);

    EXPECT_FALSE(m_graph.addEdge(&node1, &node1)); // selfloop
    EXPECT_TRUE(m_graph.addEdge(&node1, &node2));
    EXPECT_FALSE(m_graph.addEdge(&node2, &node1)); // cycle
    EXPECT_TRUE(m_graph.addEdge(&node2, &node3));
    EXPECT_FALSE(m_graph.addEdge(&node3, &node1)); // cycle
    EXPECT_TRUE(m_graph.addEdge(&node1, &node3));  // ok, not a directed cycle
    EXPECT_TRUE(m_graph.addEdge(&node2, &node4));
    EXPECT_TRUE(m_graph.addEdge(&node2, &node5));
    EXPECT_FALSE(m_graph.addEdge(&node5, &node1)); // cycle
    EXPECT_FALSE(m_graph.addEdge(&node5, &node2)); // cycle
    EXPECT_TRUE(m_graph.addEdge(&node5, &node3));
    EXPECT_TRUE(m_graph.addEdge(&node4, &node3));
    EXPECT_FALSE(m_graph.addEdge(&node5, &node5)); // selfloop
    EXPECT_FALSE(m_graph.addEdge(&node3, &node2)); // cycle
}
} // namespace
