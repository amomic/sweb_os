#include "Mutex.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "backtrace.h"
#include "assert.h"
#include "Stabs2DebugInfo.h"
#include "Semaphores.h"
#include "Mutex.h"
#include "types.h"
extern Stabs2DebugInfo const* kernel_debug_info;


Semaphore::Semaphore(const char *name1, int initialValue) : Lock(name1), lock_("Semaphores::lock_") {
    semaphore_ = initialValue;
}

void Semaphore::wait() {
    lock_.acquire();
    while (semaphore_ <= 0) {

        lockWaitersList();
        if(semaphore_>0)
        {
            unlockWaitersList();
            break;
        }
        // Block the current thread until signaled
        sleepAndRelease();
        lock_.acquire();
    }
    semaphore_--;
    lock_.release();
}

void Semaphore::signal() {
    lock_.acquire();
    semaphore_++;
    if (semaphore_ <= 0) {
        // Wake up a waiting thread
        Thread* thread_to_be_woken_up = popBackThreadFromWaitersList();
        lock_.release();
        Scheduler::instance()->wake(thread_to_be_woken_up);
    } else {
        lock_.release();
    }
}