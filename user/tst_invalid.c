//############## LLM Generated Code Begins ##############

#include "kernel/types.h"
#include "user.h"

void test_invalid_access() {
    printf("[TEST] Starting Invalid Memory Access Test\n");
    
    printf("[INFO] Attempting to access NULL pointer...\n");
    printf("       This should trigger a page fault and terminate the process.\n");
    
    // Try to access NULL (should be invalid)
    volatile int *ptr = 0;
    *ptr = 42;  // This should trigger KILL invalid-access
    
    // Shouldn't reach here if invalid access handling works
    printf("[FAIL] Did not catch invalid memory access\n");
}

void test_stack_overflow() {
    printf("[TEST] Starting Stack Overflow Test\n");
    
    printf("[INFO] Creating deep recursion to cause stack overflow...\n");
    
    // This will cause stack growth beyond the allowed limit
    volatile char large_array[8192];  // Larger than one page
    large_array[0] = 'A';    // Touch the page
    (void)large_array;       // Prevent unused variable warning
    
    printf("[FAIL] Stack overflow not detected\n");
}

int main() {
    int pid = fork();
    if (pid < 0) {
        printf("[ERROR] fork failed\n");
        exit(1);
    }
    
    if (pid == 0) {
        // Child process - test NULL access
        test_invalid_access();
        exit(1);
    } else {
        // Parent process - wait for child
        int status;
        wait(&status);
        
        // Test stack overflow in parent
        test_stack_overflow();
    }
    
    exit(0);
}
//############## LLM Generated Code Ends ################

