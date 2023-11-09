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

        lock_.release();

        checkCurrentThreadStillWaitingOnAnotherLock();
        lockWaitersList();


        if(semaphore_>0)
        {

            unlockWaitersList();


            break;
        }
        doChecksBeforeWaiting();

        // Block the current thread until signaled
        sleepAndRelease();
        lock_.acquire();
        currentThread->lock_waiting_on_=0;


    }
    semaphore_--;
    lock_.release();

}

void Semaphore::post() {

    lockWaitersList();
    lock_.acquire();
    semaphore_++;
    lock_.release();
    // Wake up a waiting thread
    Thread* thread_to_be_woken_up = popBackThreadFromWaitersList();

    unlockWaitersList();
    if(thread_to_be_woken_up != nullptr)
        Scheduler::instance()->wake(thread_to_be_woken_up);




}