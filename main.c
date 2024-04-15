#include "umem.c"
#include "umem.h"
#include <stdio.h>

void print_free_list(node_t *list) {
    while (list) {
        printf("Free block: Address=%p, Size=%zu, Is_Free=%d, Magic=0x%lx\n",
               (void *)list, list->header->size, list->header->is_free, list->header->magic);
        list = list->next;
    }
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

    // Additional tests can be performed here to ensure the allocator works

    return 0;
}

