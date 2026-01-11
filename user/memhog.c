#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096
#define ONE_MB (1024 * 1024)

int main() {
  // xv6 has 128MB of RAM by default.
  // The kernel, init, and sh take some.
  // Let's try to access 150MB worth of pages.
  int total_pages = (150 * ONE_MB) / PGSIZE;
  char *mem;

  printf("Memory Hog Test: Starting...\n");
  printf("Attempting to access %d pages (%d MB)...\n", total_pages, total_pages * PGSIZE / ONE_MB);

  // 1. Allocate all the memory first.
  // This should NOT trigger kalloc or page replacement.
  mem = sbrk(total_pages * PGSIZE);
  if (mem == (char*)-1) {
    printf("sbrk failed. Not enough virtual address space?\n");
    exit(-1);
  }
  printf("sbrk(150MB) returned: %p\n", mem);
  printf("Now, writing to every page to force allocation...\n");

  // 2. Write to each page. This WILL trigger page faults.
  for (int i = 0; i < total_pages; i++) {
    // Write a value to the first byte of each page
    mem[i * PGSIZE] = (char)(i & 0xFF);

    if (i % 1000 == 0) {
      // Print progress
      printf("... accessed page %d\n", i);
    }
  }

  printf("All pages accessed.\n");

  // 3. Verify data to check for data loss (this is an advanced check).
  // If replacement is working (but swap isn't), pages will be
  // re-allocated as zero-filled.
  printf("Verifying data (expecting failures if dirty pages were evicted)...\n");
  int errors = 0;
  for (int i = 0; i < total_pages; i++) {
    char expected = (char)(i & 0xFF);
    if (mem[i * PGSIZE] != expected) {
      // This is expected! A dirty page was evicted, then
      // re-faulted, and came back as a new zeroed page.
      errors++;
    }
  }

  if (errors > 0) {
    printf("Test finished. Saw %d 'errors', which is GOOD.\n", errors);
    printf("It means dirty pages were evicted and re-faulted (as required).\n");
  } else {
    printf("Test finished. Saw 0 errors. Did replacement not happen?\n");
  }

  exit(0);
}
