// Vertex order is a list of vertices where vertices are sorted from the most important to the least important.
// This file contains methods to write a vertex order to a file and read it from a file.
//
// Copyright (c) savrus, 2014

#pragma once

#include <vector>
#include <fstream>
#include <istream>
#include <iostream>
#include "graph.hpp"

// Class to read/write order
struct Order {
    // Write vertex order to file
    static bool write(char *filename, std::vector<Vertex> &order) {
        std::ofstream file;
        file.open(filename);
        file << order.size() << std::endl;
        for (size_t i = 0; i < order.size(); ++i) file << order[i] << std::endl;
        file.close();
        return file.good();
    }

    // Read vertex order from file
    static bool read(char *filename, std::vector<Vertex> &order) {
        std::ifstream file;
        file.open(filename);
        size_t s = 0;
        file >> s;
        order.resize(s);
        for (size_t i = 0; i < s; ++i) file >> order[i];
        file >> std::ws;
        file.close();
        return file.eof() && !file.fail();
    }
};
