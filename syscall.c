#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "trace.h"    //project 2.4
#include "spinlock.h" //project 2.4
#include "fs.h"
// #include "strace.h" //project

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.

// project

// project 2.4
//  Initialize the buffer and index
struct trace_event trace_buffer[TRACE_BUF_SIZE];
int trace_buf_index = 0;
int trace_buf_count = 0; // Total events stored, max up to TRACE_BUF_SIZE
// extern int strcmp(const char *s1, const char *s2); // project 3.1

int filter = -1; // project 3.1

struct spinlock trace_lock; // For concurrency control

void trace_lock_init(void)
{
  initlock(&trace_lock, "trace_lock");
}
// project 2.4

// int strace_enabled = 0;

static char *syscall_names[] = {
    [SYS_fork] "fork",
    [SYS_exit] "exit",
    [SYS_wait] "wait",
    [SYS_pipe] "pipe",
    [SYS_read] "read",
    [SYS_kill] "kill",
    [SYS_exec] "exec",
    [SYS_fstat] "fstat",
    [SYS_chdir] "chdir",
    [SYS_dup] "dup",
    [SYS_getpid] "getpid",
    [SYS_sbrk] "sbrk",
    [SYS_sleep] "sleep",
    [SYS_uptime] "uptime",
    [SYS_open] "open",
    [SYS_write] "write",
    [SYS_mknod] "mknod",
    [SYS_unlink] "unlink",
    [SYS_link] "link",
    [SYS_mkdir] "mkdir",
    [SYS_close] "close",
    [SYS_trace] "trace",
    [SYS_get_trace] "get_trace",
    // [SYS_set_selective_trace] "set_selective_trace",
    // [SYS_set_selective_trace_once] "set_selective_trace_once",
    // [SYS_set_traced_syscall_num] "set_traced_syscall_num",
    // [SYS_opt_trace] "opt_trace",
};
// // project

