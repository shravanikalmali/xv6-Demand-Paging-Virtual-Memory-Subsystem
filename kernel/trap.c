//############## LLM Generated Code Begins ##############

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from, and returns to, trampoline.S
// return value is user satp for trampoline.S to switch to.
//
uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);  //DOC: kernelvec

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if((r_scause() == 12 || r_scause() == 13 || r_scause() == 15)) {
    // Page faults: 12=Instruction, 13=Load, 15=Store
    uint64 va = r_stval();
    va = PGROUNDDOWN(va);
    uint64 *pte = walk(p->pagetable, va, 0);

    // Determine access type
    char *access_type = (r_scause() == 12) ? "exec" : 
                        (r_scause() == 13) ? "read" : "write";

    // Check if page is on disk (swapped out)
    if(pte != 0 && (*pte & PTE_V) == 0 && (*pte & PTE_S) != 0) {
      // --- 1. HANDLE SWAP-IN ---
      printf("[pid %d] PAGEFAULT va=0x%lx access=%s cause=swap\n", p->pid, va, access_type);
      
      int slot = PTE_SLOT(*pte);
      uint64 perms = *pte & (PTE_R | PTE_W | PTE_X | PTE_U);

      printf("[pid %d] SWAPIN va=0x%lx slot=%d\n", p->pid, va, slot);

      char *mem = kalloc();
      if(mem == 0) {
        // kalloc failed - kill process
        setkilled(p);
      } else {
        // Read page from swap file
        if(p->swap_inode) {
          ilock(p->swap_inode);
          int n = readi(p->swap_inode, 0, (uint64)mem, (uint64)slot * PGSIZE, PGSIZE);
          iunlock(p->swap_inode);
          
          if(n != PGSIZE) {
            kfree(mem);
            setkilled(p);
          } else {
            // Free the swap slot now that data is in memory
            swap_slot_free(p, slot);

            // Clear the old PTE (which was SLOT | PERMS | PTE_S)
            *pte = 0;

            // Map the new page with original perms
            if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, perms | PTE_V) != 0) {
              kfree(mem);
              setkilled(p);
            } else {
              // Add to resident set (for FIFO replacement)
              add_to_resident_set(p, va, p->fifo_seq_num);
              printf("[pid %d] RESIDENT va=0x%lx seq=%d\n", p->pid, va, p->fifo_seq_num);
              p->fifo_seq_num++;
            }
          }
        } else {
          // No swap file - kill process
          kfree(mem);
          setkilled(p);
        }
      }
    } else if(pte == 0 || (*pte & PTE_V) != 0) {
      // --- 2. INVALID ACCESS or PAGE ALREADY MAPPED ---
      printf("[pid %d] PAGEFAULT va=0x%lx access=%s cause=invalid\n", p->pid, va, access_type);
      printf("[pid %d] KILL invalid-access va=0x%lx access=%s\n", p->pid, va, access_type);
      setkilled(p);
    } else {
      // --- 3. HANDLE REGULAR DEMAND PAGING (fresh page) ---
      // Determine cause
      char *cause = "heap";  // default
      if(va >= p->exe_end && va < p->sz) {
        cause = "heap";
      } else if(va >= p->trapframe->sp && va < p->trapframe->sp + PGSIZE) {
        cause = "stack";
      } else if(va < p->exe_end) {
        cause = "exec";
      }
      
      printf("[pid %d] PAGEFAULT va=0x%lx access=%s cause=%s\n", p->pid, va, access_type, cause);
      
      // Page is not swapped and not mapped, do normal demand paging
      if(vmfault(p->pagetable, va, (r_scause() == 13)? 1 : 0) == 0) {
        setkilled(p);
      }
    }
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}

//
// set up trapframe and control registers for a return to user space
//
void
prepare_return(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(). because a trap from kernel
  // code to usertrap would be a disaster, turn off interrupts.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

//############## LLM Generated Code Ends ################

