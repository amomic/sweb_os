#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include <ulist.h>
#include "backtrace.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "umap.h"
#include "ustring.h"
#include "Lock.h"
#include "UserThread.h"
#include "Syscall.h"

ArchThreadRegisters *currentThreadRegisters;
Thread *currentThread;

Scheduler *Scheduler::instance_ = nullptr;

Scheduler *Scheduler::instance()
{
    if (unlikely(!instance_))
        instance_ = new Scheduler();
    return instance_;
}

Scheduler::Scheduler() : sleep_lock_("Sleeping threads Lock")
{
    block_scheduling_ = 0;
    ticks_ = 0;
    addNewThread(&cleanup_thread_);
    addNewThread(&idle_thread_);
    addNewThread(&swap_thread_);
}

void Scheduler::schedule()
{
    assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");
    if (block_scheduling_)
    {
        debug(SCHEDULER, "schedule: currently blocked\n");
        return;
    }

    auto it = threads_.begin();
    for (; it != threads_.end(); ++it)
    {
        if ((*it)->schedulable())
        {
//            Scheduler::instance()->sleep_lock_.acquire();
            auto sleeping_entry = sleeping_threads_.find(*it);
            if (sleeping_entry != sleeping_threads_.end()) // Check if the thread is sleeping
            {
                uint64 i = ArchThreads::rdtsc();
                if (sleeping_entry->second > i)
                {
//                    Scheduler::instance()->sleep_lock_.release();
                    continue;
                }
            }
//            Scheduler::instance()->sleep_lock_.release();
            if(currentThread != NULL && currentThread->type_ == Thread::USER_THREAD && currentThread->loader_ != nullptr) {
                if(SwapThread::instance()->active_pra_ == SC_PRA || SwapThread::instance()->active_pra_ == AGING_PRA) {
                    ((UserThread*)currentThread)->getProcess()->updateArchMem();
                }
            }
            currentThread = *it;
            break;
        }
    }

    assert(it != threads_.end() && "No schedulable thread found");
    ustl::rotate(threads_.begin(), it + 1,
                 threads_.end()); // no new/delete here - important because interrupts are disabled
    //debug(SCHEDULER, "Scheduler::schedule: new currentThread is %p %s, switch_to_userspace: %d\n", currentThread, currentThread->getName(), currentThread->switch_to_userspace_);

    UserThread *pUserThread = reinterpret_cast<UserThread *>(currentThread);
    if (pUserThread->switch_to_userspace_ &&
        pUserThread->getThreadCancelType() == UserThread::ASYNCHRONOUS &&
        pUserThread->getThreadCancellationState() == UserThread::ISCANCELED)
    {
        // Does not have to be atomic, we are in the schedulers, Interrupts are disabled
        pUserThread->switch_to_userspace_ = 0;
        pUserThread->kernel_registers_->rip = (size_t) (Syscall::pthread_exit);
        pUserThread->kernel_registers_->rdi = -1ULL;
    }

    currentThreadRegisters = currentThread->switch_to_userspace_ ? currentThread->user_registers_
                                                                 : currentThread->kernel_registers_;
}

void Scheduler::addNewThread(Thread *thread)
{
    assert(thread);
    debug(SCHEDULER, "addNewThread: %p  %zd:%s\n", thread, thread->getTID(), thread->getName());
    if (currentThread)
        ArchThreads::debugCheckNewThread(thread);
    KernelMemoryManager::instance()->getKMMLock().acquire();
    lockScheduling();
    KernelMemoryManager::instance()->getKMMLock().release();
    threads_.push_back(thread);
    unlockScheduling();
}

void Scheduler::sleep()
{
    currentThread->setState(Sleeping);
    assert(block_scheduling_ == 0);
    yield();
}

void Scheduler::wake(Thread *thread_to_wake)
{
    // wait until the thread is sleeping
    while (thread_to_wake->getState() != Sleeping)
        yield();
    thread_to_wake->setState(Running);
}

