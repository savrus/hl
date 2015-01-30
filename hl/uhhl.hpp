// Hierarchical Hub Labels can be found by greedy algorithms.
// When shortest paths are unique (there is at most one shortest path between any two vertices)
// greedy algorithms can be implemented more efficiently.
// This file contains implementation of path-greedy and level-greedy HHL algorithms for USP systems.
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
#include "dijkstra.hpp"
#include "kheap.hpp"
#include "labeling.hpp"
#include <cassert>
#include <omp.h>

namespace hl {

// Hierarchical Hub Labeling implementation for unique shortest paths (USP)
class UHHL {
    // Class to store shortest paths trees (SPT)
    class SP {
        // Class for Dijkstra algorithm with USP emulation
        class USPDijkstra : BasicDijkstra {
            std::vector<int> hops;          // hops[v] is the number of hops from source to v

            // Set new distance, parent and hops for v
            void uspd_update(Vertex v, Distance dd, int h, Vertex u = none) {
                hops[v] = h;
                update(v, dd, u);
            }

            // Clear internal structures
            void uspd_clear() {
                for (size_t i = 0; i < dirty.size(); ++i) hops[dirty[i]] = 0;
                clear();
            }
        public:
            USPDijkstra(Graph &g) : BasicDijkstra(g), hops(g.get_n()) {}

            Distance get_distance(Vertex v) { return distance[v]; }  // Distance from source to v
            Vertex get_parent(Vertex v) { return parent[v]; }        // v's parent in the shortest path tree

            // Find distances from v to all other vertices and build shortest path tree
            // Choose parent with the smallest id to break ties
            void run(Vertex v, bool forward = true) {
                uspd_clear();
                uspd_update(v, 0, 0);
                while (!queue.empty()) {
                    Vertex u = queue.pop();
                    Distance d = distance[u];
                    for (Graph::arc_iterator a = g->begin(u, forward), end = g->end(u, forward); a < end; ++a) {
                        Distance dd = d + a->length;
                        assert(dd > d && dd < infty);
                        if (dd < distance[a->head]
                            || (dd == distance[a->head] && hops[u] + 1 < hops[a->head])
                            || (dd == distance[a->head] && hops[u] + 1 == hops[a->head] && u < parent[a->head]) )
                            uspd_update(a->head, dd, hops[u] + 1, u);
                    }
                }
            }
        };

        Graph &g;                                    // Graph
        Vertex n;                                    // number of vertices
        std::vector< std::vector<Distance> > dist;   // Distance table: dist[u][v] = distance(u,v)
        std::vector< std::vector<int> > cover;       // cover[u][v] is true if (u,v) pair is covered (int for thread safety)
        std::vector< std::vector<bool> > visited_pt; // mark visited vertices during graph traversal (one array per thread)
        std::vector< std::vector< std::vector<Vertex> > > parent;   // Parent table: parent[forward][u][v] = v's parent in u's SPT

        // Check if v is on u--w path under condition that there is an arc (v,w)
        bool is_path(Vertex u, Vertex v, Vertex w, bool forward) { return v == get_parent(u, w, forward); }

    public:
        SP(Graph &g, int num_threads) :
            g(g),
            n(g.get_n()),
            dist(n, std::vector<Distance>(n, infty)),
            cover(n, std::vector<int>(n)),
            visited_pt(num_threads, std::vector<bool>(n)),
            parent(2, std::vector< std::vector<Vertex> >(n, std::vector<Vertex>(n, none)))
        {
            // Build distance table and forward SPT
            std::vector<USPDijkstra> dijkstra;
            for (int i = 0; i < num_threads; ++i) dijkstra.push_back(USPDijkstra(g));
            #pragma omp parallel for
            for (Vertex u = 0; u < n; ++u) {
                USPDijkstra &dij = dijkstra[omp_get_thread_num()];
                dij.run(u);
                for (Vertex v = 0; v < n; ++v) {
                    dist[u][v] = dij.get_distance(v);
                    parent[true][u][v] = dij.get_parent(v);
                }
            }
            // Build reverse SPT from forward SPT to support non-USP graphs
            #pragma omp parallel
            {
                std::vector<Vertex> d;
                #pragma omp for
                for (Vertex u = 0; u < n; ++u) {
                    for(Graph::arc_iterator a = g.begin(u,true), end = g.end(u,true); a < end; ++a) {
                        if (parent[true][u][a->head] != u) continue;
                        get_descendants(u, a->head, d);
                        for (size_t i = 0; i < d.size(); ++i) parent[false][d[i]][u] = a->head;
                    }
                }
            }
        }

