#pragma once

#include "types.h"
#include <ulist.h>
#include "IdleThread.h"
#include "CleanupThread.h"
#include "SwapThread.h"
#include "umap.h"
#include "Mutex.h"

class Thread;

class Mutex;

class SpinLock;

class Lock;

class Scheduler
{
public:
    Mutex sleep_lock_;
    ustl::map<Thread *, uint64> sleeping_threads_ ;

    static Scheduler *instance();

    void addNewThread(Thread *thread);

    void sleep();

    void wake(Thread *thread_to_wake);

    void yield();

    void printThreadList();

    void printStackTraces();

    void printLockingInformation();

    bool isSchedulingEnabled();

    bool isCurrentlyCleaningUp();

    void incTicks();

    size_t getTicks();

    /**
     * NEVER EVER EVER CALL THIS METHOD OUTSIDE OF AN INTERRUPT CONTEXT
     * this is the method that decides which threads will be scheduled next
     * it is called by either the timer interrupt handler or the yield interrupt handler
     * and changes the global variables currentThread and currentThreadRegisters
     */
    void schedule();

    SwapThread swap_thread_;

protected:
    friend class IdleThread;

    friend class CleanupThread;

    friend class SwapThread;

    void cleanupDeadThreads();

private:
    Scheduler();

    /**
     * Scheduler internal lock abstraction method
     * locks the thread-list against concurrent access by prohibiting a thread switch
     * don't call this from an Interrupt-Handler, since Atomicity won't be guaranteed
     */
    void lockScheduling();

    /**
     * Scheduler internal lock abstraction method
     * unlocks the thread-list
     */
    void unlockScheduling();

    static Scheduler *instance_;

    typedef ustl::list<Thread *> ThreadList;
    ThreadList threads_;

    size_t block_scheduling_;

    size_t ticks_;

    IdleThread idle_thread_;
    CleanupThread cleanup_thread_;

public:
    void calculateClockFrequency();

    uint64 last_tsc_;
    uint64 clock_f;

    uint64 cycles_tick[40];

};
