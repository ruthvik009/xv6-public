// System call numbers
#define SYS_fork 1
#define SYS_exit 2
#define SYS_wait 3
#define SYS_pipe 4
#define SYS_read 5
#define SYS_kill 6
#define SYS_exec 7
#define SYS_fstat 8
#define SYS_chdir 9
#define SYS_dup 10
#define SYS_getpid 11
#define SYS_sbrk 12
#define SYS_sleep 13
#define SYS_uptime 14
#define SYS_open 15
#define SYS_write 16
#define SYS_mknod 17
#define SYS_unlink 18
#define SYS_link 19
#define SYS_mkdir 20
#define SYS_close 21
#define SYS_nice 22             // Assign an unused syscall number
#define SYS_set_priority 23     // extra credit-1
#define SYS_lock 24             // Extra Credit Replace XX with the next available syscall number
#define SYS_release 25          // Extra Credit Replace XX accordingly
#define SYS_trace 26            // project
#define SYS_get_trace 27        // project 2.4
#define SYS_tracefilter 28      // project 3.1
#define SYS_traceonlysuccess 29 // project 3.2
#define SYS_traceonlyfail 30    // project 3.3
                                //    #define SYS_trace_set_filter 28 // project 3.1
                                //    #define SYS_set_selective_trace 28      // project 3.1
                                //    #define SYS_set_selective_trace_once 29 // project 3.1
                                //    #define SYS_set_traced_syscall_num 30   // project 3.1