int fetchint(uint addr, int *ip)
{
  struct proc *curproc = myproc();

  if (addr >= curproc->sz || addr + 4 > curproc->sz)
    return -1;
  *ip = *(int *)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  struct proc *curproc = myproc();

  if (addr >= curproc->sz)
    return -1;
  *pp = (char *)addr;
  ep = (char *)curproc->sz;
  for (s = *pp; s < ep; s++)
  {
    if (*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4 * n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int argptr(int n, char **pp, int size)
{
  int i;
  struct proc *curproc = myproc();

  if (argint(n, &i) < 0)
    return -1;
  if (size < 0 || (uint)i >= curproc->sz || (uint)i + size > curproc->sz)
    return -1;
  *pp = (char *)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int argstr(int n, char **pp)
{
  int addr;
  if (argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_nice(void);
extern int sys_set_priority(void);     // extra credit-1
extern int sys_lock(void);             // extra credit
extern int sys_release(void);          // extra credit
extern int sys_trace(void);            // project
extern int sys_get_trace(void);        // project 2.4
extern int sys_tracefilter(void);      // project 3.1
extern int sys_traceonlysuccess(void); // project 3.2
extern int sys_traceonlyfail(void);    // project 3.3
// extern int sys_trace_set_filter(void); // project 3.1
// extern int sys_set_selective_trace(void);      // project 3.1
// extern int sys_set_selective_trace_once(void); // project 3.1
// extern int sys_set_traced_syscall_num(void);   // project 3.1
// extern int sys_opt_trace(void);    // project 3.1
// extern int sys_setpriority(void); // extra credit-1
// extern int sys_getpriority(void); // extra credit-1

static int (*syscalls[])(void) = {
    [SYS_fork] sys_fork,
    [SYS_exit] sys_exit,
    [SYS_wait] sys_wait,
    [SYS_pipe] sys_pipe,
    [SYS_read] sys_read,
    [SYS_kill] sys_kill,
    [SYS_exec] sys_exec,
    [SYS_fstat] sys_fstat,
    [SYS_chdir] sys_chdir,
    [SYS_dup] sys_dup,
    [SYS_getpid] sys_getpid,
    [SYS_sbrk] sys_sbrk,
    [SYS_sleep] sys_sleep,
    [SYS_uptime] sys_uptime,
    [SYS_open] sys_open,
    [SYS_write] sys_write,
    [SYS_mknod] sys_mknod,
    [SYS_unlink] sys_unlink,
    [SYS_link] sys_link,
    [SYS_mkdir] sys_mkdir,
    [SYS_close] sys_close,
    [SYS_nice] sys_nice,
    [SYS_set_priority] sys_set_priority,         // extra credit-1
    [SYS_lock] sys_lock,                         // extra credit
    [SYS_release] sys_release,                   // extra credit
    [SYS_trace] sys_trace,                       // project
    [SYS_get_trace] sys_get_trace,               // project 2.4
    [SYS_tracefilter] sys_tracefilter,           // project 3.1
    [SYS_traceonlysuccess] sys_traceonlysuccess, // project 3.2
    [SYS_traceonlyfail] sys_traceonlyfail,       // project 3.3
                                                 //      [SYS_trace_set_filter] sys_trace_set_filter, // project 3.1
                                                 //       [SYS_set_selective_trace] sys_set_selective_trace,           // project 3.1
                                                 //       [SYS_set_selective_trace_once] sys_set_selective_trace_once, // project 3.1
                                                 //       [SYS_set_traced_syscall_num] sys_set_traced_syscall_num,     // project 3.1
                                                 //        [SYS_opt_trace] sys_opt_trace,       // project 3.1
                                                 //              [SYS_setpriority] sys_setpriority, // extra credit-1
                                                 //              [SYS_getpriority] sys_getpriority, // extra credit-1
};

// void syscall(void)
// {
//   int num;
//   struct proc *curproc = myproc();

//   num = curproc->tf->eax;
//   // project
//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {
//     // Print trace message before the system call for 'exit' and 'exec'
//     if (curproc->tracing)
//     {
//       if (num == SYS_exit || num == SYS_exec)
//       {
//         cprintf("TRACE: pid = %d | command_name = %s | syscall = %s\n",
//                 curproc->pid, curproc->name, syscall_names[num]);
//         // project2.4
//         //  Store the event in the buffer
//         // Store the event in the buffer
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = 0; // For 'exit' and 'exec', retval is not applicable
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//         // project2.4
//       }
//     }

//     int retval = syscalls[num]();

//     // Print trace message after the system call for other syscalls
//     if (curproc->tracing)
//     {
//       if (num != SYS_exit && num != SYS_exec && num != SYS_sbrk)
//       {
//         cprintf("TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
//                 curproc->pid, curproc->name, syscall_names[num], retval);
//         // project2.4
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = retval;
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);

//         // project2.4
//       }
//     }
//     curproc->tf->eax = retval;
//   }
//   // project
//   //  if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   //  {
//   //    curproc->tf->eax = syscalls[num]();
//   //  }
//   else
//   {
//     cprintf("%d %s: unknown sys call %d\n",
//             curproc->pid, curproc->name, num);
//     curproc->tf->eax = -1;
//   }
// } // if project 2.6 doesn't work

// void syscall(void)
// {
//   int num;
//   int retval;
//   struct proc *curproc = myproc();

//   num = curproc->tf->eax;

//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {
//     // Initialize retval
//     retval = -1;

//     // Handle special cases for 'exit' and 'exec' system calls

//     if (num == SYS_exit)
//     {
//       if (curproc->tracing)
//       {
//         // Since 'exit' doesn't return, set retval to 0 or appropriate value
//         retval = 0;
//         // Print trace message before the system call
//         cprintf("TRACE: pid = %d | command_name = %s | syscall = %s \n",
//                 curproc->pid, curproc->name, syscall_names[num]);

//         // Store the event in the buffer (Project 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
//                    sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = retval; // Include return value
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }

//       // Execute the system call
//       retval = syscalls[num]();

//       // Set the return value (although for 'exit' this may not have an effect)
//       curproc->tf->eax = retval;

//       // For 'exit', the process will terminate here
//       return; // Exit the syscall function
//     }

//     // Execute the system call for other syscalls
//     retval = syscalls[num]();

//     // Print trace message after the system call
//     if (curproc->tracing)
//     {
//       if (num != SYS_exit && num != SYS_sbrk)
//       {
//         cprintf("TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
//                 curproc->pid, curproc->name, syscall_names[num], retval);

//         // Store the event in the buffer (Project 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
//                    sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = retval; // Include return value
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }
//     }

//     // Set the return value of the system call
//     curproc->tf->eax = retval;
//   }
//   else
//   {
//     cprintf("%d %s: unknown sys call %d\n",
//             curproc->pid, curproc->name, num);
//     curproc->tf->eax = -1;
//   }
// }

// project 3.1

static int strcmp(const char *p, const char *q)
{
  while (*p && *p == *q)
  {
    p++;
    q++;
  }
  return (unsigned char)*p - (unsigned char)*q;
}

void syscall(void)
{
  int num;
  int retval;
  struct proc *curproc = myproc();

  num = curproc->tf->eax;

  if (num > 0 && num < NELEM(syscalls) && syscalls[num])
  {
    retval = -1;

    // Special handling for SYS_exit
    if (num == SYS_exit)
    {
      if (curproc->tracing)
      {
        // This is a special case: 'exit' we treat as retval=0 for trace consistency
        // Check filtering first:
        if (curproc->filtering && strcmp(syscall_names[num], curproc->filter_syscall) != 0)
        {
          // Not the filtered syscall, run exit silently
          retval = syscalls[num]();
          curproc->tf->eax = retval;
          return;
        }

        // success/fail checks:
        // 'exit' we consider retval=0 (successful) for printing consistency
        retval = 0;

        // If fail_only = 1 and retval=0 (not -1), skip printing:
        if (curproc->fail_only && retval != -1)
        {
          retval = syscalls[num]();
          curproc->tf->eax = retval;
          return;
        }

        // If success_only = 1 and retval=0 is fine (not -1), we print it
        // no extra check needed since it's successful

        cprintf("TRACE: pid = %d | command_name = %s | syscall = %s \n",
                curproc->pid, curproc->name, syscall_names[num]);

        acquire(&trace_lock);
        trace_buffer[trace_buf_index].pid = curproc->pid;
        safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
        safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
                   sizeof(trace_buffer[trace_buf_index].syscall_name));
        trace_buffer[trace_buf_index].retval = retval;
        trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
        if (trace_buf_count < TRACE_BUF_SIZE)
          trace_buf_count++;
        release(&trace_lock);
      }

      retval = syscalls[num]();
      curproc->tf->eax = retval;
      return;
    }

    // For other syscalls:
    retval = syscalls[num]();

    if (curproc->tracing)
    {
      // Check fail_only and success_only first
      // If fail_only is set and retval != -1, skip
      if (curproc->fail_only && retval != -1)
      {
        curproc->tf->eax = retval;
        return;
      }

      // If success_only is set and retval == -1, skip
      if (curproc->success_only && retval == -1)
      {
        curproc->tf->eax = retval;
        return;
      }

      // Check filtering next
      if (curproc->filtering && strcmp(syscall_names[num], curproc->filter_syscall) != 0)
      {
        // Not the filtered syscall, skip
        curproc->tf->eax = retval;
        return;
      }

      // If we reach here, it passes success/fail checks and filtering checks
      if (num != SYS_exit)
      {
        cprintf("TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
                curproc->pid, curproc->name, syscall_names[num], retval);

        acquire(&trace_lock);
        trace_buffer[trace_buf_index].pid = curproc->pid;
        safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
        safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
                   sizeof(trace_buffer[trace_buf_index].syscall_name));
        trace_buffer[trace_buf_index].retval = retval;
        trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
        if (trace_buf_count < TRACE_BUF_SIZE)
          trace_buf_count++;
        release(&trace_lock);
      }
    }

    curproc->tf->eax = retval;
  }
  else
  {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}

// project 3.1

// project 4
// project 4

// project 2.6
// void itoa(int n, char *str)
// {
//   int i = 0;
//   int is_negative = 0;
//   char temp[16];

//   if (n == 0)
//   {
//     str[0] = '0';
//     str[1] = '\0';
//     return;
//   }

//   if (n < 0)
//   {
//     is_negative = 1;
//     n = -n;
//   }

//   while (n != 0)
//   {
//     temp[i++] = (n % 10) + '0';
//     n /= 10;
//   }

//   if (is_negative)
//     temp[i++] = '-';

//   int j;
//   for (j = 0; j < i; j++)
//     str[j] = temp[i - j - 1];

//   str[j] = '\0';
// }

// void safe_strcat(char *dest, const char *src, int dest_size)
// {
//   int dest_len = strlen(dest);
//   int i = 0;
//   while (src[i] != '\0' && dest_len + i < dest_size - 1)
//   {
//     dest[dest_len + i] = src[i];
//     i++;
//   }
//   dest[dest_len + i] = '\0';
// }

// void syscall(void)
// {
//   int num;
//   struct proc *curproc = myproc();

//   num = curproc->tf->eax;
//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {
//     int retval;
//     char trace_msg[128]; // Temporary buffer for trace message
//     int n = 0;           // Number of characters in trace message

//     // Handle special cases for 'exit' and 'exec' system calls
//     if (curproc->tracing)
//     {
//       if (num == SYS_exit || num == SYS_exec)
//       {
//         // Build the trace message manually
//         trace_msg[0] = '\0'; // Initialize the string

//         safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//         char pid_str[16];
//         itoa(curproc->pid, pid_str);
//         safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//         // For 'exit' and 'exec', return value is not applicable
//         safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//         n = strlen(trace_msg);

//         // Buffer the trace message in the per-process trace buffer
//         if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//         {
//           memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//           curproc->trace_buf_index += n;
//         }
//         // Handle buffer overflow if needed

//         // Also store the event in the global trace buffer (Task 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = 0; // For 'exit' and 'exec', retval is not applicable
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }
//     }

//     // Execute the actual system call
//     retval = syscalls[num]();

//     // Set the return value of the system call
//     curproc->tf->eax = retval;

//     // Handle tracing after system call execution
//     if (curproc->tracing)
//     {
//       if (num != SYS_exit && num != SYS_exec && num != SYS_sbrk)
//       {
//         // Build the trace message manually
//         trace_msg[0] = '\0'; // Initialize the string

//         safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//         char pid_str[16];
//         itoa(curproc->pid, pid_str);
//         safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//         // Always include the return value
//         safe_strcat(trace_msg, " | return value = ", sizeof(trace_msg));
//         char retval_str[16];
//         itoa(retval, retval_str);
//         safe_strcat(trace_msg, retval_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//         n = strlen(trace_msg);

//         // Buffer the trace message in the per-process trace buffer
//         if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//         {
//           memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//           curproc->trace_buf_index += n;
//         }
//         // Handle buffer overflow if needed

//         // Also store the event in the global trace buffer (Task 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = retval;
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }
//     }
//   }
//   else
//   {
//     cprintf("%d %s: unknown sys call %d\n",
//             curproc->pid, curproc->name, num);
//     curproc->tf->eax = -1;
//   }
// }

