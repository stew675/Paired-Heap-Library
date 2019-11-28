Stew Forster's Paired Heap Library

git@github.com:stew675/Paired-Heap-Library.git

My personal interpretation and implementation of the pairing heap algorithm as described by:

https://www.cs.cmu.edu/~sleator/papers/pairing-heaps.pdf

The author, Stew Forster (stew675@gmail.com) reserves all rights to the code herein.
I make this code available for general public non-commercial and educational use
Please contact the author at stew675@gmail.com regarding any requests for commercial use


###### Contents
- README.md - This file
- Makefile - To build the library with the provided test framework
- ph.h - A documented user facing API header file
- ph.c - The implementation of the paired heap algorithm
- phtest.c - A light-weight test framework for the algorithm
- pht.c - A test utility to analyse performance of `pheap_delete()`

###### Experimentally Observed Function Execution Times On Random Data Sets

Function | Execution Time | Description
-----|:-----:|-----
`pheap_create()` | **O(1)**  | Create a new heap
`pheap_destroy()` | **O(n)**  | Destroy a heap of *n* nodes
`pheap_insert()` | **O(1)**   | Insert a new node into the heap
`pheap_delete_min()` | **O(log n)** <sup>(1)</sup> | Delete the minimum node from a heap of *n* nodes
`pheap_delete()` | **O(log log n)** <sup>(2)</sup> | Delete a specific node from the heap of *n* nodes
`pheap_change_key()` | **O(log log n)** <sup>(3)</sup> | Change the key of a specific node within a heap of *n* nodes
`pheap_get_min_node()` | **O(1)** | Retrieve the minimum node
`pheap_set_data()` | **O(1)** | Set the data of a specific node

1. Has an **O(n)** worst case upper bound (observable when operating on a fresh heap with nothing other than `pheap_insert()` operations having taken place prior which means no internal pair merges have yet run).
When repeatedly operating on the root node the research paper suggests an upper-bounded theoretical amortised cost of **O(log n)**, and this is observable in practise.

2. When operating on the root node only, observable execution times are as per `pheap_delete_min()`.
When operating on nodes selected at random from within an active heap the execution time is observed to scale according to **O(log log n)**.

3. When operating on the root node only, if decreasing the key, or increasing the key without root child nodes this has an **O(1)** execution time, otherwise observable execution times on the root node are as per `pheap_delete_min()`.  When operating on nodes selected at random from within an active heap the execution time is observed to scale according to **O(log log n)**.
