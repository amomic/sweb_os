#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"
#include "UserThread.h"
#include "UserProcess.h"
#include "Loader.h"
#include "ArchMemory.h"
#include "ArchThreads.h"

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{

    UserThread *pUserThread = reinterpret_cast<UserThread *>(currentThread);
    if (pUserThread->getThreadCancellationState() == UserThread::ISCANCELED &&
        pUserThread->getThreadCancelState() == UserThread::ENABLED &&
        pUserThread->getThreadCancelType() == UserThread::DEFERRED)
    {
        pthread_exit(reinterpret_cast<void *>(-1ULL));
    }

    size_t return_value = 0;
    if ((syscall_number != sc_sched_yield) &&
        (syscall_number != sc_outline)) // no debug print because these might occur very often
    {
        debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
              syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
    }

    switch (syscall_number)
    {
        case sc_sched_yield:
            Scheduler::instance()->yield();
            break;
        case sc_createprocess:
            return_value = createprocess(arg1, arg2);
            break;
        case sc_exit:
            exit(arg1);
            break;
        case sc_write:
            return_value = write(arg1, arg2, arg3);
            break;
        case sc_read:
            return_value = read(arg1, arg2, arg3);
            break;
        case sc_open:
            return_value = open(arg1, arg2);
            break;
        case sc_close:
            return_value = close(arg1);
            break;
        case sc_outline:
            outline(arg1, arg2);
            break;
        case sc_trace:
            trace();
            break;
        case sc_pseudols:
            pseudols((const char *) arg1, (char *) arg2, arg3);
            break;
        case sc_pthread_create:
            return_value = pthread_create(arg1, arg2, reinterpret_cast<void *(*)(void *)>(arg3), arg4, arg5);
            break;
        case sc_pthread_exit:
            pthread_exit(reinterpret_cast<void *>(arg1));
            break;
        case sc_pthread_cancel:
            return_value = pthread_cancel(arg1);
            break;
        case sc_pthread_setcanceltype:
            return_value = pthread_setcanceltype(arg1, (size_t *) arg2);
            break;
        case sc_pthread_setcancelstate:
            return_value = pthread_setcancelstate(arg1, (size_t *) arg2);
            break;
        case sc_pthread_join:
            return_value = pthread_join(arg1, arg2);
            break;
        case sc_pthread_detach:
            return_value = pthread_detach(arg1);
            break;
        case sc_fork:
            return_value = fork();
            break;
        case sc_execv:
            return_value = execv(reinterpret_cast<char *>(arg1), reinterpret_cast<char **>(arg2));
            break;
        case sc_clock:
            return_value = clock();
            break;
        case sc_sleep:
            return_value = thread_sleep(arg1);
            break;
        case sc_waitpid:
            return_value = waitpid(arg1, reinterpret_cast<int *>(arg2), arg2);
            break;
        default:
            return_value = -1;
            kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
    }
    if (pUserThread->getThreadCancellationState() == UserThread::ISCANCELED &&
        pUserThread->getThreadCancelState() == UserThread::ENABLED &&
        pUserThread->getThreadCancelType() == UserThread::DEFERRED)
    {
        pthread_exit(reinterpret_cast<void *>(-1ULL));
    }

    return return_value;
}


void Syscall::pseudols(const char *pathname, char *buffer, size_t size)
{
    if (buffer && ((size_t) buffer >= USER_BREAK || (size_t) buffer + size > USER_BREAK))
        return;
    if ((size_t) pathname >= USER_BREAK)
        return;
    VfsSyscall::readdir(pathname, buffer, size);
}

void Syscall::exit(size_t exit_code)
{
    debug(SYSCALL, "Syscall::exit: %zu\n", exit_code);
    // auto current = ((UserThread *) currentThread);
    ProcessRegistry::instance()->updateExitCode(exit_code); //todo

    pthread_exit((void *) exit_code);
}

