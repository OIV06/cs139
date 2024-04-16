#include "umem.h"
#include "umem.c"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

void print_free_list(node_t *list) {
    printf("Current free list:\n");
    while (list) {
        printf("Free block: Address=%p, Size=%zu, Is_Free=%d\n",
               (void *)list, list->header->size, list->header->is_free);
        list = list->next;
    }
}

void test_allocation_alignment(size_t size) {
    printf("Test 1: Allocation and Alignment\n");
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
    // Remember to free after testing if you're going to reuse this memory
    // ufree(ptr);
}

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
    // Free not needed as ptr should be NULL
}

void test_ufree() {
    printf("Test ufree\n");

    // Allocate several blocks
    void *block1 = umalloc(100);
    void *block2 = umalloc(200);
    void *block3 = umalloc(300);

    // Free the first block
    assert(ufree(block1) == 0);
    printf("Block1 freed\n");
    print_free_list(free_list);

    // Free the third block
    assert(ufree(block3) == 0);
    printf("Block3 freed\n");
    print_free_list(free_list);

    // Free the second block, which should trigger coalescing with the first and third blocks
    assert(ufree(block2) == 0);
    printf("Block2 freed and coalesced\n");
    print_free_list(free_list);
}

int main() {
    size_t region_size = 4096; // The size of the region you want to allocate
    int alloc_algo = 1; // Placeholder for allocation algorithm identifier

    printf("Initializing memory allocator...\n");
    if (umeminit(region_size, alloc_algo) == -1) {
        fprintf(stderr, "Memory allocator initialization failed.\n");
        return 1;
    }

    printf("Memory allocator initialized.\n");
    printf("Free list after initialization:\n");
    print_free_list(free_list);

    // Run test for normal allocation and alignment
    test_allocation_alignment(128);

    // Run test for zero-size allocation
    test_allocation_zero_size();

     test_ufree();

    // Additional tests can be performed here to ensure the allocator works
    // Remember to handle freeing any allocated memory and cleaning up

    return 0;
}

