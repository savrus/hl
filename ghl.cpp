// Hub Labeling is a data structure used to build a distance oracle in graph.
// This file contains a program to consrtuct approximately optimal HL using GHLp algorithm
//
// Copyright (c) savrus, 2014

#include <vector>
#include <iostream>
#include <cstdlib>
#include <omp.h>
#include <string.h>
#include "graph.hpp"
#include "ghl.hpp"
#include "labeling.hpp"

using namespace hl;

void usage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [-p norm] [-a alpha] [-l labeling] [-t threads] graph" << std::endl
              << "  -p norm    \tApproximate p-norm of labels. Use '-p max' to approximate maximum label size" << std::endl
              << "  -a alpha   \tAlpha parameter (>=1.0) to GHLp algorithm which sets tradeoff between speed and labeling size" << std::endl
              << "  -l labeling\tFile to write the labeling" << std::endl
              << "  -t threads \tNumber of threads" << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    char *graph_file = NULL;
    char *label_file = NULL;
    int num_threads = omp_get_max_threads();
    double alpha = 1.1;
    double p = 1.0;
    bool linf = false;
    int argi;
    for (argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp("--", argv[argi])) { ++argi; break; }
            else if (!strcmp("-h", argv[argi])) usage(argv);
            else if (!strcmp("-p", argv[argi])) { if (++argi >= argc) usage(argv); if (!strcmp("max", argv[argi])) linf = true; else p = strtod(argv[argi], NULL); }
            else if (!strcmp("-a", argv[argi])) { if (++argi >= argc) usage(argv); alpha = strtod(argv[argi], NULL); }
            else if (!strcmp("-l", argv[argi])) { if (++argi >= argc) usage(argv); label_file = argv[argi]; }
            else if (!strcmp("-t", argv[argi])) { if (++argi >= argc) usage(argv); num_threads = strtoul(argv[argi], NULL, 10); }
            else usage(argv);
        } else if (graph_file == NULL) graph_file = argv[argi];
        else break;
    }
    if (argi != argc || !graph_file || alpha < 1.0) usage(argv);
    assert(num_threads > 0);
    omp_set_num_threads(num_threads);

    Graph g;
    if (!g.read(graph_file)) {
        std::cerr << "Unable to read graph from file " << graph_file << std::endl;
        std::exit(1);
    }
    std::cout << "Graph has " << g.get_n() << " vertices and " << g.get_m() << " arcs" << std::endl;

    Labeling labels(g.get_n());
    if (linf) p = log(static_cast<double>(g.get_n()));

    GHL(g, num_threads).run(labels, alpha, p);

    std::cout << "Average label size " << labels.get_avg() << std::endl;
    std::cout << "Maximum label size " << labels.get_max() << std::endl;

    if (label_file && (!labels.write(label_file))) std::cerr << "Unable to write labels to file " << label_file << std::endl;
}
