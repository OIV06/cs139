#include <stdio.h>
#include "umem.h"
#include "umem.c"

int main() {
    // Test umeminit
    umeminit (4096, -1); 
    umemdump(); 
    umalloc(100);

    
}
