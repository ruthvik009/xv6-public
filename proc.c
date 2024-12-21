#include "types.h"
// #include "user.h" //project 3.1
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern uint ticks; // extra credit-1

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

// extra credit
// #define MAX_LOCKS 7 if extra credit-1 not works

// Define struct priority_lock
struct priority_lock
{
  int id;                  // Lock identifier
  int locked;              // Lock status (0: unlocked, 1: locked)
  int owner_pid;           // PID of the process holding the lock
  int owner_original_nice; // Original priority of the owner
};

// project 2.4
struct trace_event
{
  int pid;
  char name[16];
  char syscall_name[16];
  int retval;
};

// Initialize the buffer and index
// struct trace_event trace_buffer[TRACE_BUF_SIZE];
// int trace_buf_index = 0;
// int trace_buf_count = 0;
// project 2.4

// extra credit-1
struct lock_t
{
  int locked;         // Is the lock held?
  int id;             // Lock identifier (1-7)
  struct proc *owner; // Process holding the lock
  struct spinlock lk; // Spinlock protecting this lock
};
// extra credit-1

// Declare the locks array
// struct priority_lock locks[MAX_LOCKS]; ///if extra credit-1 not works
struct lock_t locks[MAX_LOCKS]; // extra credit-1

// void init_locks(void)
// {
//   int i;
//   for (i = 0; i < MAX_LOCKS; i++)
//   {
//     locks[i].id = i + 1; // IDs from 1 to 7
//     locks[i].locked = 0;
//     locks[i].owner_pid = -1;
//     locks[i].owner_original_nice = -1;
//   }
// } if extra credit-1 not works
// extra credit

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->nice = 3;
  // // extra credit-1
  p->priority = 5; // Default priority
  p->original_priority = 0;
  // p->base_priority = 5; // Default base priority (e.g., 5 for normal priority)
  // p->priority = p->base_priority;
  // // extra credit-1

  // project
  p->tracing = 0;                               // Initialize tracing flag
  p->trace_buf_index = 0;                       // project 2.6
  memset(p->trace_buf, 0, PROC_TRACE_BUF_SIZE); // project 2.6
  p->filtering = 0;                             // project 3.1
  p->filter_syscall[0] = '\0';                  // project 3.1
  p->success_only = 0;                          // project 3.2
  p->fail_only = 0;                             // project 3.3
  // p->filter = -1;                               // project 3.1
  // p->trace_syscall_num = -1;                    // project 3.1
  // p->trace_pid = 0;                             // project 3.1
  // p->selective_tracing = 0;    // project 3.1
  // p->syscall_to_trace = -1;    // project 3.1
  // p->selective_trace_once = 0; // project 3.1
  // p->traced_syscall_num = -1;  // project 3.1
  //                             //  memset(p->trace_buf, 0, PROC_TRACE_BUF_SIZE); // Clear trace buffer
  // memset(p->syscall_filter, 0, sizeof(p->syscall_filter)); // project 3.1
  // project

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  init_locks(); // extra credit-1
  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  np->tracing = curproc->tracing;                // project alternative
  np->trace_buf_index = 0;                       // project 2.6
  memset(np->trace_buf, 0, PROC_TRACE_BUF_SIZE); // project 2.6
  // np->filter = curproc->filter;                  // project 3.1
  // np->trace_syscall_num = curproc->trace_syscall_num; // project 3.1
  // np->selective_tracing = curproc->selective_tracing;       // project 3.1
  // np->syscall_to_trace = curproc->syscall_to_trace;         // project 3.1
  // np->selective_trace_once = curproc->selective_trace_once; // project 3.1
  // // safestrcpy(np->syscall_filter, curproc->syscall_filter, sizeof(np->syscall_filter)); // project 3.1
  // np->traced_syscall_num = curproc->traced_syscall_num; // project 3.1
  // // np->trace_pid = curproc->trace_pid;                 // project 3.1

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  // project 2.6
  //  Before freeing the process, print the trace buffer
  // Print the per-process trace buffer if tracing is enabled
  // if (curproc->tracing && curproc->trace_buf_index > 0)
  // {
  //   // Ensure the buffer is null-terminated
  //   if (curproc->trace_buf_index < PROC_TRACE_BUF_SIZE)
  //     curproc->trace_buf[curproc->trace_buf_index] = '\0';
  //   else
  //     curproc->proc_trace_buf[PROC_TRACE_BUF_SIZE - 1] = '\0';

  //   // Print a separator (optional)
  //   cprintf("\n--- Tracing Output for PID %d ---\n", curproc->pid);

  //   // Print the trace buffer
  //   cprintf("%s", curproc->trace_buf);
  // }
  // // project 2.6
  // memset(curproc->syscall_filter, 0, sizeof(curproc->syscall_filter)); // project 3.1
  // project 3.1
  // if (curproc->tracing && curproc->trace_buf_index > 0 && strcmp(curproc->name, "sh") != 0)
  // {
  //   // Ensure the buffer is null-terminated
  //   if (curproc->trace_buf_index < PROC_TRACE_BUF_SIZE)
  //     curproc->trace_buf[curproc->trace_buf_index] = '\0';
  //   else
  //     curproc->trace_buf[PROC_TRACE_BUF_SIZE - 1] = '\0';

  //   // Print a separator (optional)
  //   cprintf("\n--- Tracing Output for PID %d ---\n", curproc->pid);

  //   // Print the trace buffer
  //   cprintf("%s", curproc->trace_buf);
  // }
  // project 3.1

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}

