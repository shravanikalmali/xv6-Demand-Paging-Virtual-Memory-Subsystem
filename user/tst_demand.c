//############## LLM Generated Code Begins ##############
#include "kernel/types.h"
#include "user.h"
#include "kernel/memstat.h"

void test_basic_demand_paging() {
    printf("[TEST] Starting Basic Demand Paging Test\n");
    
    // Allocate 4 pages but only access first and last
    char *mem = sbrk(4 * 4096);
    if (mem == (char*)-1) {
        printf("[ERROR] sbrk failed\n");
        exit(1);
    }
    
    printf("[INFO] Allocated 4 pages at %p\n", mem);
    
    // Access first page (should trigger ALLOC + RESIDENT)
    printf("[INFO] Accessing first page...\n");
    mem[0] = 'A';
    
    // Access last page (should trigger ALLOC + RESIDENT)
    printf("[INFO] Accessing last page...\n");
    mem[3*4096] = 'B';
    
    // Check memory statistics
    struct proc_mem_stat stat;
    if (memstat(&stat) < 0) {
        printf("[ERROR] memstat failed\n");
        exit(1);
    }
    
    printf("[INFO] Resident pages: %d (expected at least 2)\n", stat.num_resident_pages);
    
    if (stat.num_resident_pages >= 2) {
        printf("[PASS] Basic demand paging test passed!\n");
    } else {
        printf("[FAIL] Expected at least 2 resident pages\n");
    }
}

int main() {
    test_basic_demand_paging();
    exit(0);
}
//############## LLM Generated Code Ends ################
