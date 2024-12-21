// #include "spinlock.h" //extra credit
//  #include "param.h"     //extra credit
//  #include "memlayout.h" //extra credit

#define MAX_LOCKS 7              // extra credit-1
#define LOW_PRIORITY 1           // extra credit-1
#define HIGH_PRIORITY 10         // extra credit-1
#define PROC_TRACE_BUF_SIZE 4096 // project 2.6
#define TRACE_BUF_SIZE 4096      // project 2.6
// #define TRACE_NONE 0             // project 3.1
// #define TRACE_ALL 1              // project 3.1
// #define TRACE_ONCE 2             // project 3.1
// Per-CPU state
struct cpu
{
  uchar apicid;              // Local APIC ID
  struct context *scheduler; // swtch() here to enter scheduler
  struct taskstate ts;       // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS]; // x86 global descriptor table
  volatile uint started;     // Has the CPU started?
  int ncli;                  // Depth of pushcli nesting.
  int intena;                // Were interrupts enabled before pushcli?
  struct proc *proc;         // The process running on this cpu or null
};

int setnice(int pid, int value);
extern struct cpu cpus[NCPU];
extern int ncpu;
void init_locks(void); // extra credit-1
// struct proc *myproc(void); // project

// PAGEBREAK: 17
//  Saved registers for kernel context switches.
//  Don't need to save all the segment registers (%cs, etc),
//  because they are constant across kernel contexts.
//  Don't need to save %eax, %ecx, %edx, because the
//  x86 convention is that the caller has saved them.
//  Contexts are stored at the bottom of the stack they
//  describe; the stack pointer is the address of the context.
//  The layout of the context matches the layout of the stack in swtch.S
//  at the "Switch stacks" comment. Switch doesn't save eip explicitly,
//  but it is on the stack and allocproc() manipulates it.
struct context
{
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate
{
  UNUSED,
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
};

// // extra credit
// struct priority_lock
// {
//   int id;                            // Lock identifier
//   int locked;                        // Lock status (0: unlocked, 1: locked)
//   int owner_pid;                     // PID of the process holding the lock
//   int owner_original_nice;           // Original priority of the owner
//   struct proc *waiting_procs[NPROC]; // Array of pointers to waiting processes
// };
// // extra credit

// Per-process state
struct proc
{
  uint sz;                    // Size of process memory (bytes)
  pde_t *pgdir;               // Page table
  char *kstack;               // Bottom of kernel stack for this process
  enum procstate state;       // Process state
  int pid;                    // Process ID
  struct proc *parent;        // Parent process
  struct trapframe *tf;       // Trap frame for current syscall
  struct context *context;    // swtch() here to run process
  void *chan;                 // If non-zero, sleeping on chan
  int killed;                 // If non-zero, have been killed
  struct file *ofile[NOFILE]; // Open files
  struct inode *cwd;          // Current directory
  char name[16];              // Process name (debugging)
  int nice;                   // Add this line to store the nice value
  int original_nice;          // extra-task Original priority before inheritance
  int has_inherited;          // extra-task Flag to indicate if priority has been inherited
  // // extracredit-1
  int priority;
  int original_priority; // Original priority before inheritance
  // int base_priority;
  // // extra credit-1
  int tracing; // Project alternative
  // int syscall_mask; // project alternative
  char trace_buf[TRACE_BUF_SIZE];           // project 2.6
  char proc_trace_buf[PROC_TRACE_BUF_SIZE]; // project 2.6
  int trace_buf_index;                      // project 2.6
  int filtering;                            // 0 = no filter, 1 = filter active //project 3.1
  char filter_syscall[16];                  // project 3.1
  int success_only;                         // project 3.2
  int fail_only;                            // project 3.3
                                            //      int filter;                               // project 3.1
                                            //      int selective_tracing;                    // project 3.1
                                            //      int syscall_to_trace;                     // project 3.1
                                            //      int selective_trace_once;                 // project 3.1
                                            //      int trace_option;                         // project 3.1
                                            //      int trace_syscall_num;                    // project 3.1
                                            //      int trace_syscall;                        // project 3.1
                                            //      char syscall_filter[16];                  // project 3.1
                                            //      int traced_syscall_num;                   // project 3.1
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

// // extra credit-1

// #define NULL 0

// // Function prototypes
// struct proc *myproc(void);
// void setpriority(int pid, int priority); // Function to set process priority
// int getpriority(int pid);                // Function to get process priori
// struct lock_t
// {
//   int id;                // Lock identifier (0-6)
//   int locked;            // 0 = unlocked, 1 = locked
//   struct proc *owner;    // Process holding the lock
//   struct spinlock lk;    // Spinlock for synchronizing access to this lock
//   int original_priority; // Original priority before inheritance (-1 if not elevated)
// };

struct lock_t;
// {
//   int locked;         // Is the lock held?
//   int id;             // Lock identifier (1-7)
//   struct proc *owner; // Process holding the lock
//   struct spinlock lk; // Spinlock protecting this lock
// };

// extern struct lock_t locks[MAX_LOCKS];

void init_locks(void);
struct lock_t *get_lock(int id);
void acquire_lock(struct lock_t *lk);
void release_lock(struct lock_t *lk);

// // extra credit-1
// extern int strace_enabled; // project alternative