        // Set (u,v) pair covered
        void set_cover(Vertex u, Vertex v) { cover[u][v] = true; }
        // Check if pair (u,v) is covered
        bool get_cover(Vertex u, Vertex v, bool forward = true) const { return forward ? cover[u][v] : cover[v][u]; }
        // Get distance(u,v)
        Distance get_distance(Vertex u, Vertex v, bool forward = true) { return forward ? dist[u][v] : dist[v][u]; }
        // Get v's parent in u's 'forward' SPT
        Vertex get_parent(Vertex u, Vertex v, bool forward = true) { return parent[forward][u][v]; }

        // Get v's descendants in u's SPT
        void get_descendants(Vertex u, Vertex v, std::vector<Vertex> &d, bool forward = true) {
            std::vector<bool> &visited = visited_pt[omp_get_thread_num()];
            d.clear();
            if (get_cover(u,v,forward) || (u != v && get_parent(u,v,forward) == none)) return;
            d.push_back(v);
            visited[v] = true;
            for (size_t i = 0; i < d.size(); ++i) {
                for (Graph::arc_iterator a = g.begin(d[i], forward), end = g.end(d[i], forward); a < end; ++a) {
                    if (!visited[a->head] && !get_cover(u,a->head,forward) && is_path(u,d[i],a->head,forward)) {
                        d.push_back(a->head);
                        visited[a->head] = true;
                    }
                }
            }
            for (size_t i = 0; i < d.size(); ++i) visited[d[i]] = false;
        }

        // Get v's ascendants in u's SPT
        void get_ascendants(Vertex u, Vertex v, std::vector<Vertex> &d, bool forward = true) {
            std::vector<bool> &visited = visited_pt[omp_get_thread_num()];
            d.clear();
            if (get_cover(u,v,forward) || (u != v && get_parent(u,v,forward) == none)) return;
            d.push_back(v);
            visited[v] = true;
            for (size_t i = 0; i < d.size(); ++i) {
                for (Graph::arc_iterator a = g.begin(d[i], !forward), end = g.end(d[i], !forward); a < end; ++a) {
                    if (!visited[a->head] && is_path(u,a->head,d[i],forward)) {
                        d.push_back(a->head);
                        visited[a->head] = true;
                    }
                }
            }
            for (size_t i = 0; i < d.size(); ++i) visited[d[i]] = false;
        }

        // Make all (u,v) pairs uncovered
        void clear() {
            for (Vertex u = 0; u < n; ++u) {
                for (Vertex v = 0; v < n; ++v) cover[u][v] = false;
            }
        }
    };

    Graph &g;                                            // Graph
    Vertex n;                                            // Number of vertices
    int num_threads;                                     // number of threads
    SP sp;                                               // Shortest paths data structure
    KHeap<Vertex,double> queue;                          // Queue for greedy selectoin

    std::vector<bool> selected;                          // selected[v] is true if we already picked v into the cover
    std::vector<long long> cover_size;                   // cover[v] is number of (uncovered) paths covered by v
    std::vector<long long> sp_size;                      // sp_size[v] is the number of uncovered (v,x) and (x,v) pairs
    std::vector< std::vector<long long> > cover_diff_pt; // cover_diff[i][v] is the difference in cover[v] observed by i'th thread
    std::vector< std::vector<long long> > subtree_pt;    // array to temporarily calculate subtree size;

    // Apply cover_diff found by threads to cover_size
    void apply_cover_diff() {
        for (int i = 0; i < num_threads; ++i) {
            for (Vertex v = 0; v < n; ++v) {
                cover_size[v] += cover_diff_pt[i][v];
                cover_diff_pt[i][v] = 0;
            }
        }
    }

