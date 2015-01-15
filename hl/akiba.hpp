// Akiba et al. presented a 'pruned labeling' algorithm to build Hierarchical Hub Labels from a vertex order.
// This file contains Akiba et. al. algorithm implementation
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

#include <vector>
#include <cassert>
#include "graph.hpp"
#include "dijkstra.hpp"
#include "labeling.hpp"

namespace hl {

// Akiba et. al. 'pruned labeling' algorithm implementation
class Akiba : BasicDijkstra {

    // Add i'th vertex from the order into the labels of reachable vertices
    void iteration(size_t i, bool forward, std::vector<Vertex> &order, Labeling &labeling) {
        clear();
        Vertex v = order[i];
        distance[v] = 0;
        update(v, 0);
        while (!queue.empty()) {
            Vertex u = queue.pop();
            Distance d = distance[u];
            labeling.add(u, !forward, i, d);
            for (Graph::arc_iterator a = g->begin(u, forward), end = g->end(u, forward); a < end; ++a) {
                Distance dd = d + a->length;
                assert(dd > d && dd < infty);
                if (dd < distance[a->head] && dd < labeling.query(v, a->head, forward)) update(a->head, dd);
            }
        }
    }

public:
    Akiba(Graph &g) : BasicDijkstra(g) {}

    // Buld HHL from a vertex order
    void run(std::vector<Vertex> &order, Labeling &labeling) {
        assert(order.size() == g->get_n());
        labeling.clear();
        for (size_t i = 0; i < order.size(); ++i) {
            iteration(i, false, order, labeling);
            iteration(i, true, order, labeling);
        }
    }
};

}
