// Dijkstra algorithm is well-known algorithm for single-source shortes path problem.
// This file contains Dijkstra algorithm implementation.
//
// Copyright (c) 2014, 2015 savrus
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "graph.hpp"
#include "kheap.hpp"
#include <vector>
#include <cassert>

namespace hl {

// Data structures to implement Dijkstra-like algorithms
class BasicDijkstra {
protected:
    Graph *g;                             // graph
    KHeap<Vertex, Distance> queue;        // Dijkstra queue
    std::vector<Vertex> parent;           // parent[v] is v's parent in the shortest path tree
    std::vector<Distance> distance;       // distance[v] is distance from source to v
    std::vector<bool> is_dirty;           // is_dirty[v] is true if we touched v in current run
    std::vector<Vertex> dirty;            // list of visited vertices

    // Update vertex v: set distance to d and parent to p
    void update(Vertex v, Distance d, Vertex p = none) {
        distance[v] = d;
        parent[v] = p;
        queue.update(v, d);
        if (!is_dirty[v]) { dirty.push_back(v); is_dirty[v] = true; }
    }

    // Clear internal structures
    void clear() {
        queue.clear();
        for(size_t i = 0; i < dirty.size(); ++i) {
            parent[dirty[i]] = none;
            distance[dirty[i]] = infty;
            is_dirty[dirty[i]] = false;
        }
        dirty.clear();
        dirty.reserve(g->get_n());
    }

    BasicDijkstra(Graph &g) :
        g(&g),
        queue(g.get_n()),
        parent(g.get_n(), none),
        distance(g.get_n(), infty),
        is_dirty(g.get_n()) {}
};

// Dijkstra algorithm implementation
class Dijkstra : BasicDijkstra{
public:
    Dijkstra(Graph &g) : BasicDijkstra(g) {}

    Distance get_distance(Vertex v) { return distance[v]; }  // Distance from source to v
    Vertex get_parent(Vertex v) { return parent[v]; }        // v's parent in the shortest path tree

    // Find distances from v to all other vertices and build shortest path tree
    void run(Vertex v, bool forward = true) {
        clear();
        update(v, 0);
        while (!queue.empty()) {
            Vertex u = queue.pop();
            Distance d = distance[u];
            for (Graph::arc_iterator a = g->begin(u, forward), end = g->end(u, forward); a < end; ++a) {
                Distance dd = d + a->length;
                assert(dd > d && dd < infty);
                if (dd < distance[a->head]) update(a->head, dd, u);
            }
        }
    }
};

}
