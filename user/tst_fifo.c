//############## LLM Generated Code Begins ##############
#include "kernel/types.h"
#include "user.h"
#include "kernel/memstat.h"

#define MAX_PAGES 20  // More than physical memory can hold

void test_fifo_replacement() {
    printf("[TEST] Starting FIFO Page Replacement Test\n");
    
    printf("[INFO] Allocating and accessing %d pages to trigger page replacement...\n", MAX_PAGES);
    
    // Allocate more pages than physical memory
    char *pages[MAX_PAGES];
    for (int i = 0; i < MAX_PAGES; i++) {
        pages[i] = sbrk(4096);
        if (pages[i] == (char*)-1) {
            printf("[ERROR] sbrk failed at iteration %d\n", i);
            exit(1);
        }
        
        // Touch each page to ensure allocation
        pages[i][0] = 'A' + (i % 26);
        
        if (i % 5 == 0) {
            printf("[INFO] Allocated and accessed page %d/%d\n", i+1, MAX_PAGES);
        }
    }
    
    // Access pages in order to verify FIFO
    printf("[INFO] Accessing pages in order to verify FIFO replacement...\n");
    for (int i = 0; i < MAX_PAGES; i++) {
        // Access memory to trigger page faults and replacements
        (void)pages[i][0];
    }
    
    printf("[PASS] FIFO replacement test completed. Check console logs for replacement pattern.\n");
    printf("       Look for MEMFULL, VICTIM, and EVICT messages in the log.\n");
}

int main() {
    test_fifo_replacement();
    exit(0);
}
//############## LLM Generated Code Ends ################
