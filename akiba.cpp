// Hierarchical Hub Labeling is a data structure used to build a distance oracle in graph.
// This file contains a program to consrtuct HHL given a specific vertex order.
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
#include <iostream>
#include <cstdlib>
#include <string.h>
#include "graph.hpp"
#include "akiba.hpp"
#include "labeling.hpp"
#include "ordering.hpp"

using namespace hl;

void usage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [-l labeling] -o ordering graph" << std::endl
              << "  -o ordering\tFile with the vertex order" << std::endl
              << "  -l labeling\tFile to write the labeling" << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    char *graph_file = NULL;
    char *order_file = NULL;
    char *label_file = NULL;
    int argi;
    for (argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp("--", argv[argi])) { ++argi; break; }
            else if (!strcmp("-h", argv[argi])) usage(argv);
            else if (!strcmp("-l", argv[argi])) { if (++argi >= argc) usage(argv); label_file = argv[argi]; }
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

    Labeling labels(g.get_n());
    std::vector<Vertex> order;

    if (!Order::read(order_file, order)) {
        std::cerr << "Unable to read vertex order from file " << order_file << std::endl;
        std::exit(1);
    }
    if (order.size() != g.get_n()) {
        std::cerr << "Order is incompatible with graph." << std::endl;
        std::exit(1);
    }

    Akiba(g).run(order, labels);

    std::cout << "Average label size " << labels.get_avg() << std::endl;
    std::cout << "Maximum label size " << labels.get_max() << std::endl;

    if (label_file && (!labels.write(label_file))) std::cerr << "Unable to write labels to file " << label_file << std::endl;
}

