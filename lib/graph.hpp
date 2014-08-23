// This file contains definitions of basic types: Vertex, Distance, Arc, and Graph.
// Graph can be read from file in DIMACS or METIS format.
//
// Copyright (c) savrus, 2014

#pragma once

#include <vector>
#include <utility>
#include <algorithm>
#include <limits>
#include <cassert>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

// Types for vertices and distances to avoid typing 'int' for almost every variable
typedef int Vertex;
typedef int Distance;
static const Vertex none = -1;
static const Distance infty = std::numeric_limits<Distance>::max();

// Arc class
struct Arc {
    Vertex head:30;    // head vertex ID
    bool forward:1;    // true if this is an outgoing arc
    bool reverse:1;    // true if this is an incoming arc
    Distance length;   // arc length
    Arc(Vertex h, Distance l, bool f, bool r) : head(h), forward(f), reverse(r), length(l) {}
    Arc() {}
};


// Directed graph class
class Graph {
    long long n;                                      // Number of vertices
    long long m;                                      // Number of arcs
    std::vector<Arc> arcs;                            // Compressed adjacency lists
    std::vector< std::vector<Arc*> > a_begin;         // Index of first outgoing/incoming arc
    std::vector< std::vector<Arc*> > a_end;           // Index of last outgoing/incoming arc + 1
    std::vector< std::pair<Vertex, Arc> > tmp_arcs;   // Temporary list of arcs

    // Compare arcs by head
    struct cmp_by_head {
        bool operator() (const std::pair<Vertex,Arc> &xx, const std::pair<Vertex,Arc> &yy) {
            const Arc &x = xx.second, &y = yy.second;
            if (xx.first != yy.first) return xx.first < yy.first;
            if (x.head != y.head) return x.head < y.head;
            return x.length < y.length;
        }
    };

    // Compare arcs by direction
    struct cmp_by_direction {
        bool operator() (const std::pair<Vertex,Arc> &xx, const std::pair<Vertex,Arc> &yy) {
            const Arc &x = xx.second, &y = yy.second;
            if (xx.first != yy.first) return xx.first < yy.first;
            if (x.reverse != y.reverse) return x.reverse == true;
            if (x.forward != y.forward) return x.forward == false;
            if (x.head != y.head) return x.head < y.head;
            return x.length < y.length;
        }
    };

    // Construct adjacency lists from the temporary list of arcs
    void init_arcs() {
        std::vector< std::pair<int, Arc> > &a = tmp_arcs;
        size_t i;
        // Remove redundant arcs
        std::sort(a.begin(), a.end(), cmp_by_direction());
        i = 0;
        for (size_t j = 1; j < a.size(); ++j) {
            Arc &ai = a[i].second, &aj = a[j].second;
            if (a[i].first == a[j].first && ai.head == aj.head && ai.forward == aj.forward && ai.reverse == aj.reverse) continue;
            if (++i < j) a[i] = a[j];
        }
        a.resize(i+1);

        // Merge forward and reverse arcs into one bidirectional arc
        std::sort(a.begin(), a.end(), cmp_by_head());
        i = 0;
        for (size_t j = 1; j < a.size(); ++j) {
            Arc &ai = a[i].second, &aj = a[j].second;
            if (a[i].first == a[j].first && ai.head == aj.head && ai.length == aj.length) {
                ai.forward |= aj.forward;
                ai.reverse |= aj.reverse;
                continue;
            }
            if (++i < j) a[i] = a[j];
        }
        a.resize(i+1);

        // Order arcs by direction and determine adjacency lists
        std::sort(a.begin(), a.end(), cmp_by_direction());
        arcs.resize(a.size());
        a_begin[0].resize(n);
        a_begin[1].resize(n);
        a_end[0].resize(n);
        a_end[1].resize(n);
        for (size_t j = 0; j < a.size(); ++j) {
            arcs[j] = a[j].second;
            if (j == 0 || a[j].first != a[j-1].first) {
                if (arcs[j].reverse) a_begin[0][a[j].first] = &arcs[j];
                if (arcs[j].forward) a_begin[1][a[j].first] = &arcs[j];
            }
            if (j+1 == a.size() || a[j].first != a[j+1].first) {
                if (arcs[j].reverse) a_end[0][a[j].first] = &arcs[j] + 1;
                if (arcs[j].forward) a_end[1][a[j].first] = &arcs[j] + 1;
            }
            if (j > 0 && a[j].first == a[j-1].first) {
                if (arcs[j].reverse != arcs[j-1].reverse) a_end[0][a[j].first] = &arcs[j];
                if (arcs[j].forward != arcs[j-1].forward) a_begin[1][a[j].first] = &arcs[j];
            }
        }
        a.clear();
    }

    // Add arc to the temporary list of arcs
    bool add_tmp_arc(Vertex u, Vertex v, Distance w, bool undirected) {
        if (u < 0 || v < 0 || u >= n || v >= n) return false;
        tmp_arcs.push_back(std::make_pair(u, Arc(v, w, true, undirected)));
        tmp_arcs.push_back(std::make_pair(v, Arc(u, w, undirected, true)));
        m += 1 + undirected;
        return true;
    }

