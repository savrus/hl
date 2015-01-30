// Hub Labels are lists of hubs and distances to them attached to every vertex in a graph.
// This file contains the class to store labels.
// Available methods include making a query, write labels to file, read labels from file.
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
#include <vector>
#include <limits>
#include <cassert>
#include <algorithm>
#include <utility>
#include <fstream>
#include <istream>

namespace hl {

// Class to store labels
class Labeling {
    std::vector< std::vector< std::vector<Vertex> > > label_v;     // Lists of forward/reverse hubs
    std::vector< std::vector< std::vector<Distance> > > label_d;   // Lists of distances to hubs
    Vertex n;

public:
    Labeling(size_t n = 0) :
        label_v(n, std::vector< std::vector<Vertex> >(2)),
        label_d(n, std::vector< std::vector<Distance> >(2)),
        n(n) {}

    // Find u-v distance
    Distance query(Vertex u, Vertex v, bool f = true) {
        Distance r = infty;
        for (size_t i=0, j=0; i < label_v[u][f].size() && j < label_v[v][!f].size();) {
            if (label_v[u][f][i] == label_v[v][!f][j]) {
                assert(label_d[u][f][i] < infty - label_d[v][!f][j]);
                r = std::min(r, label_d[u][f][i++] + label_d[v][!f][j++]);
            } else if (label_v[u][f][i] < label_v[v][!f][j]) ++i;
            else ++j;
        }
        return r;
    }

    // Add hub (v,d) to forward or reverse label of u
    void add(Vertex u, bool forward, Vertex v, Distance d) {
        label_v[u][forward].push_back(v);
        label_d[u][forward].push_back(d);
    }

    // Get labels
    std::vector< std::vector<Vertex> > &get_label_hubs(Vertex u) { return label_v[u]; }
    std::vector< std::vector<Distance> > &get_label_distances(Vertex u) { return label_d[u]; }

    // Get maximum label size
    size_t get_max() const {
        size_t max = 0;
        for (Vertex v = 0; v < n; ++v)
            for (int side = 0; side < 2; ++side)
                max = std::max(max, label_v[v][side].size());
        return max;
    }

    // Get average label size
    double get_avg() const {
        long long total = 0;
        for (Vertex v = 0; v < n; ++v)
            total += label_v[v][0].size() + label_v[v][1].size();
        return static_cast<double>(total)/n/2;
    }

    // Write labels to file
    bool write(char *filename) {
        std::ofstream file;
        file.open(filename);
        file << n << std::endl;
        for (Vertex v = 0; v < n; ++v) {
            for (int side = 0; side < 2; ++side) {
                file << label_v[v][side].size();
                for (size_t i = 0; i < label_v[v][side].size(); ++i) {
                    file << " " << label_v[v][side][i];
                    file << " " << label_d[v][side][i];
                }
                file << std::endl;
            }
        }
        file.close();
        return file.good();
    }

    // Read labels from file
    bool read(char *filename, Vertex check_n = 0) {
        std::ifstream file;
        file.open(filename);
        file >> n;
        if (check_n && n != check_n) return false;
        label_v.resize(n, std::vector< std::vector<Vertex> >(2));
        label_d.resize(n, std::vector< std::vector<Distance> >(2));
        for (Vertex v = 0; v < n; ++v) {
            for (int side = 0; side < 2; ++side) {
                size_t s;
                file >> s;
                label_v[v][side].resize(s);
                label_d[v][side].resize(s);
                for (size_t i = 0; i < s; ++i) {
                    file >> label_v[v][side][i];
                    file >> label_d[v][side][i];
                }
            }
        }
        file >> std::ws;
        file.close();
        return file.eof() && !file.fail();
    }

    // Clear labels
    void clear() {
        for (Vertex v = 0; v < n; ++v) {
            for (int side = 0; side < 2; ++side) {
                label_v[v][side].clear();
                label_d[v][side].clear();
            }
        }
    }

    // Sort labels before making queries
    void sort() {
        for (Vertex v = 0; v < n; ++v) {
            for (int side = 0; side < 2; ++side) {
                std::vector< std::pair<Vertex,Distance> > label(label_v[v][side].size());
                for (size_t i = 0; i < label_v[v][side].size(); ++i)
                    label[i] = std::make_pair(label_v[v][side][i], label_d[v][side][i]);
                std::sort(label.begin(),label.end());
                for (size_t i = 0; i < label_v[v][side].size(); ++i) {
                    label_v[v][side][i] = label[i].first;
                    label_d[v][side][i] = label[i].second;
                }
            }
        }
    }

    #if 0
    // Print labels
    void print() const {
        for (Vertex v = 0; v < n; ++v) {
            for (int side = 0; side < 2; ++side) {
                std::cout << "L(" << v << "," << side << ") =";
                for (size_t i = 0; i < label_v[v][side].size(); ++i) std::cout << " (" << label_v[v][side][i] << "," << label_d[v][side][i] << ")";
                std::cout << std::endl;
            }
        }
    }
    #endif

};

}
