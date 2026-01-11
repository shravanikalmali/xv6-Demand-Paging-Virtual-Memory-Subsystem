#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEPTH 500 // Should be deep enough to cross a page boundary

void deep_recursion(int count) {
    volatile char buffer[100]; // Allocate some space on the stack
    buffer[0] = 'a'; // Touch it to ensure the stack page is faulted in

    // This line tells the compiler we intentionally aren't using the buffer variable further.
    // It silences the "unused variable" warning.
    (void)buffer; 

    if (count > DEPTH) {
        printf("Reached max recursion depth.\n");
        return;
    }
    if (count % 100 == 0) {
        printf("Recursion depth: %d\n", count);
    }
    deep_recursion(count + 1);
}

int main(void) {
    printf("stack_growth_test starting...\n");
    printf("This will test on-demand stack allocation via deep recursion.\n");

    deep_recursion(0);

    printf("stack_growth_test finished.\n");
    exit(0);
}