    // Read graph from file in DIMACS format
    //     p sp n m   - header: n vertices, m edges
    //     c bla-bla  - comment line
    //     a u v w    - arc (u,v) with length w
    bool read_dimacs(FILE *file, bool undirected = false) {
        char buf[512], c;
        long long u,v,w,m;
        bool inited = false;
        while (fgets(buf, sizeof(buf), file) != NULL) {
            if (buf[strlen(buf)-1] != '\n') {
                if (buf[0] != 'c') return false;
                while ((c = fgetc(file)) != '\n' && c != EOF);
            } else if (buf[0] == 'p') {
                if (inited) return false; inited = true;
                if (sscanf(buf, "p sp %lld %lld", &n, &m) != 2) return false;
            } else if (buf[0] == 'a') {
                if (sscanf(buf, "a %lld %lld %lld", &u, &v, &w) != 3) return false;
                if (!add_tmp_arc(u-1, v-1, w, undirected)) return false;
            } else if (buf[0] == 'c') continue;
            else return false;
        }
        finilize();
        return true;
    }

    // Write file in DIMACS format
    void write_dimacs(FILE *file) {
        long long m;
        for (Vertex v = 0; v < n; ++v) m += get_degree(v, true);
        fprintf(file, "p sp %lld %lld\n", n, m);
        for (Vertex v = 0; v < n; ++v) {
            for (arc_iterator a = begin(v), e = end(v); a < e; ++a) {
                long long u = v, w = a->head, l = a->length;
                fprintf(file, "a %lld %lld %lld\n", u+1, w+1, l);
            }
        }
    }

    // Read graph from file in METIS format
    //    n m [fmt] [ncon]  - header: fmt = ijk: i - s (vertex size) is present,
    //                                           j - w (vertex weights) are present,
    //                                           k - l (edge lengths) are present,
    //                                ncon - number of vertex weights. All defaults are 1.
    //    ...
    //    s w_1 .. w_ncon v_1 l_1 ...  - line for a vertex: size (if i), vertex weights (if j), edges with lengths (if k)
    bool read_metis(FILE *file, bool undirected = false) {
        int c, i = 0, fmt = 0, ncon = 0, skip = 0;
        long long elem = 0;
        Vertex v = 0, head;
        bool newline = true, inelem = false;
        do {
            c = fgetc(file);
            if (newline && c == '%') { while (c != '\n' && c != EOF) c = fgetc(file); }
            else if (isdigit(c)) { inelem = true; elem = elem*10 + c - '0'; }
            else if (isspace(c) || c == EOF) {
                if (inelem) {
                    if (v == 0) {
                        if (i == 0) n = elem;
                        else if (i == 1) /* m = elem */;
                        else if (i == 2) { 
                            if ((elem % 10 > 1) || ((elem/10) % 10 > 1) || elem > 111) return false;
                            fmt = elem;
                            skip = (fmt >= 100) + (fmt % 100 >= 10);
                        } else if (i == 3) {
                            if (fmt % 100 < 10) return false;
                            ncon = elem;
                            skip = (fmt >= 100) + ncon;
                        } else return false;
                    } else if (i >= skip) {
                        if (fmt % 10 == 1) {
                            if ((i-skip) % 2 == 0) head = elem-1;
                            else if (!add_tmp_arc(v-1, head, elem, undirected)) return false;
                        } else if (!add_tmp_arc(v-1, elem-1, 1, undirected)) return false;
                    }
                    ++i; elem = 0; inelem = false;
                }
                if (c == '\n' || (!newline && c == EOF)) {
                    if (v > 0 && (i < skip || (fmt % 10 == 1 && (i-skip) % 2 == 1))) return false;
                    ++v; i = 0; newline = true;
                }
            } else return false;
        } while (c != EOF);
        finilize();
        return true;
    }

public:
    Graph() : n(0), m(0), a_begin(2), a_end(2) {}

    Vertex get_n() const { return n; }            // Get number of vertices
    size_t get_m() const { return m; }            // Get number of arcs

    typedef Arc* arc_iterator;                    // Adjacency list iterator type

    // Get adjacency list iterator bounds
    arc_iterator begin(Vertex v, bool forward = true) const { return a_begin[forward][v]; }
    arc_iterator end(Vertex v, bool forward = true)   const { return a_end[forward][v]; }

    // Get vertex degree
    size_t get_degree(Vertex v, bool forward) const { return end(v, forward) - begin(v, forward); }
    size_t get_degree(Vertex v) const { return get_degree(v, true) + get_degree(v, false); }

    // Read graph from file (if the file format is supported)
    bool read(char *filename, bool undirected = false) {
        FILE *file;
        if ((file = fopen(filename, "r")) == NULL) return false;
        if (read_dimacs(file, undirected)) { fclose(file); return true; } rewind(file);
        if (read_metis(file, undirected)) { fclose(file); return true; } fclose(file);
        return false;
    }

    // Write graph to file
    bool write(char *filename) {
        FILE *file;
        if ((file = fopen(filename, "w")) == NULL) return false;
        write_dimacs(file);
        return ferror(file) ? (fclose(file) && false) : !fclose(file);
    }

    // Graph construction interface
    // User should set n, add some arcs, and call finilize()

    // Set the number of vertices
    void set_n(Vertex nn) { n = nn; };
    // Add (u,v) arc (with (v,u) arc if undirected) to the temporary list of arcs
    void add_arc(Vertex u, Vertex v, Distance w, bool undirected = false) { bool r = add_tmp_arc(u, v, w, undirected); assert(r); }
    // Construct adjacency lists from the temporary list of arcs
    void finilize() { init_arcs(); }
};
