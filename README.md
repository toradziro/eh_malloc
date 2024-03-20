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

## Time measure
To compare speed with system malloc I created an integer array with 5000 elements, each element is a pointer to memory sized as its index, so in 1t element is only 4 bytes allocated and in 5000th element is 20kb is allocated
Speed compare result:
```sh
eh_malloc took '6.083000' milliseconds to execute 
system malloc took '4.211000' milliseconds to execute 
```
It's slower but not critical, for learning project result is acceptable

(name of project is expanded to "ehillman alloc memory" since my school 42 nickname was ehillman)