void Scheduler::yield()
{
    assert(this);
    if (!ArchInterrupts::testIFSet())
    {
        assert(currentThread);
        kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %p %s)\n",
                 currentThread, currentThread->name_.c_str());
        currentThread->printBacktrace();
    }
    ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
    /* Before adding new functionality to this function, consider if that
       functionality could be implemented more cleanly in another place.
       (e.g. Thread/Process destructor) */

    assert(currentThread == &cleanup_thread_);

    lockScheduling();
    uint32 thread_count_max = sizeof(cleanup_thread_.kernel_stack_) / (2 * sizeof(Thread *));
    thread_count_max = ustl::min(thread_count_max, threads_.size());
    Thread *destroy_list[thread_count_max];
    uint32 thread_count = 0;
    for (uint32 i = 0; i < threads_.size() && thread_count < thread_count_max; ++i)
    {
        Thread *tmp = threads_[i];
        if (tmp->getState() == ToBeDestroyed)
        {
            destroy_list[thread_count++] = tmp;
            threads_.erase(threads_.begin() + i); // Note: erase will not realloc!
            --i;
        }
    }
    unlockScheduling();
    if (thread_count > 0)
    {
        for (uint32 i = 0; i < thread_count; ++i)
        {
            delete destroy_list[i];
        }
        debug(SCHEDULER, "cleanupDeadThreads: done\n");
    }
}

void Scheduler::printThreadList()
{
    lockScheduling();
    debug(SCHEDULER, "Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
    for (size_t c = 0; c < threads_.size(); ++c)
        debug(SCHEDULER, "Scheduler::printThreadList: threads_[%zd]: %p  %zd:%25s     [%s]\n", c, threads_[c],
              threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_]);
    unlockScheduling();
}

void Scheduler::lockScheduling() //not as severe as stopping Interrupts
{
    if (unlikely(ArchThreads::testSetLock(block_scheduling_, 1)))
        kpanict("FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n");
}

void Scheduler::unlockScheduling()
{
    block_scheduling_ = 0;
}

bool Scheduler::isSchedulingEnabled()
{
    return this && block_scheduling_ == 0;
}

bool Scheduler::isCurrentlyCleaningUp()
{
    return currentThread == &cleanup_thread_;
}

size_t Scheduler::getTicks()
{
    return ticks_;
}

void Scheduler::incTicks()
{
    ++ticks_;
    if(ticks_ % 20 == 0 && SwapThread::instance()->active_pra_ == AGING_PRA)
    {
        SwapThread::instance()->age();
    }
    if(!clock_f)
    {
        calculateClockFrequency();
    }
}

void Scheduler::printStackTraces()
{
    lockScheduling();
    debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

    for (const auto &thread: threads_)
    {
        thread->printBacktrace();
        debug(BACKTRACE, "\n");
        debug(BACKTRACE, "\n");
    }

    unlockScheduling();
}

void Scheduler::printLockingInformation()
{
    lockScheduling();
    kprintfd("\n");
    debug(LOCK, "Scheduler::printLockingInformation:\n");

    for (Thread *t: threads_)
    {
        if (t->holding_lock_list_)
            Lock::printHoldingList(t);
    }
    for (Thread *t: threads_)
    {
        if (t->lock_waiting_on_)
            debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p).\n",
                  t->getName(), t, t->lock_waiting_on_->getName(), t->lock_waiting_on_);
    }
    debug(LOCK, "Scheduler::printLockingInformation finished\n");
    unlockScheduling();
}

void Scheduler::calculateClockFrequency()
{
    if(ticks_ < 30)
    {
        cycles_tick[ticks_] = ArchThreads::rdtsc() - last_tsc_;
        last_tsc_ = ArchThreads::rdtsc();
        return;
    }

    if(ticks_ == 30)
    {
        uint64_t cycles_tick_average_ = 0;

        for(size_t i = 10; i < 30; ++i)
        {
            cycles_tick_average_ += cycles_tick[i];
        }

        cycles_tick_average_ = cycles_tick_average_ / 20;
        clock_f = cycles_tick_average_ / 54925;
    }

    last_tsc_ = ArchThreads::rdtsc();
}