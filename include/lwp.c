#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

// Global counter to track tid
static tid_t tid_count = 1; 
static thread current_t = NULL;
static scheduler current_scheduler = NULL;


// Calls the given function and with the given argument, then 
// lwp_exit() with the return value
static void lwp_wrap(lwpfun fun, void *arg) {
    int rval;
    rval = fun(arg);
    lwp_exit(rval);
}

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
    // Stack size of multiple page sizes
    stack_size = (stack_size + page_size - 1) & ~(page_size - 1);

    // Initialize stack (copying mmap from assignment)
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1 ,0); 

    // mmap error checking
    if (stack==MAP_FAILED) {
        return NO_THREAD;
    }

    // initialize thread context
    thread t = calloc(1, sizeof(context));
    // calloc error checking
    if (!t) {
        munmap(stack, stack_size);
        return NO_THREAD;
    }
    
    t->stack = stack;
    t->tid = tid_count++;
    t->stacksize = stack_size;
    t->status = LWP_LIVE;
    // Pointer to save floating point state (from fp.h)
    t->state.fxsave = FPU_INIT;

    // Stack entries for rfiles to track stack pointer, program counter
    // types much match registers type (unsigned long)
    // sp points to top of stack
    unsigned long *sp = (unsigned long *)((char *)stack + stack_size);    

    // return address for ret is one below top of stack
    sp--;
    sp[0] = (unsigned long)lwp_wrap;
    sp[1] = 0;

    // Set registers in thread's rfile
    t->state.rbp = (unsigned long)(sp + 1);
    t->state.rsp = (unsigned long)sp;
    t->state.rdi = (unsigned long)function;
    t->state.rsi = (unsigned long)argument;

    // Admit thread into scheduler
    current_scheduler->admit(t);
    return t->tid;
}

// Starts LWP system
void lwp_start() {
    // Transform calling thread to LWP (essentially everything in create but use existing stack
    current
}

void lwp_yield() {
    //swap_rfil
}

void lwp_exit(int exitval) {

}

tid_t lwp_wait(int *status) {

}

tid_t lwp_gettid() {
    return current_thread->tid;
}


