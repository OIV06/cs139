#include "umem.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

void testAllocAllign(size_t size);
void testAllocZero();
void testuFree();
void prntBlockInfo(const char *msg, void *ptr, size_t size);
void testFitStrat();
// Tests the allocation and alignment to ensure 8-byte alignment
void testAllocAllign(size_t size)
{
    printf("Test 1: allocation/alignment for size %zu\n", size);
    void *ptr = umalloc(size);
    if (ptr == NULL)
    {
        printf("Test 1: fail\n");
    }
    else if (((uintptr_t)ptr & 7) != 0)
    {
        printf("size %zu is not to 8 bytes. Address=%p\n", size, ptr);
        printf("Test 1: fail\n");
    }
    else
    {
        printf("allocation/allignment successful for size %zu. Address=%p\n", size, ptr);
        printf("Test 1: PASS\n");
    }
    ufree(ptr); // free the allocated memory
}

// Tests that a request for zero bytes returns NULL
void testAllocZero()
{
    printf("Test 2:zero alloc \n");
    void *ptr = umalloc(0);
    if (ptr != NULL)
    {
        printf("Test 2: fail\n");
    }
    else
    {
        printf("returned null for zero alloc \n");
        printf("Test 2: PASS\n");
    }
}

// Tests freeing of memory blocks and checks for coalescing of free space
void testuFree()
{
    printf("Test 3: Free and Coalesce\n");

    void *block1 = umalloc(100);
    void *block2 = umalloc(200);
    void *block3 = umalloc(300);

    ufree(block1); // Free the first block
    printf("Block1 freed\n");
    umemdump();
    ufree(block3); // Free the third block
    printf("Block3 freed\n");
    umemdump();
    ufree(block2); // Free the second block, which should coalesce all blocks
    printf("Block2 freed and coalesced\n");

    umemdump(); // check status
}

void prntBlockInfo(const char *msg, void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        printf("%s - allocation failed size %zu\n", msg, size);
    }
    else
    {
        printf("%s - allocation successful size %zu. Address=%p\n", msg, size, ptr);
    }
}

void testFitStrat()
{
    printf("\nTESTING FIT STRAT\n");
    // allocate a few blocks of different sizes
    void *ptr1 = umalloc(100);
    prntBlockInfo("100 bytes", ptr1, 100);
    void *ptr2 = umalloc(500);
    prntBlockInfo("500 bytes", ptr2, 500);
    void *ptr3 = umalloc(800);
    prntBlockInfo("800 bytes", ptr3, 800);
    // check free list status
    printf("\nfree list after allocations:\n");
    umemdump();
    // free the first two blocks
    ufree(ptr1);
    printf("freed 100 bytes block.\n");
    ufree(ptr2);
    printf("freed 500 bytes block.\n");
    // checking status
    printf("\nfree list after freeing two blocks:\n");
    umemdump();
    void *ptr4 = umalloc(200);
    prntBlockInfo("allocate 200 bytes", ptr4, 200);

    // check final state of free list
    printf("\nfinal free list state:\n");
    umemdump();
    // Cleanup
    ufree(ptr3);//freeeee
    ufree(ptr4);
}

int main()
{
    size_t region_size = 4096;
    int alloc_algo = WORST_FIT; // BEST_FIT NEXT_FIT FIRST_FIT where fit strategies are set

    if (umeminit(region_size, alloc_algo) == -1)
    {
        fprintf(stderr, "memory allocator initialization failed.\n");
        return 1;
    }

    printf("memory allocator initialized with strategy %d.\n", alloc_algo);
    printf("free list after initialization:\n");
    umemdump();

    // Execute tests
    testAllocAllign(128);
    testAllocZero();
    testuFree();
    testFitStrat();

    return 0;
}
