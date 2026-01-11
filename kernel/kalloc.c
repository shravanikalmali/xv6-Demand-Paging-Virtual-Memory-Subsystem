//############## LLM Generated Code Begins ##############

// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
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
  struct proc *p = myproc();
  if(p) {
    printf("[pid %d] MEMFULL\n", p->pid);
  } else {
    printf("MEMFULL\n");
  }

  // Try to evict a page from the current process
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

//############## LLM Generated Code Ends ################

