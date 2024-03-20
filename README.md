# eh_malloc

The main goal of this project is to understand the basic internal structure of allocators and to learn how to write one's own allocators.

## Description
The project involves the implementation of two basic memory allocation mechanisms - Boundary Tag Algorithm (Knuth KNU73 [link to read](https://www.bradrodriguez.com/papers/ms/pat4th-c.html)) with defragmentation algorithm called on free and [SLAB allocator](https://en.wikipedia.org/wiki/Slab_allocation).

Global Heap has thee SLAB Caches for three sizeos of blocks:
Small Blocks Cache: 0b < size < 64b,
Medium Blocks Cache: 64b < size < 512b,
Big Blocks Cache: 512b < size < 4096b

And list of Boundry Tags heaps for large objects (over 4096b)

## Build and run
To build project just clone the repo and run
```sh
make
```

To build and run tests run one after another
```sh
make run_list_test
make run_test
```

To build project with profiling memory mode
```sh
BUILD_MODE=Debug make
```

To build and run tests with profiling memory mode
```sh
BUILD_MODE=Debug make run_list_test
BUILD_MODE=Debug make run_test
```

## Time mesure
To compare speed with system malloc I took two cases:
1. Created an char array with 4096 elements, each element is a pointer to memory sized as its index, so it would fit in cache algorithm.
2. Created an integer array with 5000 elements, each element is a pointer to memory sized as its index, so it's bigger than cache.
Results:
```sh
eh_malloc took '0.307000' milliseconds to execute with data from 1 bytes to 4096 bytes
system malloc took '1.495000' milliseconds to execute with data from 1 bytes to 4096 bytes

eh_malloc took '137.714000' milliseconds to execute with data from 4 bytes to 20000 bytes
system malloc took '4.328000' milliseconds to execute with data from 4 bytes to 20000 bytes
```

As we can see allocator shows perfomance better than system's one while we stay in cache and show really bad perfomance on big allocations.

(name of project is expanded to "ehillman alloc memory" since my school 42 nickname was ehillman)