// // project 3.1
// // int strcmp(const char *p, const char *q)
// // {
// //   while (*p && *p == *q)
// //   {
// //     p++;
// //     q++;
// //   }
// //   return (unsigned char)*p - (unsigned char)*q;
// // }
// // project 3.1

// void syscall(void)
// {
//   int num;
//   struct proc *curproc = myproc();

//   num = curproc->tf->eax;
//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {
//     int retval;
//     char trace_msg[128]; // Temporary buffer for trace message
//     int n = 0;           // Number of characters in trace message

//     // Handle special cases for 'exit' and 'exec' system calls
//     if (curproc->tracing)
//     {
//       if (num == SYS_exit || num == SYS_exec)
//       {
//         // Build the trace message manually
//         trace_msg[0] = '\0'; // Initialize the string

//         safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//         char pid_str[16];
//         itoa(curproc->pid, pid_str);
//         safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//         safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//         n = strlen(trace_msg);

//         // Buffer the trace message in the per-process trace buffer
//         if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//         {
//           memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//           curproc->trace_buf_index += n;
//         }
//         else
//         {
//           // Handle buffer overflow (optional)
//         }

//         // Also store the event in the global trace buffer (Task 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = 0; // For 'exit' and 'exec', retval is not applicable
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }
//     }

