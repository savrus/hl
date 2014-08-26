// Hierarchical Hub Labels can be found by greedy algorithms.
// This file contains implementation of path-greedy and level-greedy HHL algorithms.
//
// Copyright (c) savrus, 2014

#pragma once

#include <omp.h>
#include "graph.hpp"
#include "dijkstra.hpp"
#include "kheap.hpp"
#include "labeling.hpp"

// Hierarchical Hub Labeling implementation
class HHL {
    // Class to store all shortest paths
    class SP {
        Graph &g;                                    // Graph
        Vertex n;                                    // number of vertices
        std::vector< std::vector<Distance> > dist;   // Distance table: dist[u][v] = distance(u,v)
        std::vector< std::vector<int> > cover;       // cover[u][v] is true if (u,v) pair is covered (int for thread safety)
        std::vector< std::vector<bool> > visited_pt; // mark visited vertices during graph traversal (one array per thread)

        // Check if v is on u--w path under condition that dist(v,w) = lenght.
        bool is_path(Vertex u, Vertex v, Vertex w, Distance length, bool forward) {
            return get_distance(u, w, forward) != infty
                   && get_distance(u, v, forward) != infty
                   && get_distance(u, w, forward) == get_distance(u, v, forward) + length;
        }

    public:
        SP(Graph &g, int num_threads) :
            g(g),
            n(g.get_n()),
            dist(n, std::vector<Distance>(n, infty)),
            cover(n, std::vector<int>(n)),
            visited_pt(num_threads, std::vector<bool>(n))
        {
            // Calculate distance table
            std::vector<Dijkstra> dijkstra;
            for (int i = 0; i < num_threads; ++i) dijkstra.push_back(Dijkstra(g));
            #pragma omp parallel for
            for (Vertex u = 0; u < n; ++u) {
                Dijkstra &dij = dijkstra[omp_get_thread_num()];
                dij.run(u);
                for (Vertex v = 0; v < n; ++v) dist[u][v] = dij.get_distance(v);
            }
        }

        // Set (u,v) pair covered
        void set_cover(Vertex u, Vertex v) { cover[u][v] = true; }
        // Check if pair (u,v) is covered
        bool get_cover(Vertex u, Vertex v, bool forward = true) const { return forward ? cover[u][v] : cover[v][u]; }
        // Get distance(u,v)
        Distance get_distance(Vertex u, Vertex v, bool forward = true) { return forward ? dist[u][v] : dist[v][u]; }

        // Get v's descendants in u's shorted paths DAG
        void get_descendants(Vertex u, Vertex v, std::vector<Vertex> &d, bool forward = true) {
            std::vector<bool> &visited = visited_pt[omp_get_thread_num()];
            d.clear();
            if (get_cover(u,v,forward) || get_distance(u,v,forward) == infty) return;
            d.push_back(v);
            visited[v] = true;
            for (size_t i = 0; i < d.size(); ++i) {
                for (Graph::arc_iterator a = g.begin(d[i], forward), end = g.end(d[i], forward); a < end; ++a) {
                    if (!visited[a->head] && !get_cover(u,a->head,forward) && is_path(u,d[i],a->head,a->length,forward)) {
                        d.push_back(a->head);
                        visited[a->head] = true;
                    }
                }
            }
            for (size_t i = 0; i < d.size(); ++i) visited[d[i]] = false;
        }

        // Get v's ascendants in u's shorted paths DAG
        void get_ascendants(Vertex u, Vertex v, std::vector<Vertex> &d, bool forward = true) {
            std::vector<bool> &visited = visited_pt[omp_get_thread_num()];
            d.clear();
            if (get_cover(u,v,forward) || get_distance(u,v,forward) == infty) return;
            d.push_back(v);
            visited[v] = true;
            for (size_t i = 0; i < d.size(); ++i) {
                for (Graph::arc_iterator a = g.begin(d[i], !forward), end = g.end(d[i], !forward); a < end; ++a) {
                    if (!visited[a->head] && is_path(u,a->head,d[i],a->length,forward)) {
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
    std::vector< std::vector<long long> > cover_diff_pt; // cover_diff_pt[i][v] is the difference in cover[v] observed by i'th thread

    // Vertex weight for HHL queue, type 0 for path-greedy, type 1 for weighted greedy
    double weight(Vertex v, int type) {
        if (type == 0) return 1.0/cover_size[v];
        else if (type == 1) return static_cast<double>(sp_size[v])/cover_size[v];
    }

public:
    HHL(Graph &g, int num_threads) :
        g(g),
        n(g.get_n()),
        num_threads(num_threads),
        sp(g, num_threads),
        queue(n),
        selected(n),
        cover_size(n),
        sp_size(n),
        cover_diff_pt(num_threads, std::vector<long long>(n)) {}

    // Build HHL using greedy strategy. type = 0 for path-greedy, type = 1 for label-greedy.
    void run(int type, std::vector<Vertex> &order, Labeling &labeling) {
        order.clear();
        order.resize(n);
        labeling.clear();
        queue.clear();
        sp.clear();
        for (Vertex v = 0; v < n; ++v) selected[v] = 0;

        // Calculate initial cover[v] and sp_size[v]
        #pragma omp parallel
        {
            std::vector<Vertex> d;
            #pragma omp for schedule(dynamic)
            for (Vertex v = 0; v < n; ++v) {
                for (Vertex u = 0; u < n; ++u) {
                    sp.get_descendants(u, v, d);
                    cover_size[v] += d.size();
                    if (u == v) sp_size[v] += d.size();
                }
                sp.get_descendants(v, v, d, false);
                sp_size[v] += d.size();
            }
        }

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
                for(size_t i = 0; i < d.size(); ++i) labeling.add(d[i], !forward, wi, sp.get_distance(d[i],w,!forward));
            }

            // Update cover[v] and sp_size[v]
            #pragma omp parallel
            {
                std::vector<Vertex> d;
                std::vector<Vertex> a;
                for (int forward = 0; forward < 2; ++forward) {
                    #pragma omp for schedule(dynamic)
                    for (Vertex v = 0; v < n; ++v) {
                        sp.get_descendants(v, w, d, forward);   // Cet all v--w--d[i] shortest paths
                        sp_size[v] -= d.size();                 // Remove (v,d[i]) and (d[i],v) from sp_size[v]

                        // Iterate over all v--d[i] shortest paths and update cover
                        // Note that there could be a v--d[i] shortest path without w in the middle
                        if (forward) {
                            for(size_t i = 0; i < d.size(); ++i) {
                                sp.get_ascendants(v, d[i], a, forward);
                                for (size_t j = 0; j < a.size(); ++j) ++cover_diff_pt[omp_get_thread_num()][a[j]];
                                sp.set_cover(v, d[i]);
                            }
                        }
                    }
                    // Wait until all reverese paths are processed before updating cover
                    #pragma omp barrier
                }
            }

            // Collect cover[v] updates from all threads
            for (int i = 0; i < num_threads; ++i) {
                for (Vertex v = 0; v < n; ++v) {
                    cover_size[v] -= cover_diff_pt[i][v];
                    cover_diff_pt[i][v] = 0;
                }
            }
            assert(cover_size[w] == 0 && sp_size[w] == 0);

            // Update queue
            for (Vertex v = 0; v < n; ++v) if (!selected[v]) queue.update(v, weight(v, type));
        }
    }
};
