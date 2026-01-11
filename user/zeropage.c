#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int main(void) {
    printf("zero_page_test starting...\n");
    
    char *mem = sbrk(PGSIZE);
    if (mem == (char*)-1) {
        fprintf(2, "sbrk failed\n");
        exit(1);
    }

    // This read will trigger a page fault, and the kernel should provide a zeroed page.
    int is_zero = 1;
    for (int i = 0; i < PGSIZE; i++) {
        if (mem[i] != 0) {
            is_zero = 0;
            break;
        }
    }

    if (is_zero) {
        printf("TEST PASSED: Heap page was zero-filled.\n");
    } else {
        fprintf(2, "TEST FAILED: Heap page was NOT zero-filled.\n");
    }

    exit(0);
}

