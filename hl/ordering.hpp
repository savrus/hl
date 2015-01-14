// Vertex order is a list of vertices where vertices are sorted from the most important to the least important.
// This file contains methods to write a vertex order to a file and read it from a file.
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
#include <fstream>
#include <istream>
#include <iostream>
#include "graph.hpp"

namespace hl {

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

} // namespace hl

