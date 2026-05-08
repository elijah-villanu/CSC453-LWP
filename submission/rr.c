#include "lwp.h"
#include <stdlib.h>
#include <unistd.h>

// These variables will hold our data of our linked list,
// static so they will hold values after multiple calls
static thread head = NULL;
static thread tail = NULL;
static int qlen_count = 0;

// Adds a new thread to the end of the queue
static void rr_admit(thread new) {
    // Setting the "next" pointer of this new thread
    new->sched_one = NULL;

    if (!head) {
        head = new;
        tail = new;
    } else {
        tail->sched_one = new;
        tail = new;
    }
    qlen_count++;
}

// Removes a specific thread from the queue
static void rr_remove(thread victim) {
    if (!victim || !head) {
        return;
    }

    // Checking if first thread is the victim
    if (head == victim) {
        head = victim->sched_one;

        // If last thread, make sure tail not pointing to it still
        if (!head) {
            tail = NULL;
        }

        qlen_count--;
        return;
    }

    thread curr = head;
    while (curr->sched_one != NULL) {
        // If next thread is the victim
        if (curr->sched_one == victim) {
            if (victim == tail) {
                tail = curr;
            }
            curr->sched_one = curr->sched_one->sched_one;
            qlen_count--;
            return;
        }
        curr = curr->sched_one;
    }
}

// Returns the thread from the start of the queue,
// enqueues it until rr_remove completely removes it
static thread rr_next() {
    // Queue empty
    if (!head || !tail) {
        return NULL;
    }

    thread next = head;

    // If only existing thread
    if (head == tail) {
        return next;
    }

    // Progress the queue
    head = next->sched_one;

    // Enqueue the chosen thread according to round robin
    tail->sched_one = next;
    next->sched_one = NULL;
    tail = next;

    return next;
}

// Returns number of threads in the queue
static int rr_qlen() {
    return qlen_count;
}

static struct scheduler rr_struct = { 
    NULL,
    NULL,
    rr_admit,
    rr_remove,
    rr_next,
    rr_qlen
};

// Global, so any program can pass RoundRobin to lwp_set_scheduler
scheduler RoundRobin = &rr_struct;
