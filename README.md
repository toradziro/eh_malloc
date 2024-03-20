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

(name of project is expanded to "ehillman alloc memory" since my school 42 nickname was ehillman)
