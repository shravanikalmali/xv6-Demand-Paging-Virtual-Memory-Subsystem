//############## LLM Generated Code Begins ##############

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "kernel/memstat.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_memstat(void)
{
  uint64 user_info_ptr; // User-space pointer to the struct
  struct proc_mem_stat k_info; // Kernel-space copy
  struct proc *p = myproc();

  // 1. Get the user-space pointer argument
  argaddr(0, &user_info_ptr);

  // 2. Initialize the kernel-space struct
  memset(&k_info, 0, sizeof(k_info));
  k_info.pid = p->pid;
  k_info.next_fifo_seq = p->fifo_seq_num;
  k_info.num_pages_total = p->sz / PGSIZE;

  // 3. Loop through the process's virtual memory
  int page_count = 0;
  for(uint64 va = 0; va < p->sz && page_count < MAX_PAGES_INFO; va += PGSIZE) {
    struct page_stat *ps = &k_info.pages[page_count];
    pte_t *pte = walk(p->pagetable, va, 0);

    ps->va = va;
    ps->seq = -1;       // Default
    ps->swap_slot = -1; // Default

    if(pte == 0) {
      // Page not in page table at all
      ps->state = UNMAPPED;
    } else if((*pte & PTE_V) != 0) {
      // --- Page is RESIDENT ---
      ps->state = RESIDENT;
      k_info.num_resident_pages++;
      ps->is_dirty = (*pte & PTE_D) ? 1 : 0;
      ps->seq = find_seq_in_resident_set(p, va);

    } else if((*pte & PTE_S) != 0) {
      // --- Page is SWAPPED ---
      ps->state = SWAPPED;
      k_info.num_swapped_pages++;
      ps->is_dirty = 0; // Not resident, so not dirty in RAM
      ps->swap_slot = PTE_SLOT(*pte);

    } else {
      // --- Page is UNMAPPED ---
      // (e.g., allocated by sbrk but never faulted)
      ps->state = UNMAPPED;
      ps->is_dirty = 0;
    }

    page_count++;
  }

  // 4. Copy the completed kernel struct back to user space
  if(copyout(p->pagetable, user_info_ptr, (char *)&k_info, sizeof(k_info)) < 0) {
    return -1;
  }

  return 0;
}

//############## LLM Generated Code Ends ################

