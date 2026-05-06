#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <schedulers.h>

// Global counter to track tid
static tid_t tid_count = 1; 
static thread current_thread = NULL;
static scheduler current_scheduler = NULL;

// Queue to hold the terminated threads
static thread term_head = NULL;
static thread term_tail = NULL;

// Queue to hold threads that are waiting for another thread to exit
// Kind of like waitpid()
static thread wait_head = NULL;
static thread wait_tail = NULL;

static void enqueue(thread *head, thread *tail, thread t) {
	t->lib_one = NULL;

	if (!(*head)) {
		*head = t;
	} else {
		(*tail)->lib_one = t;
	}
	*tail = t;
}

static thread dequeue(thread *head, thread *tail) {
	if (!(*head)) {
		return NULL;	
	}
	thread chosen = *head;
	*head = chosen->lib_one;
	
	// Queue is now empty
	if (!(*head)) {
		*tail = NULL;
	}
	chosen->lib_one = NULL;
	return chosen;
}


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
    void *stack = mmap(
		NULL, 
		stack_size, 
		PROT_READ|PROT_WRITE, 
		MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,
		-1,
		0
	); 

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
    sp -= 2;
    sp[0] = (unsigned long)lwp_wrap;
    sp[1] = 0;

    // Set registers in thread's rfile
    t->state.rbp = (unsigned long)(sp + 1);
    t->state.rsp = (unsigned long)sp;
    t->state.rdi = (unsigned long)function;
    t->state.rsi = (unsigned long)argument;

    if (!current_scheduler) {
        lwp_set_scheduler(NULL);
    }

    // Admit thread into scheduler
    current_scheduler->admit(t);
    return t->tid;
}

// Starts LWP system
void lwp_start() {
    // Transform calling process as a LWP (needed for lwp_yield) without stack
    // Invoke first thread to run determined by scheduler with lwp_yield
    thread calling_thread = calloc(1, sizeof(context));
    if (!calling_thread) {
        exit(3);
    }
       
    // Instead using already made stack
    calling_thread->stack = NULL;
    calling_thread->stacksize = 0;    
    calling_thread->tid = tid_count++;
    calling_thread->status = LWP_LIVE;
    
    // swap r_files reads from CPU directly, already knows registers

    if (!current_scheduler) {
        lwp_set_scheduler(NULL);
    }
    // Admit into scheduler
    current_scheduler->admit(calling_thread);

    current_thread = calling_thread;

    // Switch to first process in scheduler
    lwp_yield();
}

void lwp_yield() {
    thread next_thread = current_scheduler->next();

    if (!next_thread) {
		exit(LWPTERMSTAT(current_thread->status));
    }

	thread prev = current_thread;
	current_thread = next_thread;
    swap_rfiles(&prev->state, &next_thread->state);
}

void lwp_exit(int exitval) {
	current_thread->status = MKTERMSTAT(LWP_TERM, exitval);
	current_scheduler->remove(current_thread);
	
	// Basic flow I think
	// 1. Check if anyone waiting to clean up a thread in the wait queue
	// 2. Take them out of waiter queue
	// 3. Attach the exiting thread onto them with exitted (in struct) 
	// 4. Resume the thread
	// 5. Otherwise if no thread in wait queue, add thread to term queue
	
	lwp_yield(); 
}

tid_t lwp_wait(int *status) {
	// No threads terminated, and this is last existing thread
	if (!term_head && current_scheduler->qlen() <= 1) {
		return NO_THREAD;
	} 

	// If no threads terminated yet, block until one does
	if (!term_head) {
		current_scheduler->remove(current_thread);
		enqueue(&wait_head, &wait_tail, current_thread);
		lwp_yield();	
	}

	thread oldest = dequeue(&term_head, &term_tail);
	
	// ...WIP...

	return NO_THREAD; // TEMP, REMOVE	
}

tid_t lwp_gettid() {
    if (!current_thread) {
        return NO_THREAD;
    }

    return current_thread->tid;
}

thread tid2thread(tid_t tid) {
    scheduler tmp = current_scheduler;
    thread next = tmp->next();
    
    // Loop through scheduled threads to find matching tid
    while (next != NULL) {
        if (next->tid == tid) {
            return next;
        }
        next = tmp->next();
    }

    return NULL;
}

void lwp_set_scheduler(scheduler sched) {
    // If no scheduler provided, revert back to RoundRobin
    if (!sched) {
        current_scheduler = RoundRobin;
        return;
    }
    
    // If init function exists on new scheduler; call it
    if (sched->init != NULL) {
        sched->init();
    }

    // Grab the first thread that was upcoming from old scheduler
    thread next = current_scheduler->next();
    // Loop until all previous threads are converted to new scheduler
    while (next != NULL) {
        sched->admit(next);
        next = current_scheduler->next();
    }

    // If shutdown function exists on old scheduler; call it
    if (current_scheduler->shutdown != NULL) {
        current_scheduler->shutdown();
    }

    // Change LWP to be running with new scheduler
    current_scheduler = sched;
}

scheduler lwp_get_scheduler(void) {
    return current_scheduler;
}
