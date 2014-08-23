// Binary Heap is a data structure used for priority queues.
// This file contains Binary Heap implementation.
//
// Copyright (c) savrus, 2014

#pragma once

#include <vector>
#include <algorithm>
#include <cassert>

// Binary Heap implementation
template<typename V, typename K> class BinHeap {
    std::vector<V> heap;            // array to store the heap
    std::vector<size_t> heap_pos;   // heap_pos[v] is index of v in the heap
    std::vector<K> key;             // key[v] is a vertex key
    size_t size;                    // current number of elements in the heap

    // swap elements with positions i and j
    void swap(size_t i, size_t j) {
        std::swap(heap_pos[heap[i]], heap_pos[heap[j]]);
        std::swap(heap[i], heap[j]);
    }
    // get position of heap[i]'s smallest child
    size_t kid(size_t i) const {
        return 2*i + (2*i+1 <= size && (key[heap[2*i+1]] < key[heap[2*i]]));
    }
    // move heap[i] up or down
    void fixup(size_t i) {
        size_t c;
        while (i*2 <= size && key[heap[i]] > key[heap[(c = kid(i))]]) { swap(i, c); i = c; }
        while (i > 1 && key[heap[i]] < key[heap[i/2]]) { swap(i, i/2); i = i/2; }
    }

    // extract v from the queue (if v is present)
    void extract(V v) {
        if (heap_pos[v] == 0) return;
        if (heap_pos[v] < size--) {
            size_t pos = heap_pos[v];
            swap(pos, size+1);
            fixup(pos);
        }
        heap_pos[v] = 0;
    }

public:
    BinHeap(size_t n) : heap(n+1), heap_pos(n), key(n), size(0) {}

    bool empty() const { return size == 0; }        // is heap empty?
    V top() const { return heap[1]; }               // get minimum elenment (heap should be non-empty)
    V pop() { V v = top(); extract(v); return v; }  // get minimum element and extract it from the queue

    // update v's key to k
    void update(V v, K k) {
        if (heap_pos[v] == 0) { heap_pos[v] = ++size; heap[heap_pos[v]] = v; }
        key[v] = k;
        fixup(heap_pos[v]);
    }

    // clear heap
    void clear() {
        for (size_t i = 1; i <= size; ++i) heap_pos[heap[i]] = 0;
        size = 0;
    }
};