// PAGEBREAK: 42
//  Per-CPU process scheduler.
//  Each CPU calls scheduler() after setting itself up.
//  Scheduler never returns.  It loops, doing:
//   - choose a process to run
//   - swtch to start running that process
//   - eventually that process transfers control
//       via swtch back to the scheduler.
// void scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();
//   c->proc = 0;

//   for (;;)
//   {
//     // Enable interrupts on this processor.
//     sti();

//     // Loop over process table looking for process to run.
//     acquire(&ptable.lock);
//     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     {
//       if (p->state != RUNNABLE)
//         continue;

//       // Switch to chosen process.  It is the process's job
//       // to release ptable.lock and then reacquire it
//       // before jumping back to us.
//       c->proc = p;
//       switchuvm(p);
//       p->state = RUNNING;

//       swtch(&(c->scheduler), p->context);
//       switchkvm();

//       // Process is done running for now.
//       // It should have changed its p->state before coming back.
//       c->proc = 0;
//     }
//     release(&ptable.lock);
//   }
// }
// void scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu(); // Get the current CPU
//   c->proc = 0;

// #if PRIORITY_SCHEDULER
//   struct proc *highest_p;
//   int highest_priority;
// #endif

//   for (;;)
//   {
//     sti(); // Enable interrupts on this processor

//     acquire(&ptable.lock);

// #if PRIORITY_SCHEDULER
//     highest_p = 0;
//     highest_priority = 6; // Set to one higher than the lowest priority

//     // Loop over the process table to find the highest priority RUNNABLE process
//     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     {
//       if (p->state != RUNNABLE)
//         continue;
//       if (p->nice < highest_priority)
//       {
//         highest_priority = p->nice;
//         highest_p = p;
//       }
//     }

//     if (highest_p)
//     {
//       // Run the highest priority process
//       p = highest_p;
//       c->proc = p;
//       switchuvm(p);
//       p->state = RUNNING;

//       swtch(&c->scheduler, p->context);

//       switchkvm();
//       c->proc = 0;
//     }
// #else
//     // Original Round Robin scheduler
//     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     {
//       if (p->state != RUNNABLE)
//         continue;

//       c->proc = p;
//       switchuvm(p);
//       p->state = RUNNING;

//       swtch(&c->scheduler, p->context);

//       switchkvm();
//       c->proc = 0;
//     }
// #endif

//     release(&ptable.lock);
//   }
// } if extra credit-1 doesnot work

// extra credit-1
// proc.c
void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu(); // Get the current CPU
  c->proc = 0;

#if PRIORITY_SCHEDULER
  struct proc *highest_p;
  int highest_priority;
