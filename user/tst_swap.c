//############## LLM Generated Code Begins ##############

#include "kernel/types.h"
#include "user.h"
#include "kernel/memstat.h"

#define PAGES_TO_SWAP 10

void test_swapping() {
    printf("[TEST] Starting Swapping Test\n");
    
    printf("[INFO] Allocating and dirtying %d pages to force swapping...\n", PAGES_TO_SWAP);
    
    // Allocate and touch pages to force swapping
    char *pages[PAGES_TO_SWAP];
    for (int i = 0; i < PAGES_TO_SWAP; i++) {
        pages[i] = sbrk(4096);
        if (pages[i] == (char*)-1) {
            printf("[ERROR] sbrk failed at iteration %d\n", i);
            exit(1);
        }
        
        // Write to make pages dirty
        for (int j = 0; j < 10; j++) {
            pages[i][j] = 'A' + (i % 26);
        }
        
        printf("[INFO] Allocated and dirtied page %d/%d\n", i+1, PAGES_TO_SWAP);
    }
    
    printf("[INFO] Accessing pages to trigger swap-in...\n");
    
    // Access pages again to trigger swap-in
    for (int i = 0; i < PAGES_TO_SWAP; i++) {
        char c = pages[i][0];
        printf("[INFO] Read from page %d: %c\n", i, c);
    }
    
    printf("[PASS] Swapping test completed. Check console logs for swap operations.\n");
    printf("       Look for SWAPOUT and SWAPIN messages in the log.\n");
}

int main() {
    test_swapping();
    exit(0);
}
//############## LLM Generated Code Ends ################
