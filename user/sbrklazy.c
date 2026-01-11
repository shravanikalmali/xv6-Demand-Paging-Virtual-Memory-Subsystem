#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int main(void) {
    printf("sbrk_lazy_test starting...\n");

    // Allocate 10 pages of memory, but don't use it yet.
    char *mem = sbrk(10 * PGSIZE);
    if (mem == (char*)-1) {
        fprintf(2, "sbrk failed\n");
        exit(1);
    }
    
    printf("sbrk(10 * PGSIZE) returned %p. No page faults should have occurred yet.\n", mem);
    printf("Press any key to access the 5th new page...\n");
    
    // This will block until you type something in the other QEMU window
    char buf[1];
    read(0, buf, 1);

    // Now, access a page in the middle of the allocated region.
    // This should trigger the first page fault for the heap.
    printf("Accessing address %p...\n", mem + 5 * PGSIZE);
    mem[5 * PGSIZE] = 'A';
    
    printf("Successfully wrote to the page. Check kernel logs for a page fault.\n");
    printf("sbrk_lazy_test finished.\n");
    
    exit(0);
}