//     // Execute the actual system call
//     retval = syscalls[num]();

//     // Handle tracing after system call execution
//     if (curproc->tracing)
//     {
//       if (num != SYS_exit && num != SYS_exec && num != SYS_sbrk)
//       {
//         // Build the trace message manually
//         trace_msg[0] = '\0'; // Initialize the string

//         safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//         char pid_str[16];
//         itoa(curproc->pid, pid_str);
//         safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//         safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//         safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//         safe_strcat(trace_msg, " | return value = ", sizeof(trace_msg));
//         char retval_str[16];
//         itoa(retval, retval_str);
//         safe_strcat(trace_msg, retval_str, sizeof(trace_msg));

//         safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//         n = strlen(trace_msg);

//         // Buffer the trace message in the per-process trace buffer
//         if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//         {
//           memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//           curproc->trace_buf_index += n;
//         }
//         else
//         {
//           // Handle buffer overflow (optional)
//         }

//         // Also store the event in the global trace buffer (Task 2.4)
//         acquire(&trace_lock);
//         trace_buffer[trace_buf_index].pid = curproc->pid;
//         safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//         safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//         trace_buffer[trace_buf_index].retval = retval;
//         trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//         if (trace_buf_count < TRACE_BUF_SIZE)
//           trace_buf_count++;
//         release(&trace_lock);
//       }
//     }

