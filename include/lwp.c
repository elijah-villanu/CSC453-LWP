#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

// Global counter to track tid (non-zero 0 is NO_THREAD? on header)
static tid_t tid_count = 1; 

tid_t lwp_create(lwpfun function, void *argument) {
    // Determine soft limit (needed to determine stack size when mmaping)
    struct rlimit r1;
    size_t stack_size;
    size_t page_size = sysconf(_SC_PAGE_SIZE);

    // If there's no limit, fallback to 8mb soft limit
    if (getrlimit(RLIMIT_STACK, &r1) == 0 && r1.rlim_cur != RLIM_INFINITY) {
        stack_size = r1.rlim_cur;
    } else {
        stack_size = 1024 * 1024 * 8;   
    }

    // Initialize stack (copying mmap from assignment)
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1 ,0); 

    // mmap error checking
    if (stack==MAP_FAILED) {
        // mmap cleanup
        munmap(2);
        return NO_THREAD;
    }

    // initialize thread context
    thread t = calloc(1, sizeof(context));
    t->stack = stack;
    t->tid = tid_counter++;
    t->stacksize = stack_size;
    // Assuming 0 is an okay status (NEED TO CHECK)
    t->status = 0;
    // Pointer to save floating point state
    t->state.fxsave = ;

    // Stack entries for rfiles to track stack pointer, program counter
    // types much match registers type (unsigned long)
    unsigned long *sp =     

    
}

void lwp_start() {

}
