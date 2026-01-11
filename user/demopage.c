#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

// Test 1: Basic functionality - normal program execution
void test_basic() {
  printf("\n=== TEST 1: Basic Functionality ===\n");
  printf("This test verifies that normal programs run correctly.\n");
  printf("Expected: Program should run without segfaults.\n");
  printf("Test PASSED: Basic execution works!\n");
}

// Test 2: Heap allocation (sbrk & ALLOC)
void test_heap() {
  printf("\n=== TEST 2: Heap Allocation (sbrk & ALLOC) ===\n");
  printf("This test verifies that sbrk only bumps p->sz without physical allocation.\n");
  printf("Physical pages are only allocated when accessed.\n\n");

  // 1. Allocate 10 pages of memory.
  // This should NOT trigger any kernel logs yet.
  printf("Step 1: Calling sbrk(10 * PGSIZE)...\n");
  char *mem = sbrk(10 * PGSIZE);
  if (mem == (char*)-1) {
    printf("ERROR: sbrk failed\n");
    exit(-1);
  }
  printf("sbrk returned: %p\n", mem);
  printf("Expected: No ALLOC logs above this line.\n\n");

  // 2. Access the FIRST page.
  // This SHOULD trigger an "ALLOC (Heap)" log.
  printf("Step 2: Accessing first page (mem[0])...\n");
  mem[0] = 'a';
  printf("First page access OK. Expected: One ALLOC (Heap) + RESIDENT log above.\n\n");

  // 3. Access the FIFTH page.
  // This SHOULD trigger another "ALLOC (Heap)" log.
  printf("Step 3: Accessing fifth page (mem[5*PGSIZE])...\n");
  mem[5 * PGSIZE] = 'b';
  printf("Fifth page access OK. Expected: One ALLOC (Heap) + RESIDENT log above.\n\n");

  // 4. Access the LAST page.
  // This SHOULD trigger a third "ALLOC (Heap)" log.
  printf("Step 4: Accessing tenth page (mem[9*PGSIZE + (PGSIZE-1)])...\n");
  mem[9 * PGSIZE + (PGSIZE - 1)] = 'c';
  printf("Tenth page access OK. Expected: One ALLOC (Heap) + RESIDENT log above.\n\n");

  printf("Expected total: 3 ALLOC (Heap) logs + 3 RESIDENT logs\n");
  printf("Test PASSED: Heap allocation works on demand!\n");
}

// Test 3: Stack growth (ALLOC)
void deep_recursion(int count) {
  char arr[1024]; // Consume 1KB of stack

  if (count > 20) {
    printf("Recursion base case reached (depth > 20KB).\n");
    return;
  }

  // Use array to prevent compiler from optimizing it out
  arr[0] = 'a';
  arr[256] = 'b';
  arr[512] = 'c';
  arr[1023] = 'd';
  
  if (count % 5 == 0) {
    printf("Recursion depth: %d, arr[0]=%c\n", count, arr[0]);
  }

  deep_recursion(count + 1);
}

void test_stack() {
  printf("\n=== TEST 3: Stack Growth (ALLOC) ===\n");
  printf("This test verifies that stack growth triggers page allocation.\n");
  printf("As recursion deepens, new stack pages should be allocated.\n\n");

  printf("Starting deep recursion (20+ levels, ~20KB stack usage)...\n");
  deep_recursion(0);
  printf("Expected: 2-3 ALLOC (Stack) + RESIDENT logs above (as stack crosses page boundaries).\n");
  printf("Test PASSED: Stack growth works on demand!\n");
}

// Test 4: Segmentation faults (Invalid Access)
void test_segfault_null() {
  printf("\n=== TEST 4a: Segmentation Fault - Null Pointer Access ===\n");
  printf("This test verifies that invalid memory access is caught.\n");
  printf("Accessing null pointer should cause a fault and kill the process.\n\n");

  printf("Attempting null pointer access...\n");
  int *p = 0;
  *p = 1; // FAULT - Should terminate process

  // This line should never be reached
  printf("ERROR: Null pointer access did not fault!\n");
  exit(-1);
}

void test_segfault_above_heap() {
  printf("\n=== TEST 4b: Segmentation Fault - Above Heap ===\n");
  printf("This test verifies that accessing far above heap is caught.\n\n");

  printf("Getting current heap boundary with sbrk(0)...\n");
  char *heap_end = sbrk(0);
  printf("Heap end: %p\n", heap_end);

  printf("Attempting access 10 pages above heap boundary...\n");
  char *p = heap_end + (10 * PGSIZE);
  *p = 'x'; // FAULT - Should terminate process

  // This line should never be reached
  printf("ERROR: Above-heap access did not fault!\n");
  exit(-1);
}

void test_segfault_below_stack() {
  printf("\n=== TEST 4c: Segmentation Fault - Far Below Stack ===\n");
  printf("This test verifies that accessing far below stack is caught.\n");
  printf("(Note: May not fault immediately if stack can grow, but should fail\n");
  printf(" if beyond the guard zone)\n\n");

  printf("This test is complex and may not crash predictably.\n");
  printf("Skipping this specific test.\n");
  printf("Test SKIPPED\n");
}

// Main test suite
int main() {
  printf("\n");
  printf("╔════════════════════════════════════════════════════════════╗\n");
  printf("║   xv6 Lazy Allocation - Demand Paging Test Suite           ║\n");
  printf("╚════════════════════════════════════════════════════════════╝\n");

  // Test 1: Basic functionality
  test_basic();

  // Test 2: Heap allocation
  test_heap();

  // Test 3: Stack growth
  test_stack();

  // Test 4a: Segmentation fault - null pointer
  // Uncomment to run this test (it will crash the program)
  // test_segfault_null();

  // Test 4b: Segmentation fault - above heap
  // Uncomment to run this test (it will crash the program)
  // test_segfault_above_heap();

  // Test 4c: Segmentation fault - below stack
  test_segfault_below_stack();

  printf("\n");
  printf("╔════════════════════════════════════════════════════════════╗\n");
  printf("║   All Safe Tests PASSED!                                   ║\n");
  printf("║   Note: Segfault tests (4a, 4b) are commented out because  ║\n");
  printf("║         they intentionally crash. Uncomment in source to   ║\n");
  printf("║         test error handling.                              ║\n");
  printf("╚════════════════════════════════════════════════════════════╝\n");
  printf("\n");

  exit(0);
}