#endif

  for (;;)
  {
    sti(); // Enable interrupts on this processor

    acquire(&ptable.lock);

#if PRIORITY_SCHEDULER
    highest_p = 0;
    highest_priority = 6; // Set to one higher than the lowest priority

    // Loop over the process table to find the highest priority RUNNABLE process
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;
      if (p->nice < highest_priority)
      {
        highest_priority = p->nice;
        highest_p = p;
      }
    }

    if (highest_p)
    {
      // Run the highest priority process
      p = highest_p;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&c->scheduler, p->context);

      switchkvm();
      c->proc = 0;
    }
#else
    // Original Round Robin scheduler
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&c->scheduler, p->context);

      switchkvm();
      c->proc = 0;
    }
#endif

    release(&ptable.lock);
  }
}

// extra credit-1

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  acquire(&ptable.lock); // DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock)
  {                        // DOC: sleeplock0
    acquire(&ptable.lock); // DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { // DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// PAGEBREAK: 36
//  Print a process listing to console.  For debugging.
//  Runs when user types ^P on console.
//  No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// adding this

// proc.c
int setnice(int pid, int value)
{
  struct proc *p;
  int old_value = -1;

  acquire(&ptable.lock);
  if (pid == 0)
  {
    // Change the nice value of the current process
    p = myproc();
    old_value = p->nice;
    p->nice = 3;
  }
  else
  {
    // Find the process with the given pid
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->pid == pid)
      {
        old_value = p->nice;
        p->nice = value;
        break;
      }
    }
  }
  release(&ptable.lock);

  if (old_value == -1)
    return -1; // Process not found

  return old_value; // Return the old nice value
}

// extra credit

// int lock_resource(int id)
// {
//   struct proc *p = myproc();
//   struct priority_lock *lk;
//   struct proc *owner;
//   int print_priority_inversion = 0;
//   int inversion_owner_pid = 0;
//   int inversion_owner_nice = 0;
//   int acquired = 0;
//   int print_acquired_lock = 0;
//   int pid_acquired = 0;

//   // Validate lock ID
//   if (id < 1 || id > MAX_LOCKS)
//     return -1;

//   lk = &locks[id - 1];

//   acquire(&ptable.lock);

//   while (!acquired)
//   {
//     if (lk->locked == 0)
//     {
//       // Lock is free
//       lk->locked = 1;
//       lk->owner_pid = p->pid;
//       lk->owner_original_nice = p->nice;
//       acquired = 1;

//       // Collect data to print after releasing lock
//       print_acquired_lock = 1;
//       pid_acquired = p->pid;
//     }
//     else
//     {
//       // Lock is held, check for priority inversion
//       owner = 0;
//       struct proc *proc;
//       for (proc = ptable.proc; proc < &ptable.proc[NPROC]; proc++)
//       {
//         if (proc->pid == lk->owner_pid)
//         {
//           owner = proc;
//           break;
//         }
//       }
//       if (owner && owner->nice > p->nice)
//       {
//         // Priority inversion detected
//         if (!owner->has_inherited)
//         {
//           owner->original_nice = owner->nice;
//           owner->has_inherited = 1;
//         }
//         owner->nice = p->nice; // Elevate owner's priority

//         // Collect data to print after releasing lock
//         print_priority_inversion = 1;
//         inversion_owner_pid = owner->pid;
//         inversion_owner_nice = owner->nice;
//       }

//       // Sleep on the lock
//       sleep(lk, &ptable.lock); // Sleep until the lock is released

//       // When woken up, loop back to check if we can acquire the lock
//       // ptable.lock is reacquired by sleep
//     }
//   }

//   release(&ptable.lock);

//   // Now, after releasing the lock, we can call cprintf

//   if (print_priority_inversion)
//   {
//     cprintf("Priority inversion detected. Process %d inherits priority %d\n", inversion_owner_pid, inversion_owner_nice);
//   }

//   if (print_acquired_lock)
//   {
//     cprintf("Process %d acquired lock %d\n", pid_acquired, id);
//   }

