#include <umem.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h> 

#define PAGE_SIZE getpagesize()

node_t* free_list = NULL;
static int allocator_initialized = 0;  

typedef struct header_t {
    size_t size;     // Size of the memory block (including the header)
    int is_free;    // Flag indicating if this block is free
    long magic;     // Magic number for error checking, if necessary
} header_t;

typedef struct node_t {
    struct header_t *header;   // Pointer to the header of the block
    struct node_t *next;       // Pointer to the next node in the list
    struct node_t *prev;       // Pointer to the previous node in the list (if using a doubly-linked list)
} node_t;

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (allocator_initialized) {
        fprintf(stderr, "Allocator is already initialized.\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Invalid size of region.\n");
        return -1;
    }

    size_t pages = (sizeOfRegion + sizeof(node_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t region_size = pages * PAGE_SIZE;

    node_t *region = (node_t *)mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (region == MAP_FAILED) {
        perror("Failed to mmap");
        return -1;
    }

    // Initialize the embedded header
    region->header->size = region_size - sizeof(header_t);
    region->header->is_free = 1;
    region->header->magic = 0x12345678; // Example magic number

    // Initialize the free list links
    region->next = NULL;
    region->prev = NULL;
    free_list = region;

    allocator_initialized = 1;  // Mark the allocator as initialized
    return 0;
}
