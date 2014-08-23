hl - Hub Labeling Algorithms
==

*Hub Labeling* is a data structure used to find a distance between two vertices in a graph.
It provides the excellent query time for real-world graphs such as road networks and social networks.
Hub Labeling has two stages: preprocessing and querying.
While the query algorithm is simple, the preprocessing is much more involved and there are several preprocessing algorithms in the literature.

There is an algorithm to find O(log n)-approximately optimal Hub Labels. However, it is slow.
Practical labeling algorithms find a *Hierarchical Hub Labeling*.
HHL algorithms order vertices and find hub labels which respect the order.
In fact given an order one can build the minimum HHL with respect to this order.

## Usage

There are several programs in the repository:

* `hhl` — find Hierarchical Hub Labeling (and a vertex order) using greedy algorithm
* `akiba` — construct Hierarchical Hub Labeling from a vertex order
* `degree` — order vertices by their degree
* `lcheck` — check labels

### Build

Just use `make` to build the project:
```
$ make
g++ -fopenmp -O3 -I./lib -o akiba akiba.cpp
g++ -fopenmp -O3 -I./lib -o degree degree.cpp
g++ -fopenmp -O3 -I./lib -o hhl hhl.cpp
g++ -fopenmp -O3 -I./lib -o lcheck lcheck.cpp
```

### HHL

Once you've built the `hhl` program you can compute HHL for a graph, say `email.graph`:
```
$ ./hhl email.graph
Average label size 39.6134
Maximum label size 78

real    0m1.165s
```
The `hhl` program contains implementations of two greedy HHL algorithms: path-greedy and label-greedy (as defined in [1]).
The default choice is path-greedy. To use label-greedy add `-w` argument:
```
$ ./hhl -w email.graph
Average label size 36.797
Maximum label size 82

real    0m1.170s
```

If shortest paths are unique, greedy algorithms can be implemented more efficiently in terms of the running time.
The `hhl` program has argument `-u` which turns on the unique shortest paths mode.
You can use `-u` with any graph, `hhl` will just emulate the unique shortest path structure and run fast algorithm:
```
$ ./hhl -u email.graph
Average label size 50.6531
Maximum label size 112

real    0m0.336s
```

Options `-o order_file` and `-l label_file` can be specified to write the ordering and labels to files:
```
$ ./hhl -o email.order -l email.labels email.graph
Average label size 39.6134
Maximum label size 78
```

If your CPU has Hyper Threading enabled, use `-t` argument to specify the number of threads to be equal to the real number of cores.
Performance may reduce dramatically if there are more threads than cores.

Keep in mind that `hhl` requires O(n^2) memory and the time complexity is O(n^3) (O(nm log n) if `-u` is set).


### Akiba

Akiba et al. found an efficient algorithm to construct minimal HHL from a vertex order.
You can use the `akiba` program to construct the labels if you have an order file prepared (for example, by the `hhl` program).
```
$ ./akiba -o email.order email.graph
Average label size 39.6134
Maximum label size 78

real    0m0.081s
```
As you can see, `akiba` program reported exactly the same labeling size.
What's interesting is that the order doesn't have to be from a rigorous algorithm.
For example we can find the order in the unique shortest paths mode and use `akiba` program to find the best labels corresponding to this order:
```
$ ./hhl -u -o email.order email.graph
Average label size 50.6531
Maximum label size 112
$ ./akiba -o email.order email.graph
Average label size 39.707
Maximum label size 82
```
Another simple way of ordering vertices is the degree order that works well for some social and computer networks: to order vertices by their degrees, breaking ties at random.
Use `degree` program to find the degree order:
```
$ ./degree -o email.order email.graph
$ ./akiba -o email.order email.graph
Average label size 39.2462
Maximum label size 89
```
Akiba et al algorithm is much faster than greedy HHL algorithms. Furthermore, it requires a linear amount of memory, so `degree`+`akiba` can be used for large graphs:
```
$ ./degree -o coAuthorsCiteseer.order coAuthorsCiteseer.graph
$ ./akiba -o coAuthorsCiteseer.order coAuthorsCiteseer.graph
Average label size 405.171
Maximum label size 1170
```
This may take about 20 minutes. The coAuthorsCiteseer graph has about 200000 vertices which is too much for `hhl`.

The `akiba` program also has argument `-l label_file` to write the labels.

### lcheck

Once you have the labels from a complex algorithm you may want to verify that this labels are correct.
You can use `lcheck` with an option `-c` for that.
Without this option `lcheck` only reports the average and maximum label sizes.
```
$ ./lcheck -c -l email.labels email.graph
Labels OK
Average label size 39.6134
Maximum label size 78

real    0m0.233s
```

## Graphs

You can use any graph in DIMACS shortest path or METIS format. We suggest visiting [Dimacs 10 Challenge](http://www.cc.gatech.edu/dimacs10/archive/clustering.shtml) page for general graphs and [Dimacs 9 Challenge](http://www.dis.uniroma1.it/challenge9/download.shtml) for road networks.

## Basic Reference

Material covered in Sections 2, 3.1, 3.2, and A1 of [1] should be enough to understand `akiba` and `hhl` programs. Everything else is out of the scope of this project. Those who seek deeper knowledge of the Hub Labeling topic are encouraged to read the other papers.

1. D. Delling, A.V. Goldberg, T. Pajor, and R. F. Werneck. *Robust Exact Distance Queries on Massive Networks.* Technical Report MSR-TR-2014-12, Microsoft Research, 2014. [link](http://research.microsoft.com/apps/pubs/default.aspx?id=208867)

## Advanced Reference

2. I. Abraham, D. Delling, A.V. Goldberg, and R.F. Werneck. *Hierarchical Hub Labelings for Shortest Paths.* In Proc. 20th European Symposium on Algorithms (ESA 2012), 2012. [link](http://research.microsoft.com/apps/pubs/default.aspx?id=168725)

3. T. Akiba, Y. Iwata, and Y. Yoshida. *Fast Exact Shortest-path Distance Queries on Large Networks by Pruned Landmark Labeling.* In Proceedings of the 2013 ACM SIGMOD International Conference on Management of Data, SIGMOD'13, pages 349--360. ACM, 2013. [link](http://arxiv.org/abs/1304.4661)

4. M. Babenko, A.V. Goldberg, A. Gupta, and V. Nagarajan. *Algorithms for Hub Label Optimization.* In Proc. 30th ICALP, pages 69--80. LNCS vol. 7965, Springer, 2013. [link](http://research.microsoft.com/apps/pubs/default.aspx?id=192125)

5. E. Cohen, E. Halperin, H. Kaplan, and U. Zwick. *Reachability and Distance Queries via 2-hop Labels.* SIAM Journal on Computing, 32, 2003. [link](http://www.cs.tau.ac.il/~zwick/papers/labels-full.ps)

6. D. Delling, A.V. Goldberg, R. Savchenko, and R.F. Werneck. *Hub Labels: Theory and Practice.* In Proceedings of the 13th International Symposium on Experimental Algorithms (SEA'14), LNCS. Springer, 2014. [link](http://research.microsoft.com/apps/pubs/default.aspx?id=219802)
