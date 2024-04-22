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

void print_block_info(const char* msg, void* ptr, size_t size) {
    if (ptr == NULL) {
        printf("%s - Allocation failed for size %zu\n", msg, size);
    } else {
        printf("%s - Allocation successful for size %zu. Address=%p\n", msg, size, ptr);
    }
}

// Test the worst fit strategy
void test_worst_fit() {
    printf("\n--- Testing Worst Fit Strategy ---\n");
    
    // Allocate a few blocks of different sizes
    void* ptr1 = umalloc(100); // Small block
    print_block_info("Allocate 100 bytes", ptr1, 100);
    
    void* ptr2 = umalloc(500); // Medium block
    print_block_info("Allocate 500 bytes", ptr2, 500);
    
    // This should take the largest available block
    void* ptr3 = umalloc(800); // Large block, hoping to hit the worst fit
    print_block_info("Allocate 800 bytes", ptr3, 800);

    // Check free list status
    printf("\nFree list after allocations:\n");
    umemdump();

    // Free the first two blocks
    ufree(ptr1);
    printf("Freed 100 bytes block.\n");
    ufree(ptr2);
    printf("Freed 500 bytes block.\n");

    // Check free list status
    printf("\nFree list after freeing two blocks:\n");
    umemdump();

    // Next allocation should pick the newly coalesced largest block
    void* ptr4 = umalloc(200); // Another block, should also be from the largest block if worst fit works
    print_block_info("Allocate 200 bytes", ptr4, 200);

    // Check final state of free list
    printf("\nFinal free list state:\n");
    umemdump();

    // Cleanup
    ufree(ptr3);
    ufree(ptr4);
}

int main() {
    size_t region_size = 4096;
    int alloc_algo = WORST_FIT;  // Set your desired allocation strategy

    printf("Initializing memory allocator with the selected strategy...\n");
    if (umeminit(region_size, alloc_algo) == -1) {
        fprintf(stderr, "Memory allocator initialization failed.\n");
        return 1;
    }

    printf("Memory allocator initialized with strategy %d.\n", alloc_algo);
    printf("Free list after initialization:\n");
    umemdump();

    // Execute tests
    test_allocation_alignment(128);
    test_allocation_zero_size();
    test_ufree();
       test_worst_fit();

    return 0;
}
