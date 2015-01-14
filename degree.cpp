// This file contains a program to order vertices by their degree.
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

#include <vector>
#include <algorithm>
#include <utility>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include "graph.hpp"
#include "ordering.hpp"

using namespace hl;

void usage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " -o ordering graph" << std::endl
              << "  -o ordering\tFile with the vertex order" << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    char *graph_file = NULL;
    char *order_file = NULL;
    int argi;
    for (argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp("--", argv[argi])) { ++argi; break; }
            else if (!strcmp("-h", argv[argi])) usage(argv);
            else if (!strcmp("-o", argv[argi])) { if (++argi >= argc) usage(argv); order_file = argv[argi]; }
            else usage(argv);
        } else if (graph_file == NULL) graph_file = argv[argi];
        else break;
    }
    if (argi != argc || !graph_file || !order_file) usage(argv);

    Graph g;
    if (!g.read(graph_file)) {
        std::cerr << "Unable to read graph from file " << graph_file << std::endl;
        std::exit(1);
    }
    std::cout << "Graph has " << g.get_n() << " vertices and " << g.get_m() << " arcs" << std::endl;

    std::vector<Vertex> order(g.get_n());
    std::vector< std::pair<int, Vertex> > d(g.get_n());

    for (Vertex v = 0; v < g.get_n(); ++v) d[v] = std::make_pair(g.get_degree(v),v);
    std::sort(d.begin(),d.end());
    for (size_t i = 0; i < d.size(); ++i) order[i] = d[d.size() - i - 1].second;

    if (order_file && (!Order::write(order_file, order))) std::cerr << "Unable to write order to file " << order_file << std::endl;
}

