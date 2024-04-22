#include "umem.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

// Tests the allocation and alignment to ensure 8-byte alignment
void test_allocation_alignment(size_t size) {
    printf("Test 1: Allocation and Alignment for size %zu\n", size);
    void *ptr = umalloc(size);
    if (ptr == NULL) {
        printf("Allocation failed for size %zu\n", size);
        printf("Test 1: FAIL\n");
    } else if (((uintptr_t)ptr & 7) != 0) {
        printf("Allocation for size %zu is not aligned to 8 bytes. Address=%p\n", size, ptr);
        printf("Test 1: FAIL\n");
    } else {
        printf("Allocation and alignment successful for size %zu. Address=%p\n", size, ptr);
        printf("Test 1: PASS\n");
    }
    ufree(ptr); // Frees the allocated memory
}

// Tests that a request for zero bytes returns NULL
void test_allocation_zero_size() {
    printf("Test 2: Zero-Size Allocation\n");
    void *ptr = umalloc(0);
    if (ptr != NULL) {
        printf("Non-NULL returned for zero-size allocation.\n");
        printf("Test 2: FAIL\n");
    } else {
        printf("Correctly returned NULL for zero-size allocation.\n");
        printf("Test 2: PASS\n");
    }
}

// Tests freeing of memory blocks and checks for coalescing of free space
void test_ufree() {
    printf("Test 3: Free and Coalesce\n");

    void *block1 = umalloc(100);
    void *block2 = umalloc(200);
    void *block3 = umalloc(300);

    ufree(block1);  // Free the first block
    printf("Block1 freed\n");

    ufree(block3);  // Free the third block
    printf("Block3 freed\n");

    ufree(block2);  // Free the second block, which should coalesce all blocks
    printf("Block2 freed and coalesced\n");

    umemdump();  // Displays the state of memory after coalescing
}



int main() {
    size_t region_size = 4096;
    int alloc_algo = FIRST_FIT;

    printf("Initializing memory allocator...\n");
    if (umeminit(region_size, alloc_algo) == -1) {
        fprintf(stderr, "Memory allocator initialization failed.\n");
        return 1;
    }
    printf("Memory allocator initialized with FIRST FIT strategy.\n");
    printf("Free list after initialization:\n");
    umemdump();

    // Execute tests
    test_allocation_alignment(128);
    test_allocation_zero_size();
    test_ufree();
   

    return 0;
}
