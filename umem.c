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
#define PAGE_SIZE getpagesize()
static int allocator_initialized = 0;  

typedef struct hed_r{
    size_t size;     
    int is_free;    
} hed_r;
//set up for linked
typedef struct node {
    struct hed_r *header;   
    struct node *next;       
    struct node *prev;      
} node;
node* freeList = NULL;

//get allign size to m of 8
size_t align_size(size_t size) {
    return (size + 7) & ~((size_t)7);
}

static int alloc_algo = BEST_FIT;
//umeminit
int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (allocator_initialized) {
        fprintf(stderr, "allocator is alreafy initialized.\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "wrong size of region.\n");
        return -1;
    }

    size_t pages = (sizeOfRegion + sizeof(node) + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t region_size = pages * PAGE_SIZE;

    node *region = (node *)mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (region == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
    region->header = (hed_r*)((char *)region + sizeof(node));
    
    region->header->size = region_size - sizeof(node) - sizeof(hed_r);
    region->header->is_free = 1;
    
    region->next = NULL;
    region->prev = NULL;
    freeList = region;

    allocator_initialized = 1;  
     alloc_algo = allocationAlgo; 
    return 0;
}


//fit strategy algo,best,worst,next,first
static node* last_allocated = NULL;

node* findBestFit(size_t size) {
    node *curr = freeList;
    node *bestFit = NULL;
    while (curr != NULL) {// Iterate through the free list to find the best fit.
        if (curr->header->is_free && curr->header->size >= size) {
            if (bestFit == NULL || curr->header->size < bestFit->header->size) {
                bestFit = curr;
            }
        }
        curr = curr->next;
    }
    return bestFit;
}
//find largest free block for the given size.
node* findWorstFit(size_t size) {
    node *curr = freeList;
    node *worstFit = NULL;
    while (curr != NULL) {
        if (curr->header->is_free && curr->header->size >= size) {
            if (worstFit == NULL || curr->header->size > worstFit->header->size) {
                worstFit = curr;
            }
        }
        curr = curr->next;
    }
    return worstFit;
}
//finding first block that fits
node* findFirstFit(size_t size) {
    node *curr = freeList;
    while (curr != NULL) {
        if (curr->header->is_free && curr->header->size >= size) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}
//finding next freeblock with last allocated
node* findNextFit(size_t size) {
    if (last_allocated == NULL) {
        last_allocated = freeList;
    }
    node *curr = last_allocated;
    do {
        if (curr->header->is_free && curr->header->size >= size) {
            last_allocated = curr;
            return curr;
        }//going back to start of list if needed
        curr = curr->next ? curr->next : freeList; 
    } while (curr != last_allocated);

    return NULL;
}
//umalloc
void *umalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
//choosing best fit 
    size = align_size(size);  
    node *block = NULL;

    switch (alloc_algo) {
        case BEST_FIT:
            block = findBestFit(size);
            break;
        case WORST_FIT:
            block = findWorstFit(size);
            break;
        case FIRST_FIT:
            block = findFirstFit(size);
            break;
        case NEXT_FIT:
            block = findNextFit(size);
            break;
        default:
            
            return NULL;
    }

    if (!block) {
        
        return NULL;
    }

    // check for block splitiing 
    size_t total_size = block->header->size + sizeof(hed_r);  
    if (total_size > size + sizeof(hed_r) + sizeof(node)) {
        
        node *newBlock = (node *)((char *)(block->header) + size + sizeof(hed_r));
        newBlock->header = (hed_r*)((char *)newBlock + sizeof(node));
        newBlock->header->size = total_size - size - sizeof(hed_r) - sizeof(node);
        newBlock->header->is_free = 1;
        

        
        newBlock->next = block->next;
        newBlock->prev = block;
        if (block->next) {
            block->next->prev = newBlock;
        }
        block->next = newBlock;

        
        block->header->size = size;
    }

    
    block->header->is_free = 0;
    return (void *)(block->header + 1);  
}


int ufree(void *ptr) {
    if (ptr == NULL) {
        return 0;
    }
    hed_r *blockHeadr = (hed_r*)ptr - 1;

    blockHeadr->is_free = 1;

    // coalesce if free
    node *blockNode = (node *)((char *)blockHeadr - sizeof(node));
    if (blockNode->next && blockNode->next->header->is_free) {
        blockHeadr->size += blockNode->next->header->size + sizeof(hed_r) + sizeof(node);
        blockNode->next = blockNode->next->next;//linked
        if (blockNode->next) {//updating linked
            blockNode->next->prev = blockNode;
        }
    }

    // coalesce with previous block if free
    if (blockNode->prev && blockNode->prev->header->is_free) {
        blockNode->prev->header->size += blockHeadr->size + sizeof(hed_r) + sizeof(node);
        blockNode->prev->next = blockNode->next;
        if (blockNode->next) {
            blockNode->next->prev = blockNode->prev;
        }
    }

    return 0;
}
void umemdump() {
    node *curr = freeList; 
    printf("current free list:\n");
    while (curr != NULL) {
        printf("Free block: Address=%p, Size=%zu, Is_Free=%d\n",
               (void *)curr, curr->header->size, curr->header->is_free);
        curr = curr->next;
    }
}


