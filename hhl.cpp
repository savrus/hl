// Hierarchical Hub Labeling is a data structure used to build a distance oracle in graph.
// This file contains a program to consrtuct HHL using greedy HHL algorithm
//
// Copyright (c) savrus, 2014

#include <vector>
#include <iostream>
#include <cstdlib>
#include <omp.h>
#include <string.h>
#include "graph.hpp"
#include "hhl.hpp"
#include "uhhl.hpp"
#include "labeling.hpp"
#include "ordering.hpp"

void usage(char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [-w] [-l labeling] [-o ordering] [-t threads] graph" << std::endl
              << "  -w         \tUse label-greedy algorithm instead of path-greedy" << std::endl
              << "  -u         \tAssume that shortest paths are unique" << std::endl
              << "  -o ordering\tFile to write the vertex order" << std::endl
              << "  -l labeling\tFile to write the labeling" << std::endl
              << "  -t threads \tNumber of threads" << std::endl
              << "WARNING: performance may reduce dramatically when HyperThreading is active. Please bound the number of threads by real cores." << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    char *graph_file = NULL;
    char *order_file = NULL;
    char *label_file = NULL;
    int num_threads = omp_get_max_threads();
    int type = 0;
    bool is_usp = false;
    int argi;
    for (argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp("--", argv[argi])) { ++argi; break; }
            else if (!strcmp("-h", argv[argi])) usage(argv);
            else if (!strcmp("-w", argv[argi])) type = 1;
            else if (!strcmp("-u", argv[argi])) is_usp = true;
            else if (!strcmp("-l", argv[argi])) { if (++argi >= argc) usage(argv); label_file = argv[argi]; }
            else if (!strcmp("-o", argv[argi])) { if (++argi >= argc) usage(argv); order_file = argv[argi]; }
            else if (!strcmp("-t", argv[argi])) { if (++argi >= argc) usage(argv); num_threads = strtoul(argv[argi], NULL, 10); }
            else usage(argv);
        } else if (graph_file == NULL) graph_file = argv[argi];
        else break;
    }
    if (argi != argc || !graph_file) usage(argv);
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

    if (is_usp) UHHL(g, num_threads).run(type, order, labels);
    else         HHL(g, num_threads).run(type, order, labels);

    std::cout << "Average label size " << labels.get_avg() << std::endl;
    std::cout << "Maximum label size " << labels.get_max() << std::endl;

    if (label_file && (!labels.write(label_file))) std::cerr << "Unable to write labels to file " << label_file << std::endl;
    if (order_file && (!Order::write(order_file, order))) std::cerr << "Unable to write order to file " << order_file << std::endl;
}
