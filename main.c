#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "mutex.h"

// External declarations
extern pde_t *kpgdir;
extern char end[]; // First address after kernel is loaded from ELF file

// Function declarations
static void start_other_processors(void);
static void main_processor(void) __attribute__((noreturn));
static void setup_initial_state(void);
static void initialize_peripherals(void);
static void setup_memory_allocator(void);

// Entry point for the bootstrap processor
int main(void)
{
    mutex_init();
    setup_initial_state();
    initialize_peripherals();
    start_other_processors(); // Start other processors
    kinit2(P2V(4 * 1024 * 1024), P2V(PHYSTOP)); // Initialize physical memory allocator
    userinit(); // Start the first user process
    main_processor(); // Final setup for this processor
}

// Function to initialize the system state
static void setup_initial_state(void)
{
    kinit1(end, P2V(4 * 1024 * 1024));  // Initialize physical page allocator
    kvmalloc(); // Setup kernel page table
    mpinit();   // Initialize multiprocessor support
    lapicinit(); // Initialize Local APIC (interrupt controller)
    seginit();  // Initialize segment descriptors
    picinit();  // Disable the PIC (programmable interrupt controller)
    ioapicinit(); // Initialize IOAPIC
}

// Function to initialize hardware peripherals
static void initialize_peripherals(void)
{
    consoleinit(); // Initialize console hardware
    uartinit();    // Initialize serial port
    pinit();       // Initialize process table
    tvinit();      // Initialize trap vectors
    binit();       // Initialize buffer cache
    fileinit();    // Initialize file system table
    ideinit();     // Initialize disk (IDE)
}

// Function to setup the other processors (APs)
static void start_other_processors(void)
{
    extern uchar _binary_entryother_start[], _binary_entryother_size[];
    uchar *code;
    struct cpu *c;
    char *stack;

    // Copy entry code for APs to unused memory at 0x7000
    code = P2V(0x7000);
    memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

    for (c = cpus; c < cpus + ncpu; c++) {
        if (c == mycpu()) // Skip the bootstrap processor
            continue;

        // Setup the entry point for the APs (using entrypgdir instead of kpgdir)
        stack = kalloc();
        *(void **)(code - 4) = stack + KSTACKSIZE;  // Set stack pointer
        *(void (**)(void))(code - 8) = mpenter;    // Set entry point
        *(int **)(code - 12) = (void *)V2P(entrypgdir); // Set page directory

        // Start the AP processor
        lapicstartap(c->apicid, V2P(code));

        // Wait for the processor to complete mpmain()
        while (c->started == 0)
            ;
    }
}

// Main setup for each processor
static void main_processor(void)
{
    cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
    idtinit();      // Initialize the interrupt descriptor table (IDT)
    xchg(&(mycpu()->started), 1);  // Notify that this processor is started
    scheduler();    // Start the scheduler to run processes
}

// AP Processor entry point (called from entryother.S)
static void mpenter(void)
{
    switchkvm();
    seginit();       // Initialize segment descriptors
    lapicinit();    // Initialize Local APIC
    main_processor(); // Continue with processor initialization
}

// Boot page table used in entry.S and entryother.S
// Ensures page tables are aligned correctly.
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
    // Map VA's [0, 4MB) to PA's [0, 4MB)
    [0] = (0) | PTE_P | PTE_W | PTE_PS,
    // Map VA's [KERNBASE, KERNBASE + 4MB) to PA's [0, 4MB)
    [KERNBASE >> PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK! 
// Blank page.
//PAGEBREAK! 
// Blank page.