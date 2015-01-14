// Hub Labels can be found by a greedy algorithm
// This file contains implementation of GHLp algorithm for p-norm O(log n)-optimal hub labels.
//
// Copyright (c) savrus, 2014

#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <cmath>
#include <omp.h>
#include "graph.hpp"
#include "dijkstra.hpp"
#include "kheap.hpp"
#include "labeling.hpp"

namespace hl {

// General Hub Labeling implementation
class GHL {
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
        bool is_covered(Vertex u, Vertex v, bool forward = true) const { return forward ? cover[u][v] : cover[v][u]; }
        // Get distance(u,v)
        Distance get_distance(Vertex u, Vertex v, bool forward = true) { return forward ? dist[u][v] : dist[v][u]; }

        // Get v's descendants in u's shorted paths DAG
        void get_descendants(Vertex u, Vertex v, std::vector<Vertex> &d, bool forward = true) {
            std::vector<bool> &visited = visited_pt[omp_get_thread_num()];
            d.clear();
            if (get_distance(u,v,forward) == infty) return;
            d.push_back(v);
            visited[v] = true;
            for (size_t i = 0; i < d.size(); ++i) {
                for (Graph::arc_iterator a = g.begin(d[i], forward), end = g.end(d[i], forward); a < end; ++a) {
                    if (!visited[a->head] && is_path(u,d[i],a->head,a->length,forward)) {
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
            if (get_distance(u,v,forward) == infty) return;
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

    // Proxy class for labeling to kee track if v is in u's label
    class ProxyLabeling {
        Labeling *l;                                              // Labeling
        std::vector< std::vector< std::vector<bool> > > inlabel;  // inlabel[v][forward][u] is true if v is already in u's label
    public:
        ProxyLabeling(size_t n) : inlabel(n, std::vector< std::vector<bool> >(2, std::vector<bool>(n))) {}
        // Initialize with new labeling
        void set_labeling(Labeling &labeling) { clear(); labeling.clear(); l = &labeling; }
        // Add hub (v,d) to forward or reverse label of u
        void add(Vertex u, bool forward, Vertex v, Distance d) {
            if (!inlabel[v][forward][u]) {
                l->add(u, forward, v, d);
                inlabel[v][forward][u] = true;
            }
        }
        // Get u's forward or reverse label size
        size_t get_size(Vertex u, bool forward) { return l->get_label_hubs(u)[forward].size(); }
        // Get if v is in u's forward or reverse label
        bool is_in_label(Vertex u, bool forward, Vertex v) { return inlabel[v][forward][u]; }
        // Clear
        void clear() {
            for (Vertex v = 0; v < inlabel.size(); ++v) {
                for (int forward = 0; forward < 2; ++forward) {
                    for (Vertex u = 0; u < inlabel[v][forward].size(); ++u) inlabel[v][forward][u] = false;
                }
            }
        }
    };

    // Class for Approximate Maximum Density Subgraph computation
    class AMDS {
        Vertex n;                                         // Number of vertices in graph
        SP *sp;                                           // Shortest paths data structure
        ProxyLabeling *l;                                 // Labeling with is_in_label() interface
        KHeap<Vertex, double> queue;                      // ADS queue
        std::vector< std::vector<size_t> > degree;        // degree[forward][u] is u's degree at `forward` side of the center graph
        std::vector< std::vector<bool> > inads;           // inads[forward][u] is true if u is at `forward` side of the center graph

        // Get p-norm vertex weight
        double weight(Vertex u, bool forward, float p) {
            double base = l->get_size(u, forward);
            return pow(base + 1, p) - pow(base, p);
        }

        // Get edges/vertices_weight with correct value when vertices_weight = 0
        double ratio(size_t edges, double vertices_weight) {
            if (edges == 0) return 0;
            if (vertices_weight == 0) return std::numeric_limits<double>::max();
            return static_cast<double>(edges)/vertices_weight;
        }

    public:
        AMDS(Graph &g, SP &sp, ProxyLabeling &l) : n(g.get_n()), sp(&sp), l(&l), queue(2*n), degree(2,std::vector<size_t>(n)), inads(2, std::vector<bool>(n)) {}

        // Check if vertex u[forward] is in AMDS.
        // This should be used only if the density repored by run exceeds the limit
        bool is_in(Vertex u, bool forward) { return inads[forward][u]; }

        // Find the density of AMDS of v's center graph or the subgraph with density greater than the limit
        double run(Vertex v, float p, double limit = std::numeric_limits<double>::max()) {
            queue.clear();
            size_t edges = 0;
            double vertices_weight = 0;
            std::vector<Vertex> descendants;

            // Determine the initial center graph and put vertex weights into the queue
            for (Vertex u = 0; u < n; ++u) {
                for (int forward = 0; forward < 2; ++forward) {
                    sp->get_descendants(u, v, descendants, forward);
                    size_t d = 0;
                    for (size_t i = 0; i < descendants.size(); ++i) {
                        if (!sp->is_covered(u, descendants[i], forward)) ++d;
                    }
                    degree[forward][u] = d;
                    inads[forward][u] = d > 0;
                    if (forward) edges += d;
                    if (d > 0 && !l->is_in_label(u, forward, v)) {
                        double uw = weight(u, forward, p);
                        queue.update(u + n*forward, static_cast<double>(d)/uw);
                        vertices_weight += uw;
                    }
                }
            }

            // AMDS loop: iteratively remove smallest-weight vertex
            double r = ratio(edges, vertices_weight), best_r = r;
            while (!queue.empty() && r < limit) {
                Vertex u = queue.pop();
                bool forward = u >= n;
                if (forward) u -= n;
                inads[forward][u] = false;
                edges -= degree[forward][u];
                vertices_weight -= weight(u, forward, p);
                sp->get_descendants(u, v, descendants, forward);
                for(size_t i = 0; i < descendants.size(); ++i) {
                    Vertex w = descendants[i];
                    if (!inads[!forward][w] || sp->is_covered(u, w, forward)) continue;
                    assert(degree[!forward][w] > 0);
                    --degree[!forward][w];
                    double ww = weight(w, !forward, p);
                    if (degree[!forward][w] == 0) inads[!forward][w] = false;
                    if (!l->is_in_label(w, !forward, v)) {
                        if (degree[!forward][w] == 0) {
                            queue.extract(w + n*(!forward));
                            vertices_weight -= ww;
                        } else queue.update( w + n*(!forward), static_cast<double>(degree[!forward][w])/ww);
                    }
                }
                r = ratio(edges, vertices_weight);
                if (best_r < r) best_r = r;
            }
            return best_r;
        }
    };

    Graph &g;                                            // Graph
    Vertex n;                                            // Number of vertices
    int num_threads;                                     // number of threads
    SP sp;                                               // Shortest paths data structure
    KHeap<Vertex,double> queue;                          // Queue for greedy selectoin
    ProxyLabeling proxy;                                 // Proxy for labels to keep additional info
    std::vector<double> density;                         // density[v] is a last_seen v's center graph AMDS density
    std::vector<AMDS> amds_pt;                           // amds_pt[i] - object to calculate AMDS for i'th thread

    // Add AMDS from v's center graph into the cover
    void increase_cover(Vertex v, AMDS &amds) {
        std::vector<Vertex> descendants;
        for (int forward = 0; forward < 2; ++forward) {
            for (Vertex u = 0; u < n; ++u) {
                if (!amds.is_in(u, forward)) continue;
                proxy.add(u, forward, v, sp.get_distance(u, v, forward));
                if (forward) {
                    sp.get_descendants(u, v, descendants);
                    for (size_t i = 0; i < descendants.size(); ++i) {
                        Vertex w = descendants[i];
                        if (amds.is_in(w, false)) {
                            sp.set_cover(u,w);
                        }
                    }
                }
            }
        }
    }

public:
    GHL(Graph &g, int num_threads) :
        g(g),
        n(g.get_n()),
        num_threads(num_threads),
        sp(g, num_threads),
        queue(n),
        proxy(n),
        density(n)
        {
            for (int i = 0; i < num_threads; ++i) amds_pt.push_back(AMDS(g, sp, proxy));
        }

    // Build GHLp labels for p-norm labels
    void run(Labeling &labeling, float alpha = 1.1, float p = 1.0) {
        queue.clear();
        sp.clear();
        proxy.set_labeling(labeling);

        // Calculate initial density[v]
        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic)
            for (Vertex v = 0; v < n; ++v) {
                double r = amds_pt[omp_get_thread_num()].run(v, p);
                density[v] = r;
                #pragma omp critical
                queue.update(v, 1.0/r);
            }
        }

        // GHLp lazy-update queue
        #if 0
        // Single-tread version
        AMDS &amds = amds_pt[0];
        while (!queue.empty()) {
            Vertex v = queue.pop();
            double r = amds.run(v, p, density[v]/alpha);
            if (r > std::numeric_limits<double>::epsilon()) {
                density[v] = r;
                queue.update(v, 1.0/r);
                if (r - density[v]/alpha > std::numeric_limits<double>::epsilon()) increase_cover(v, amds);
            }
        }
        #else
        // Multy-thread version
        while (!queue.empty()) {
            std::vector< std::pair<double, std::pair<Vertex, int> > > top(num_threads, std::make_pair(0.0, std::make_pair(none, -1)));
            for (int i = 0; i < num_threads && !queue.empty(); ++i) top[i].second = std::make_pair(queue.pop(), i);
            #pragma omp parallel
            {
                int i = omp_get_thread_num();
                Vertex v = top[i].second.first;
                if (v != none) top[i].first = amds_pt[i].run(v, p, density[v]/alpha);
            }
            std::sort(top.begin(), top.end(), std::greater<std::pair<double, std::pair<Vertex, int> > >());
            for (int i = 0; i < num_threads; ++i) {
                Vertex v = top[i].second.first;
                if (v != none && top[i].first > std::numeric_limits<double>::epsilon()) {
                    density[v] = top[i].first;
                    queue.update(v, 1.0/density[v]);
                }
            }
            Vertex v = top[0].second.first;
            if (top[0].first - density[v]/alpha > std::numeric_limits<double>::epsilon()) {
                increase_cover(v, amds_pt[top[0].second.second]);
            }
        }
        #endif

        // Order hubs in each label
        labeling.sort();
    }
};

} // namespace hl