size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
    //WARNING: this might fail if Kernel PageFaults are not handled
    if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
    {
        return -1U;
    }

    size_t num_written = 0;

    if (fd == fd_stdout) //stdout
    {
        debug(SYSCALL, "Syscall::write: %.*s\n", (int) size, (char *) buffer);
        kprintf("%.*s", (int) size, (char *) buffer);
        num_written = size;
    } else
    {
        num_written = VfsSyscall::write(fd, (char *) buffer, size);
    }
    return num_written;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
    if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
    {
        return -1U;
    }

    size_t num_read = 0;

    if (fd == fd_stdin)
    {
        //this doesn't! terminate a string with \0, gotta do that yourself
        num_read = currentThread->getTerminal()->readLine((char *) buffer, count);
        debug(SYSCALL, "Syscall::read: %.*s\n", (int) num_read, (char *) buffer);
    } else
    {
        num_read = VfsSyscall::read(fd, (char *) buffer, count);
    }
    return num_read;
}

size_t Syscall::close(size_t fd)
{
    return VfsSyscall::close(fd);
}

size_t Syscall::open(size_t path, size_t flags)
{
    if (path >= USER_BREAK)
    {
        return -1U;
    }
    return VfsSyscall::open((char *) path, flags);
}

void Syscall::outline(size_t port, pointer text)
{
    //WARNING: this might fail if Kernel PageFaults are not handled
    if (text >= USER_BREAK)
    {
        return;
    }
    if (port == 0xe9) // debug port
    {
        writeLine2Bochs((const char *) text);
    }
}

size_t Syscall::createprocess(size_t path, size_t sleep)
{
    // THIS METHOD IS FOR TESTING PURPOSES ONLY AND NOT MULTITHREADING SAFE!
    // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

    // parameter check begin
    if (path >= USER_BREAK)
    {
        return -1U;
    }

    debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%zd\n", (char *) path, sleep);
    ssize_t fd = VfsSyscall::open((const char *) path, O_RDONLY);
    if (fd == -1)
    {
        return -1U;
    }
    VfsSyscall::close(fd);
    // parameter check end

    size_t process_count = ProcessRegistry::instance()->processCount();
    ProcessRegistry::instance()->createProcess((const char *) path);
    if (sleep)
    {
        while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
        {
            Scheduler::instance()->yield();
        }
    }
    return 0;
}

void Syscall::trace()
{
    currentThread->printBacktrace();
}

size_t
Syscall::pthread_create(pointer thread, pointer attr, void *(start_routine)(void *), pointer arg, pointer wrapper)
{
    if (thread == NULL || start_routine == nullptr)
    {
        return -1;
    }

    UserThread *currThread = (UserThread *) currentThread;
    size_t ret = (size_t) (currThread->getProcess()->createThread(reinterpret_cast<size_t *>(thread),
                                                                  reinterpret_cast<size_t *>(attr), start_routine,
                                                                  reinterpret_cast<void *>(wrapper), arg, 0));

    if (!ret)
    {
        return -1;
    }

    return 0;
}

void Syscall::pthread_exit([[maybe_unused]]void *value)
{
    // TODO:
    // Finish JOIN
    //to update (freeing resources, joining etc)
    UserThread *current_thread = (UserThread *) currentThread;
    UserProcess *current_process = current_thread->getProcess();
    debug(SYSCALL, "entering pthread exit ! \n");
    current_process->return_val_lock_.acquire();

    // Store return value
    current_process->thread_retval_map.push_back({current_thread->getTID(), (void *) value});

    if (current_thread->waited_by_ != nullptr)
    {
        current_thread->waited_by_->join_condition_.signal();

        current_thread->waited_by_->waiting_for_ = nullptr;
        current_thread->waited_by_ = nullptr;
    }

    current_process->return_val_lock_.release();

    current_thread->getProcess()->threads_lock_.acquire();
    current_thread->getProcess()->threads_map_.erase(current_thread->getTID());
    current_thread->getProcess()->threads_lock_.release();
    debug(SYSCALL, "line before kill in pexit");

    current_thread->getProcess()->loader_->arch_memory_.arch_mem_lock.acquire();
    current_thread->getProcess()->unmapPage();
    current_thread->getProcess()->loader_->arch_memory_.arch_mem_lock.release();

    current_thread->kill();
}