    // Vertex weight for HHL queue, type 0 for path-greedy, type 1 for weighted greedy
    double weight(Vertex v, int type) {
        if (type == 0) return 1.0/cover_size[v];
        else if (type == 1) return static_cast<double>(sp_size[v])/cover_size[v];
        else assert(0);
    }

public:
    UHHL(Graph &g, int num_threads) :
        g(g),
        n(g.get_n()),
        num_threads(num_threads),
        sp(g, num_threads),
        queue(n),
        selected(n),
        cover_size(n),
        sp_size(n),
        cover_diff_pt(num_threads, std::vector<long long>(n)),
        subtree_pt(num_threads, std::vector<long long>(n)) {}

    // Build HHL using greedy strategy. type = 0 for path-greedy, type = 1 for label-greedy.
    void run(int type, std::vector<Vertex> &order, Labeling &labeling) {
        order.clear();
        order.resize(n);
        labeling.clear();
        queue.clear();
        sp.clear();
        for (Vertex v = 0; v < n; ++v) selected[v] = 0;

        // Calculate initial cover_size[v] and sp_size[v]
        #pragma omp parallel
        {
            std::vector<Vertex> d;
            std::vector<long long> &subtree = subtree_pt[omp_get_thread_num()];
            #pragma omp for schedule(dynamic)
            for (Vertex v = 0; v < n; ++v) {
                sp.get_descendants(v, v, d);
                sp_size[v] += d.size();
                for (size_t i = d.size(); i > 0; --i) {
                    Vertex q = d[i-1];
                    ++subtree[q];
                    cover_diff_pt[omp_get_thread_num()][q] += subtree[q];
                    if (i-1 > 0) subtree[sp.get_parent(v, q)] += subtree[q];
                    subtree[q] = 0;
                }
                sp.get_descendants(v, v, d, false);
                sp_size[v] += d.size();
            }
        }
        apply_cover_diff();

        // Initialize the greedy selection queue
        for (Vertex v = 0; v < n; ++v) queue.update(v, weight(v, type));


        // HHL greedy selection loop
        for(Vertex wi = 0; !queue.empty(); ++wi) {
            Vertex w = queue.pop();
            selected[w] = true;
            order[wi] = w;

            // Put w into labels of reachable vertices
            std::vector<Vertex> d;
            for (int forward = 0; forward < 2; ++forward) {
                sp.get_descendants(w, w, d, forward);
                // We use wi for the hub id (instead of w) so the labels are already sorted by ids
                for(size_t i = 0; i < d.size(); ++i) labeling.add(d[i], !forward, wi, sp.get_distance(d[i],w,!forward));
            }

            // Update cover[v] and sp_size[v]
            #pragma omp parallel
            {
                std::vector<Vertex> d;
                std::vector<long long> &subtree = subtree_pt[omp_get_thread_num()];
                for (int forward = 0; forward < 2; ++forward) {
                    #pragma omp for schedule(dynamic)
                    for (Vertex v = 0; v < n; ++v) {
                        sp.get_descendants(v, w, d, forward);   // Cet all v--w--d[i] shortest paths
                        sp_size[v] -= d.size();                 // Remove (v,d[i]) and (d[i],v) from sp_size[v]

                        // Iterate over all v--d[i] shortest paths and update cover.
                        // Here we use that for any d[i] there is only one v--w--d[i] shortest path.
                        // We add v--w--a--d[i] paths to the cover[a].
                        // One may ask what about v--a--w--d[i]? Look at it from d[i] in the other direction.
                        for (size_t i = d.size(); i > 0; --i) {
                            Vertex q = d[i-1];
                            ++subtree[q];
                            if (i-1 > 0 || forward) cover_diff_pt[omp_get_thread_num()][q] -= subtree[q];
                            if (i-1 > 0) subtree[sp.get_parent(v, q, forward)] += subtree[q];
                            subtree[q] = 0;
                            if (forward) sp.set_cover(v, q);
                        }
                    }
                    // Wait until all reverese paths are processed before updating cover
                    #pragma omp barrier
                }
            }

            // Collect cover[v] updates from all threads
            apply_cover_diff();
            assert(cover_size[w] == 0 && sp_size[w] == 0);

            // Update queue
            for (Vertex v = 0; v < n; ++v) if (!selected[v]) queue.update(v, weight(v, type));
        }
    }
};

}