//     // Set the return value of the system call
//     curproc->tf->eax = retval;
//   }
//   else
//   {
//     cprintf("%d %s: unknown sys call %d\n",
//             curproc->pid, curproc->name, num);
//     curproc->tf->eax = -1;
//   }
// } // if project 3.1 does not work then  back to normal

// project 2.6

// project 3.1

// void syscall(void)
// {
//   int num;
//   struct proc *curproc = myproc();

//   num = curproc->tf->eax;
//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {
//     int retval;
//     char trace_msg[128]; // Temporary buffer for trace message
//     int n = 0;           // Number of characters in trace message
//     int should_trace = 0;

//     // Determine if we should trace this syscall
//     if (curproc->tracing)
//     {
//       if (curproc->traced_syscall_num == -1)
//       {
//         // Trace all syscalls
//         should_trace = 1;
//       }
//       else if (curproc->traced_syscall_num == num)
//       {
//         // Trace only the specified syscall
//         should_trace = 1;
//       }
//     }

//     // Handle special cases for 'exit' and 'exec' system calls
//     if (should_trace && (num == SYS_exit || num == SYS_exec))
//     {
//       // Build the trace message manually
//       trace_msg[0] = '\0'; // Initialize the string

//       safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//       char pid_str[16];
//       itoa(curproc->pid, pid_str);
//       safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//       safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//       safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//       safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//       safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//       safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//       n = strlen(trace_msg);

//       // Buffer the trace message in the per-process trace buffer
//       if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//       {
//         memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//         curproc->trace_buf_index += n;
//       }
//       else
//       {
//         // Handle buffer overflow (optional)
//       }

//       // Also store the event in the global trace buffer (Task 2.4)
//       acquire(&trace_lock);
//       trace_buffer[trace_buf_index].pid = curproc->pid;
//       safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//       safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//       trace_buffer[trace_buf_index].retval = 0; // For 'exit' and 'exec', retval is not applicable
//       trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//       if (trace_buf_count < TRACE_BUF_SIZE)
//         trace_buf_count++;
//       release(&trace_lock);
//     }

//     // Execute the actual system call
//     retval = syscalls[num]();

//     // Handle tracing after system call execution
//     if (should_trace && num != SYS_exit && num != SYS_exec && num != SYS_sbrk)
//     {
//       // Build the trace message manually
//       trace_msg[0] = '\0'; // Initialize the string

//       safe_strcat(trace_msg, "TRACE: pid = ", sizeof(trace_msg));
//       char pid_str[16];
//       itoa(curproc->pid, pid_str);
//       safe_strcat(trace_msg, pid_str, sizeof(trace_msg));

//       safe_strcat(trace_msg, " | command_name = ", sizeof(trace_msg));
//       safe_strcat(trace_msg, curproc->name, sizeof(trace_msg));

//       safe_strcat(trace_msg, " | syscall = ", sizeof(trace_msg));
//       safe_strcat(trace_msg, syscall_names[num], sizeof(trace_msg));

//       safe_strcat(trace_msg, " | return value = ", sizeof(trace_msg));
//       char retval_str[16];
//       itoa(retval, retval_str);
//       safe_strcat(trace_msg, retval_str, sizeof(trace_msg));

//       safe_strcat(trace_msg, "\n", sizeof(trace_msg));

//       n = strlen(trace_msg);

//       // Buffer the trace message in the per-process trace buffer
//       if (curproc->trace_buf_index + n < PROC_TRACE_BUF_SIZE)
//       {
//         memmove(&curproc->trace_buf[curproc->trace_buf_index], trace_msg, n);
//         curproc->trace_buf_index += n;
//       }
//       else
//       {
//         // Handle buffer overflow (optional)
//       }

//       // Also store the event in the global trace buffer (Task 2.4)
//       acquire(&trace_lock);
//       trace_buffer[trace_buf_index].pid = curproc->pid;
//       safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
//       safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num], sizeof(trace_buffer[trace_buf_index].syscall_name));
//       trace_buffer[trace_buf_index].retval = retval;
//       trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
//       if (trace_buf_count < TRACE_BUF_SIZE)
//         trace_buf_count++;
//       release(&trace_lock);
//     }

//     // Set the return value of the system call
//     curproc->tf->eax = retval;
//   }
//   else
//   {
//     cprintf("%d %s: unknown sys call %d\n",
//             curproc->pid, curproc->name, num);
//     curproc->tf->eax = -1;
//   }
// }
// project 3.1