//   return 0;
// }

// int release_resource(int id)
// {
//   struct proc *p = myproc();
//   int restored_priority = 0;
//   int restored_nice = 0;
//   int pid_released = p->pid;

//   // Validate lock ID
//   if (id < 1 || id > MAX_LOCKS)
//     return -1;

//   struct priority_lock *lk = &locks[id - 1];

//   acquire(&ptable.lock);

//   if (lk->locked == 0 || lk->owner_pid != p->pid)
//   {
//     // Lock is not held by this process
//     release(&ptable.lock);
//     return -1;
//   }

//   // Release the lock
//   lk->locked = 0;
//   lk->owner_pid = -1;

//   // Restore original priority if it was inherited
//   if (p->has_inherited)
//   {
//     restored_priority = p->pid;
//     restored_nice = p->original_nice;

//     p->nice = p->original_nice;
//     p->has_inherited = 0;
//   }

//   // Wake up any processes sleeping on the lock
//   wakeup1(lk); // Use wakeup1() instead of wakeup()

//   release(&ptable.lock);

//   // Print messages after releasing the lock
//   if (restored_priority)
//   {
//     cprintf("Process %d restored priority to %d\n", restored_priority, restored_nice);
//   }

//   cprintf("Process %d released lock %d\n", pid_released, id);
//   return 0;
// }

// extra credit

// // extra credit-1
// void setpriority(int pid, int priority)
// {
//   struct proc *p;

//   if (priority < 1 || priority > 5)
//     return; // Invalid priority range

//   acquire(&ptable.lock);
//   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//   {
//     if (p->pid == pid)
//     {
//       p->base_priority = priority;
//       // Only update effective priority if it's not elevated due to inheritance
//       if (p->priority == p->base_priority)
//         p->priority = priority;
//       break;
//     }
//   }
//   release(&ptable.lock);
// }

// int getpriority(int pid)
// {
//   struct proc *p;
//   int priority = -1;

//   acquire(&ptable.lock);
//   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//   {
//     if (p->pid == pid)
//     {
//       priority = p->priority;
//       break;
//     }
//   }
//   release(&ptable.lock);

//   return priority;
// }

void init_locks(void)
{
  int i;
  for (i = 0; i < MAX_LOCKS; i++)
  {
    locks[i].locked = 0;
    locks[i].id = i + 1;
    locks[i].owner = 0;
    initlock(&locks[i].lk, "userlock");
  }
}

struct lock_t *
get_lock(int id)
{
  if (id < 1 || id > MAX_LOCKS)
    return 0;
  return &locks[id - 1];
}

void acquire_lock(struct lock_t *lk)
{
  acquire(&lk->lk);
  while (lk->locked)
  {
    // Priority inheritance
    if (lk->owner && lk->owner->priority > myproc()->priority)
    {
      if (lk->owner->original_priority == 0)
      {
        lk->owner->original_priority = lk->owner->priority;
        cprintf("[%d ticks] Priority inversion detected. Priority Inheritance is Implemented: PID %d priority elevated from %d to %d\n",
                ticks, lk->owner->pid, lk->owner->priority, myproc()->priority);
      }
      lk->owner->priority = myproc()->priority;
    }
    // Sleep and release lk->lk; it will be re-acquired upon waking up
    sleep(lk, &lk->lk);
    // Do not re-acquire lk->lk here; sleep() already does it
  }
  lk->locked = 1;
  lk->owner = myproc();
  cprintf("[%d ticks] PID %d acquired lock %d\n", ticks, myproc()->pid, lk->id);
  release(&lk->lk);
}

void release_lock(struct lock_t *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  if (lk->owner)
  {
    if (lk->owner->original_priority != 0)
    {
      cprintf("[%d ticks] PID %d priority restored to %d\n Priority Inheritance Completed\n",
              ticks, lk->owner->pid, lk->owner->original_priority);
      lk->owner->priority = lk->owner->original_priority;
      lk->owner->original_priority = 0;
    }
    lk->owner = 0;
  }
  wakeup(lk);
  release(&lk->lk);
}
// // extra credit-1