size_t Syscall::pthread_join(size_t joinee_thread, [[maybe_unused]]pointer return_val)
{
    auto joiner_thread = reinterpret_cast<UserThread *>(currentThread);
    return joiner_thread->getProcess()->joinThread(joinee_thread, return_val);
}

size_t Syscall::pthread_detach(size_t thread)
{
    kprintf("SYSCALL STARTED!\n");
    auto detach_thread = reinterpret_cast<UserThread *>(currentThread);
    return detach_thread->getProcess()->detachThread(thread);
}

size_t Syscall::pthread_cancel(size_t thread_id)
{
    debug(SYSCALL, "Syscall::pthread_cancel is being called with the following arguments: %zu \n", thread_id);
    // Get Current User Process by casting current user thread into our custom user thread object and calling getProcess() on the object
    UserThread *pUserThread = reinterpret_cast<UserThread *>(currentThread);
    UserProcess *currentUserProcess = pUserThread->getProcess();

    //Lock The thread table
    debug(CANCEL_INFO, "Syscall::pthread_cancel has locked threads map\n");
    currentUserProcess->threads_lock_.acquire();
    // get our thread from the thread table/map
    auto thread_map_entry = currentUserProcess->threads_map_.find(thread_id);
    if (thread_map_entry != currentUserProcess->threads_map_.end())
    {
        // get the thread from table with iterator
        UserThread *cancelled_thread = reinterpret_cast<UserThread *>(thread_map_entry->second);

        if (cancelled_thread->getThreadCancelState() == UserThread::THREAD_CANCEL_STATE::ENABLED &&
            cancelled_thread->getThreadCancelType() == UserThread::THREAD_CANCEL_TYPE::DEFERRED)
        {
            //cancle thread ??
            cancelled_thread->setThreadCancellationState(UserThread::ISCANCELED);
            debug(CANCEL_SUCCESS, "Syscall::pthread_cancel-> %zu thread has been flagged for cancellation\n",
                  cancelled_thread->getTID());

            // unlock threads map, ret success -> (0)
            currentUserProcess->threads_lock_.release();
            debug(CANCEL_INFO, "Syscall::pthread_cancel has unlocked threads map\n");
            return 0;
        } else
        {
            debug(CANCEL_ERROR, "Syscall::pthread_cancel: %zu found, but cannot cancelled \n", thread_id);
            currentUserProcess->threads_lock_.release();
            debug(CANCEL_INFO, "Syscall::pthread_cancel has unlocked threads map\n");
            return -1ULL;
        }
    }

    // unlock threads map
    currentUserProcess->threads_lock_.release();
    debug(CANCEL_INFO, "Syscall::pthread_cancel has unlocked threads map\n");
    // retrun if not found
    debug(CANCEL_ERROR, "Syscall::pthread_cancel: %zu not found, thus not cancelled \n", thread_id);

    return -1ULL;
}

size_t Syscall::pthread_setcancelstate(size_t state, size_t *oldstate)
{
    debug(CANCEL_INFO, "pthread_setcanclestate is being called with the following arguments: %zu %zu \n", state,
          *oldstate);
    UserThread *current_thread = reinterpret_cast<UserThread *>(currentThread);

    // check for unknown state
    if (state != UserThread::THREAD_CANCEL_STATE::ENABLED && state != UserThread::THREAD_CANCEL_STATE::DISABLED)
    {
        debug(CANCEL_ERROR, "pthread_setcanclestate-> State invalid!!!\n");
        return -1ULL;
    }

    // Oldstate should not be nullpointer or kernelpointer
    if ((size_t) oldstate >= USER_BREAK)
    {
        return -1ULL;
    }

    // myb chck invalid state?

    if (oldstate != nullptr)
    {
        *oldstate = current_thread->getThreadCancelState();
    }
    reinterpret_cast<UserThread *> (currentThread)->setThreadCancelState((UserThread::THREAD_CANCEL_STATE) state);

    return 0;
}

