#include "umem.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#define MAGIC_NUMBER 0x12345678
#define PAGE_SIZE getpagesize()


static int allocator_initialized = 0;  

typedef struct header_t {
    size_t size;     
    int is_free;    
    long magic;     // Magic number for error checking, if necessary
} header_t;

typedef struct node_t {
    struct header_t *header;   // Pointer to the header of the block
    struct node_t *next;       // Pointer to the next node in the list
    struct node_t *prev;       // Pointer to the previous node in the list (if using a doubly-linked list)
} node_t;
node_t* free_list = NULL;


size_t align_size(size_t size) {
    return (size + 7) & ~((size_t)7);
}



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

    region->header = (header_t *)((char *)region + sizeof(node_t));
    
    
    region->header->size = region_size - sizeof(node_t) - sizeof(header_t);
    region->header->is_free = 1;
    region->header->magic = 0x12345678; 

  
    region->next = NULL;
    region->prev = NULL;
    free_list = region;

    allocator_initialized = 1;  
    return 0;
}

static int allocation_algorithm = FIRST_FIT;

static node_t* last_allocated = NULL;

node_t* find_best_fit(size_t size) {
    node_t *current = free_list;
    node_t *best_fit_node = NULL;

    while (current != NULL) {
        if (current->header->is_free && current->header->size >= size) {
            if (best_fit_node == NULL || current->header->size < best_fit_node->header->size) {
                best_fit_node = current;
            }
        }
        current = current->next;
    }
    return best_fit_node;
}

node_t* find_worst_fit(size_t size) {
    node_t *current = free_list;
    node_t *worst_fit_node = NULL;

    while (current != NULL) {
        if (current->header->is_free && current->header->size >= size) {
            if (worst_fit_node == NULL || current->header->size > worst_fit_node->header->size) {
                worst_fit_node = current;
            }
        }
        current = current->next;
    }
    return worst_fit_node;
}

node_t* find_first_fit(size_t size) {
    node_t *current = free_list;
    while (current != NULL) {
        if (current->header->is_free && current->header->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

node_t* find_next_fit(size_t size) {
    if (last_allocated == NULL) {
        last_allocated = free_list;
    }
    node_t *current = last_allocated;
    do {
        if (current->header->is_free && current->header->size >= size) {
            last_allocated = current;
            return current;
        }
        current = current->next ? current->next : free_list; 
    } while (current != last_allocated);

    return NULL;
}

void *umalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = align_size(size);  
    node_t *block = NULL;

    switch (allocation_algorithm) {
        case BEST_FIT:
            block = find_best_fit(size);
            break;
        case WORST_FIT:
            block = find_worst_fit(size);
            break;
        case FIRST_FIT:
            block = find_first_fit(size);
            break;
        case NEXT_FIT:
            block = find_next_fit(size);
            break;
        default:
            
            return NULL;
    }

    if (!block) {
        // No suitable block found
        return NULL;
    }

    // Check if we can split the block
    size_t total_size = block->header->size + sizeof(header_t);  
    if (total_size > size + sizeof(header_t) + sizeof(node_t)) {
        
        node_t *new_block = (node_t *)((char *)(block->header) + size + sizeof(header_t));
        new_block->header = (header_t *)((char *)new_block + sizeof(node_t));
        new_block->header->size = total_size - size - sizeof(header_t) - sizeof(node_t);
        new_block->header->is_free = 1;
        new_block->header->magic = block->header->magic;

        
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;

        
        block->header->size = size;
    }

    
    block->header->is_free = 0;
    return (void *)(block->header + 1);  
}

int ufree(void *ptr) {
    if (!ptr) {
        // NULL pointer check - standard behavior of free to do nothing.
        return 0;
    }

    // Calculate the header address from the given pointer.
    header_t *header = (header_t *)ptr - 1;

    // Check the magic number for error checking, indicates if the block was allocated by umalloc.
    if (header->is_free || header->magic != MAGIC_NUMBER) {
        // The block is either already freed or was not allocated by umalloc (corrupted).
        return -1;
    }

    // Mark the block as free.
    header->is_free = 1;

    // Find the actual node from the header.
    node_t *node = (node_t *)((char *)header - offsetof(node_t, header));

    // Coalesce with next block if it's free.
    if (node->next && node->next->header->is_free) {
        header->size += sizeof(node_t) + node->next->header->size; // Increase the size of the current block.
        node->next = node->next->next; // Remove the next block from the list.
        if (node->next) {
            node->next->prev = node;
        }
    }

    // Coalesce with previous block if it's free.
    if (node->prev && node->prev->header->is_free) {
        node->prev->header->size += sizeof(node_t) + header->size; // Increase the size of the previous block.
        node->prev->next = node->next; // Connect the previous block to the next.
        if (node->next) {
            node->next->prev = node->prev;
        }
        node = node->prev; // Update the node pointer to the previous node.
    }

    // Update the free list head if needed.
    if (free_list == NULL || node < free_list) {
        free_list = node; // This node is now the new head of the free list.
    }

    return 0;
}
