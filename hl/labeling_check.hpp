// Since Hub Labels is a complex data structure one would like to verify that it is bult correctly.
// This file contains a checker for labels.
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
#include "labeling.hpp"
#include "dijkstra.hpp"
#include <omp.h>

namespace hl {

// Labeling checker implementation
class LabelingCheck {
    Graph &g;
    std::vector<Dijkstra> dijkstra;
    int num_threads;

public:
    LabelingCheck(Graph &g, int num_threads) : g(g), num_threads(num_threads) {
        for (int i = 0; i < num_threads; ++i) dijkstra.push_back(Dijkstra(g));
    }

    // Check if distance reported by labels is the real one
    bool run(Labeling &labeling) {
        bool res = true;
        #pragma omp parallel for schedule(dynamic)
        for (Vertex v = 0; v < g.get_n(); ++v) {
            Dijkstra &dij = dijkstra[omp_get_thread_num()];
            for (int side = 0; side < 2; ++side) {
                dij.run(v,side);
                for (Vertex u = 0; u < g.get_n(); ++u) {
                    if (dij.get_distance(u) != labeling.query(v,u,side)) res = false;
                }
            }
        }
        return res;
    }
};

}
