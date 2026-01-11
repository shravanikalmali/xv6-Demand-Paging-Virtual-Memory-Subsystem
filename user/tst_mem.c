//############## LLM Generated Code Begins ##############

#include "kernel/types.h"
#include "user.h"
#include "kernel/memstat.h"

void test_memstat() {
    printf("[TEST] Starting Memory Statistics Test\n");
    
    struct proc_mem_stat stat_before, stat_after;
    
    // Get initial stats
    printf("[INFO] Getting initial memory statistics...\n");
    if (memstat(&stat_before) < 0) {
        printf("[ERROR] memstat failed\n");
        exit(1);
    }
    
    printf("[INFO] Initial resident pages: %d\n", stat_before.num_resident_pages);
    
    // Allocate some memory
    printf("[INFO] Allocating a new page...\n");
    char *mem = sbrk(4096);
    if (mem == (char*)-1) {
        printf("[ERROR] sbrk failed\n");
        exit(1);
    }
    
    // Access the page to make it resident
    printf("[INFO] Accessing the new page...\n");
    mem[0] = 'X';
    
    // Get stats after allocation
    printf("[INFO] Getting updated memory statistics...\n");
    if (memstat(&stat_after) < 0) {
        printf("[ERROR] memstat failed\n");
        exit(1);
    }
    
    printf("[INFO] Resident pages after allocation: %d\n", stat_after.num_resident_pages);
    
    // Verify resident pages increased
    if (stat_after.num_resident_pages > stat_before.num_resident_pages) {
        printf("[PASS] Resident pages increased from %d to %d\n", 
               stat_before.num_resident_pages, stat_after.num_resident_pages);
    } else {
        printf("[FAIL] Resident pages did not increase after allocation\n");
    }
    
    // Print detailed page information if available
    if (stat_after.num_pages_total > 0) {
        printf("[INFO] Page information (first %d pages):\n", 
               stat_after.num_pages_total < 5 ? stat_after.num_pages_total : 5);
        
        int max_pages = stat_after.num_pages_total < 5 ? stat_after.num_pages_total : 5;
        for (int i = 0; i < max_pages; i++) {
            printf("  Page %d: va=0x%x, state=%s, dirty=%s, seq=%d\n", 
                   i, 
                   stat_after.pages[i].va,
                   stat_after.pages[i].state == RESIDENT ? "RESIDENT" : 
                   (stat_after.pages[i].state == SWAPPED ? "SWAPPED" : "UNMAPPED"),
                   stat_after.pages[i].is_dirty ? "yes" : "no",
                   stat_after.pages[i].seq);
        }
    }
}

int main() {
    test_memstat();
    exit(0);
}
//############## LLM Generated Code Ends ################
