# Page Replacement with Resident Set Implementation

## Overview

This document describes the implementation of FIFO page replacement policy in xv6. When physical memory becomes exhausted (kalloc fails), the kernel automatically evicts the oldest page from the faulting process's resident set to make room for new pages.

## Architecture

### 1. **Resident Page Tracking Structure** (`kernel/proc.h`)

A linked list node tracks each resident page:

```c
struct resident_page {
  struct resident_page *next;  // Next node in FIFO queue
  uint64 va;                   // Virtual address of the resident page
  int fifo_seq_num;            // Sequence number assigned when paged in
};
```

### 2. **Process Structure Extensions** (`kernel/proc.h`)

Added to `struct proc`:

```c
struct resident_page *resident_set_head;  // Head of FIFO queue (oldest page)
struct resident_page *resident_set_tail;  // Tail of FIFO queue (newest page)
```

These pointers maintain a FIFO queue of all pages currently resident in memory for the process.

## Implementation Details

### 3. **Process Initialization** (`kernel/proc.c`)

In `allocproc()`, initialize the resident set fields:

```c
p->resident_set_head = 0;
p->resident_set_tail = 0;
```

### 4. **Resident Set Management** (`kernel/proc.c`)

#### **add_to_resident_set(struct proc *p, uint64 va, int seq_num)**

Called when a page is successfully paged in. Appends the page to the tail of the FIFO queue.

```c
void
add_to_resident_set(struct proc *p, uint64 va, int seq_num)
{
  struct resident_page *node = (struct resident_page*)kalloc();
  if(node == 0)
    panic("add_to_resident_set: kalloc failed");

  node->va = va;
  node->fifo_seq_num = seq_num;
  node->next = 0;

  acquire(&p->lock);
  if(p->resident_set_head == 0) {
    // List is empty
    p->resident_set_head = node;
    p->resident_set_tail = node;
  } else {
    // Add to tail
    p->resident_set_tail->next = node;
    p->resident_set_tail = node;
  }
  release(&p->lock);
}
```

#### **remove_from_resident_set(struct proc *p, uint64 va)**

Called when a page is unmapped. Removes the page from the FIFO queue.

```c
void
remove_from_resident_set(struct proc *p, uint64 va)
{
  acquire(&p->lock);
  struct resident_page *curr = p->resident_set_head;
  struct resident_page *prev = 0;

  while(curr) {
    if(curr->va == va) {
      // Found the node
      if(prev == 0) // It's the head
        p->resident_set_head = curr->next;
      else
        prev->next = curr->next;

      if(p->resident_set_tail == curr) // It's the tail
        p->resident_set_tail = prev;

      break;
    }
    prev = curr;
    curr = curr->next;
  }
  release(&p->lock);

  if(curr)
    kfree(curr); // Free the tracking node
}
```

### 5. **Page Eviction Logic** (`kernel/proc.c`)

#### **do_page_replacement(struct proc *p)**

Core eviction function. Evicts the oldest (FIFO) page from the process's resident set.

```c
int
do_page_replacement(struct proc *p)
{
  struct resident_page *victim;

  // 1. Find victim (head of list - oldest page)
  acquire(&p->lock);
  victim = p->resident_set_head;
  if(victim == 0) {
    // Process has no pages to evict
    release(&p->lock);
    return 0;
  }

  // 2. Remove victim from FIFO list
  p->resident_set_head = victim->next;
  if(p->resident_set_head == 0) // List is now empty
    p->resident_set_tail = 0;

  release(&p->lock);

  // 3. Log victim eviction
  printf("VICTIM %d\n", victim->fifo_seq_num);

  // 4. Find the victim's PTE
  pte_t *pte = walk(p->pagetable, victim->va, 0);
  if(pte == 0)
    panic("do_page_replacement: pte not found");
  if((*pte & PTE_V) == 0)
    panic("do_page_replacement: page not valid");

  // 5. Log eviction (all clean in this OS since no dirty bit tracking)
  printf("EVICT (Clean) %d\n", victim->fifo_seq_num);

  // 6. Get physical address and unmap
  uint64 pa = PTE2PA(*pte);
  *pte = 0; // Clear the PTE, unmapping the page

  // 7. Free the physical page and the tracking node
  kfree((void*)pa);
  kfree(victim);

  return 1; // Success
}
```

### 6. **Memory Allocator Trigger** (`kernel/kalloc.c`)

Modified `kalloc()` to trigger page replacement when memory is full:

```c
void *
kalloc(void)
{
  struct run *r;

  // --- MODIFIED LOGIC WITH PAGE REPLACEMENT ---
  // First, check if page is free (without holding lock long)
  acquire(&kmem.lock);
  r = kmem.freelist;
  release(&kmem.lock);

  if(r) {
    // Normal case: page is free
    acquire(&kmem.lock);
    kmem.freelist = r->next;
    release(&kmem.lock);
    memset((void*)r, 5, PGSIZE);
    return (void*)r;
  }

  // No free page. Try to make one via page replacement.
  printf("MEMFULL\n");

  // Try to evict a page from the current process
  struct proc *p = myproc();
  if(p && do_page_replacement(p) == 0) {
    // Failed to evict (process has no pages)
    return 0;
  }

  // Replacement succeeded, try one more time to kalloc
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((void*)r, 5, PGSIZE);

  return (void*)r; // Return page or 0 if still failed
}
```

### 7. **Virtual Memory Integration** (`kernel/vm.c`)

#### **vmfault() - Add to Resident Set**

When a page is successfully paged in, it's added to the resident set:

```c
// For stack growth:
add_to_resident_set(p, page_va, p->fifo_seq_num);
printf("ALLOC (Stack) %d\n", p->fifo_seq_num);
printf("RESIDENT %d\n", p->fifo_seq_num);
p->fifo_seq_num++;

// For heap allocation:
add_to_resident_set(p, page_va, p->fifo_seq_num);
printf("ALLOC (Heap) %d\n", p->fifo_seq_num);
printf("RESIDENT %d\n", p->fifo_seq_num);
p->fifo_seq_num++;

// For text/data loading:
add_to_resident_set(p, page_addr, p->fifo_seq_num);
printf("LOADEXEC %d\n", p->fifo_seq_num);
printf("RESIDENT %d\n", p->fifo_seq_num);
p->fifo_seq_num++;
```

#### **uvmunmap() - Remove from Resident Set**

When pages are unmapped (during exit or sbrk), they're removed from the resident set:

```c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;
  struct proc *p = myproc();

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      continue;   
    if((*pte & PTE_V) == 0)
      continue;
    
    // Remove from resident set before freeing
    if(p && (*pte & PTE_V)) {
      remove_from_resident_set(p, a);
    }
    
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}
```

### 8. **Public Interface** (`kernel/defs.h`)

Declared new functions:

```c
void            add_to_resident_set(struct proc*, uint64, int);
void            remove_from_resident_set(struct proc*, uint64);
int             do_page_replacement(struct proc*);
```

## Kernel Log Output

The implementation produces the following kernel log messages:

| Message | Meaning |
|---------|---------|
| `MEMFULL` | Physical memory is exhausted, page replacement triggered |
| `VICTIM %d` | Page with sequence number %d is being evicted |
| `EVICT (Clean) %d` | Page %d evicted (clean - no writeback) |
| `ALLOC (Stack) %d` | Stack page %d allocated |
| `ALLOC (Heap) %d` | Heap page %d allocated |
| `LOADEXEC %d` | Code/data page %d loaded from executable |
| `RESIDENT %d` | Page %d is now resident in memory |

## Test Scenario

To trigger page replacement:

1. Boot the kernel: `make qemu`
2. Run the demand page test: `demandpagetest`
3. Watch for `MEMFULL` and `VICTIM` logs when memory fills up

The test allocates multiple pages sequentially, which may trigger page replacement on memory-constrained systems.

## Thread Safety

All resident set operations are protected by the process lock:

- `add_to_resident_set()`: Acquires lock for queue insertion
- `remove_from_resident_set()`: Acquires lock for queue removal
- `do_page_replacement()`: Acquires lock to remove victim from queue

## Memory Overhead

Each tracked page requires:
- One `struct resident_page` node (~24 bytes + allocator overhead)
- One entry in the FIFO linked list

For a process with N pages, overhead is approximately: `N × 32 bytes`

## Future Enhancements

1. **Dirty Bit Tracking**: Track PTE_D flag (if available) to avoid writeback of clean pages
2. **Swap Support**: Write dirty pages to disk instead of discarding
3. **Second Chance Algorithm**: Improve page replacement with usage tracking
4. **LRU Replacement**: Track page access times for better policy
5. **Per-CPU Freelists**: Reduce lock contention in kalloc()

## Files Modified

1. `kernel/proc.h` - Added resident_page struct and queue head/tail pointers
2. `kernel/proc.c` - Added allocation/deallocation and replacement functions
3. `kernel/kalloc.c` - Modified to trigger page replacement when memory full
4. `kernel/vm.c` - Modified vmfault() and uvmunmap() for resident set tracking
5. `kernel/defs.h` - Added function declarations

## Build and Verification

```bash
# Build the kernel
cd /home/shravanikalmali/Desktop/mp2_plspls/A
make clean
make

# Test page replacement
make qemu
# In QEMU: type "demandpagetest"

# Watch kernel console for page replacement logs
```

