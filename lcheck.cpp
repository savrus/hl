// This file contains a program to verify the labels.
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
#include <omp.h>
#include <string.h>
#include "graph.hpp"
#include "labeling.hpp"
#include "labeling_check.hpp"

using namespace hl;

void usage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [-c] [-l labeling] [-t threads] graph" << std::endl
              << "  -c         \tCheck labals (without this option print statistics only)" << std::endl
              << "  -l labeling\tFile to write the labeling" << std::endl
              << "  -t threads \tNumber of threads" << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    char *graph_file = NULL;
    char *label_file = NULL;
    int num_threads = omp_get_max_threads();
    bool check = false;
    int argi;
    for (argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp("--", argv[argi])) { ++argi; break; }
            else if (!strcmp("-h", argv[argi])) usage(argv);
            else if (!strcmp("-c", argv[argi])) check = true;
            else if (!strcmp("-l", argv[argi])) { if (++argi >= argc) usage(argv); label_file = argv[argi]; }
            else if (!strcmp("-t", argv[argi])) { if (++argi >= argc) usage(argv); num_threads = strtoul(argv[argi], NULL, 10); }
            else usage(argv);
        } else if (graph_file == NULL) graph_file = argv[argi];
        else break;
    }
    if (argi != argc || !graph_file || !label_file) usage(argv);
    assert(num_threads > 0);
    omp_set_num_threads(num_threads);

    Graph g;
    if (!g.read(graph_file)) {
        std::cerr << "Unable to read graph from file " << graph_file << std::endl;
        std::exit(1);
    }
    std::cout << "Graph has " << g.get_n() << " vertices and " << g.get_m() << " arcs" << std::endl;

    Labeling labels(g.get_n());
    std::vector<Vertex> order;

    if (!labels.read(label_file, g.get_n())) {
        std::cerr << "Unable to read labels from file " << label_file << std::endl;
        std::exit(1);
    }

    if (check) {
        if (!LabelingCheck(g, num_threads).run(labels)) {
            std::cout << "Bad Labels" << std::endl;
            std::exit(1);
        } else std::cout << "Labels OK" << std::endl;
    }

    std::cout << "Average label size " << labels.get_avg() << std::endl;
    std::cout << "Maximum label size " << labels.get_max() << std::endl;
}

