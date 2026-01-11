# Page Replacement Implementation - Quick Reference

## Summary of Changes

### What Was Added?

A FIFO page replacement system that evicts the oldest page when memory is full.

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `resident_page` struct | `kernel/proc.h` | FIFO node tracking virtual address and sequence number |
| `resident_set_head/tail` | `kernel/proc.h` | Queue pointers in process structure |
| `add_to_resident_set()` | `kernel/proc.c` | Add page to FIFO queue when paged in |
| `remove_from_resident_set()` | `kernel/proc.c` | Remove page from queue when unmapped |
| `do_page_replacement()` | `kernel/proc.c` | Evict oldest page, free it, log action |
| Modified `kalloc()` | `kernel/kalloc.c` | Call page replacement when memory full |
| Modified `vmfault()` | `kernel/vm.c` | Track pages when they're paged in |
| Modified `uvmunmap()` | `kernel/vm.c` | Remove pages from tracking when freed |

### Kernel Log Messages

When you run `demandpagetest` and trigger page replacement:

```
ALLOC (Stack) 5      # Stack page allocated, sequence #5
RESIDENT 5           # Page 5 now resident
ALLOC (Heap) 6       # Heap page allocated, sequence #6
RESIDENT 6           # Page 6 now resident
MEMFULL              # Out of memory!
VICTIM 5             # Evicting sequence #5 (oldest)
EVICT (Clean) 5      # Page 5 evicted (clean)
ALLOC (Heap) 7       # New heap page allocated, sequence #7
RESIDENT 7           # Page 7 now resident
```

### FIFO Policy

Pages are evicted in the order they were paged in:
- **Newest pages**: Added to tail of queue
- **Oldest pages**: Sit at head of queue
- **On eviction**: Head (oldest) removed first

```
[Oldest] -> Page 1 -> Page 2 -> Page 3 -> [Newest]
                                 ^
                            Added here
                          Removed from head
```

### Lock Safety

All queue operations are synchronized with `p->lock`:
- Prevents concurrent modifications during page replacement
- Ensures consistent queue state during eviction

## Files Modified

```
kernel/proc.h         - Add struct resident_page and queue pointers (+8 lines)
kernel/proc.c         - Add 3 functions, init queue in allocproc (+120 lines)
kernel/kalloc.c       - Add page replacement logic (+25 lines)
kernel/vm.c           - Hook add/remove into vmfault/uvmunmap (+30 lines)
kernel/defs.h         - Add 3 function declarations (+3 lines)
```

## Testing Page Replacement

### Simple Test (No Memory Pressure)

```bash
make qemu
# In QEMU:
$ demandpagetest
```

Expected: All 3 tests pass, NO page replacement logs (plenty of memory)

### Advanced Test (Trigger Replacement)

To actually see page replacement in action, you would need:
1. Reduce available memory (modify `param.h` PHYSTOP)
2. Create programs that allocate many pages
3. Watch for `MEMFULL` and `VICTIM` logs

### Compile Verification

```bash
# Clean build
make clean
make

# Check for errors
make 2>&1 | grep -i error

# Verify binaries exist
ls -lh kernel/kernel user/_demandpagetest fs.img
```

## Architecture Flow

```
User Program
    |
    v
Page Fault (va)
    |
    v
vmfault() 
    |
    +---> Allocate page: kalloc()
    |         |
    |         v
    |      Memory available?
    |         |
    |         +---> YES: Return page
    |         |
    |         +---> NO: Call do_page_replacement()
    |                    |
    |                    v
    |                 Log: MEMFULL
    |                 Log: VICTIM #X
    |                 Find oldest page in resident set
    |                 Remove from queue
    |                 Unmap PTE
    |                 Free physical memory
    |                 Log: EVICT (Clean)
    |                 Try kalloc() again
    |
    +---> Map page to VA
    |
    +---> Add to resident_set
    |
    +---> Log: RESIDENT #X
    |
    v
Resume Execution
```

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Allocate page | O(1) | Fast path if memory available |
| Page replacement | O(1) | Head pointer gives instant victim |
| Remove page | O(N) | Worst case if page is near tail, but typical N is small |
| Add page | O(1) | Always append to tail |

For small process memory sizes (typical <= 256 pages), all operations are effectively instant.

## Memory Overhead Per Process

- `resident_page` node: ~24 bytes per page + allocator overhead (~8 bytes)
- Total: ~32 bytes per resident page

Example: Process with 10 pages = ~320 bytes overhead

## Edge Cases Handled

1. **Empty resident set**: `do_page_replacement()` returns 0 (no pages to evict)
2. **Locked pages**: Kernel code/data not in resident set, not evicted
3. **Unmapping during exit**: `uvmunmap()` removes each page from queue
4. **Guard pages**: Not tracked (PTE_V check ensures only valid pages tracked)

## Next Steps

1. **Build**: `make` (done - 268K kernel)
2. **Test**: `make qemu` then `demandpagetest` at shell
3. **Monitor**: Watch for ALLOC/RESIDENT/MEMFULL/VICTIM logs
4. **Verify**: Tests should pass, no crashes, proper cleanup on exit

## Debugging Tips

If issues arise:

```bash
# Check kernel compilation
make clean && make 2>&1 | tail -20

# Verify all files in place
ls -lh kernel/{kernel,proc.o,vm.o,kalloc.o}

# Check test compiles
make user/_demandpagetest 2>&1 | tail -5

# See full kernel output in QEMU
make qemu | tee qemu_output.txt
```