size_t Syscall::pthread_setcanceltype(size_t type, size_t *oldtype)
{
    debug(CANCEL_INFO, "pthread_setcancletype is being called with the following arguments: %zu %zu \n", type,
          *oldtype);
    UserThread *current_thread = reinterpret_cast<UserThread *>(currentThread);

    // check for unknown type
    if (type != UserThread::THREAD_CANCEL_TYPE::DEFERRED && type != UserThread::THREAD_CANCEL_TYPE::ASYNCHRONOUS)
    {
        debug(CANCEL_ERROR, "pthread_setcanclestate-> State invalid!!!\n");
        return -1ULL;
    }

    // Oldstate should not be nullpointer or kernelpointer
    if ((size_t) oldtype >= USER_BREAK)
    {
        return -1ULL;
    }

    // myb chck invalid state?

    if (oldtype != nullptr)
    {
        *oldtype = current_thread->getThreadCancelType();
    }
    reinterpret_cast<UserThread *> (currentThread)->setThreadCancelType((UserThread::THREAD_CANCEL_TYPE) type);

    return 0;
}

size_t Syscall::fork()
{
    debug(SYSCALL, "Syscall::fork\n");
    size_t ret = ProcessRegistry::instance()->fork();
    return ret;
}

size_t Syscall::execv([[maybe_unused]]char *path, [[maybe_unused]]char *const *argv)
{
    char *path_name = (char *) path;

    auto path_len = strlen(path_name);

    if ((size_t) path_name >= USER_BREAK ||
        (size_t) path_name == NULL ||
        (unsigned long long int) argv >= USER_BREAK)
    {
        debug(SYSCALL, "[Exec] Wrong address!");
        return -1U;
    }

    if (path_len > 256)
    {
        debug(SYSCALL, "[Exec] Path length too long!");
        return -1U;
    }

    // Do we need anything else here?
    UserProcess *user_process = ((UserThread *) currentThread)->getProcess();

    size_t ret = user_process->exec(path_name, (char *const *) argv);

    return ret;
}

pid_t Syscall::waitpid(pid_t pid, int *status, int options)
{
    debug(SYSCALL, "Waitpid.\n");
    return ((UserThread *) currentThread)->getProcess()->waitpid(pid, status, options);
}

size_t Syscall::clock(void)
{
    uint64 time_start = reinterpret_cast<UserThread *>(currentThread)->getStartClockTcs();
    uint64 time_current = ArchThreads::rdtsc();

    size_t value = ((time_current - time_start) / (Scheduler::instance()->clock_f));
    return value;

}

size_t Syscall::thread_sleep(size_t seconds)
{
//     https://www.quora.com/What-is-the-time-for-a-processor-to-respond-to-an-interrupt-run-the-ISR-and-return-to-the-interrupted-program-given-the-following-details
//     1 sec approx 18 tick
//     current_ticks + amount of ticks for sleep -> tick value on which the thread should wake up
//     should yield and leave the resources for threads that are still running
//     return 0 if rescheduled

    debug(SYSCALL, "Syscall::Sleep for %zu seconds\n", seconds);

    uint64 wake_up_at_tcs = ArchThreads::rdtsc() + seconds * 1000000 * Scheduler::instance()->clock_f;
    Scheduler::instance()->sleeping_threads_.push_back({currentThread, wake_up_at_tcs});
    Scheduler::instance()->yield();
    Scheduler::instance()->sleeping_threads_.erase({currentThread});

    return 0;
}

