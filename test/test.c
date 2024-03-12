#include "eh_malloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_basic_allocation() {
    printf("Testing basic allocation...\n");
    void* ptr = eh_malloc(128);
    if (ptr == NULL) {
        printf("Allocation failed\n");
        exit(1);
    }
    memset(ptr, 0, 128);
    eh_free(ptr);
    printf("Basic allocation passed.\n");
}

void test_data_integrity() {
    printf("Testing data integrity...\n");
    int* numbers = (int*)eh_malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        numbers[i] = i;
    }
    for (int i = 0; i < 10; i++) {
        if (numbers[i] != i) {
            printf("Data integrity test failed at index %d\n", i);
            exit(1);
        }
    }
    eh_free(numbers);
    printf("Data integrity passed.\n");
}

void test_fragmentation() {
    printf("Testing fragmentation...\n");
    void* ptr1 = eh_malloc(64);
    void* ptr2 = eh_malloc(64);
    eh_free(ptr1); // Create a hole
    void* ptr3 = eh_malloc(32); // Should fit into the hole created by ptr1
    if (ptr3 == NULL) {
        printf("Fragmentation test failed: could not allocate 32 bytes\n");
        exit(1);
    }
    eh_free(ptr2);
    eh_free(ptr3);
    printf("Fragmentation test passed.\n");
}

void test_complex_operations() {
    printf("Testing complex operations...\n");
    const int size = 1024;
    char* data = (char*)eh_malloc(size);
    if (data == NULL) {
        printf("Failed to allocate memory for complex operations test\n");
        exit(1);
    }
    for (int i = 0; i < size; i++) {
        data[i] = (char)(i % 256);
    }
    for (int i = 0; i < size; i++) {
        if (data[i] != (char)(i % 256)) {
            printf("Complex operations test failed: data corruption at %d\n", i);
            exit(1);
        }
    }
    eh_free(data);
    printf("Complex operations test passed.\n");
}


void test_allocation_edge_cases() {
    printf("Testing allocation edge cases...\n");
    void* ptr = eh_malloc(0); // Edge case: zero size
    if (ptr != NULL) {
        printf("Allocation for zero size should return NULL\n");
        exit(1);
    }
    eh_free(ptr); // It's safe to call free on NULL as per C standard

    ptr = eh_malloc((size_t)-1); // Edge case: maximum size_t value, likely to fail
    if (ptr != NULL) {
        printf("Allocation for maximum size_t value should fail\n");
        exit(1);
    }
    eh_free(ptr);
    printf("Allocation edge cases passed.\n");
}

void test_randomized_alloc_free() {
    printf("Testing randomized allocation and free...\n");
    const int iterations = 100;
    void* pointers[iterations];
    for (int i = 0; i < iterations; i++) {
        size_t size = (rand() % 1024) + 1; // Random size between 1 and 1024
        pointers[i] = eh_malloc(size);
        if (pointers[i] == NULL) {
            printf("Failed to allocate %zu bytes\n", size);
            exit(1);
        }
        memset(pointers[i], 0xAB, size); // Fill with test pattern
    }

    for (int i = 0; i < iterations; i++) {
        eh_free(pointers[i]);
    }
    printf("Randomized allocation and free passed.\n");
}

void test_large_allocations() {
    printf("Testing large allocations...\n");
    const size_t large_sizes[] = {4096, 8192, 16384, 32768, 65536};
    const int num_tests = sizeof(large_sizes) / sizeof(large_sizes[0]);
    for (int i = 0; i < num_tests; i++) {
        void* ptr = eh_malloc(large_sizes[i]);
        if (ptr == NULL) {
            printf("Failed to allocate %zu bytes\n", large_sizes[i]);
            exit(1);
        }
        memset(ptr, 0xCD, large_sizes[i]);
        eh_free(ptr);
    }
    printf("Large allocations passed.\n");
}

void test_allocation_after_free() {
    printf("Testing allocation after free...\n");
    void* ptr1 = eh_malloc(1024);
    void* ptr2 = eh_malloc(1024);
    eh_free(ptr1);
    void* ptr3 = eh_malloc(512); // Should succeed after free
    if (ptr3 == NULL) {
        printf("Failed to allocate memory after free\n");
        exit(1);
    }
    memset(ptr3, 0xEF, 512);
    eh_free(ptr2);
    eh_free(ptr3);
    printf("Allocation after free passed.\n");
}

int main() {
    test_basic_allocation();
    test_data_integrity();
    test_fragmentation();
    test_complex_operations();
    test_allocation_edge_cases();
    test_randomized_alloc_free();
    test_large_allocations();
    test_allocation_after_free();
    // Call other tests here...
    printf("All tests completed.\n");
    return 0;
}