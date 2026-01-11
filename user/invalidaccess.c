#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void) {
    printf("invalid_access_test starting.\n");

    // This address is far beyond the initial program size and stack.
    // This should be an invalid access.
    // Note: 0x1000000 is just an example of a high, unmapped address.
    int *p = (int *)0x1000000;

    printf("Attempting to access invalid address %p\n", p);
    *p = 123; // This should cause a page fault that the kernel deems invalid.

    // This line should never be reached. If it is, something is wrong.
    fprintf(2, "TEST FAILED: Process was not terminated!\n");

    exit(1); // Exit with a failure code if this line is reached.
}

