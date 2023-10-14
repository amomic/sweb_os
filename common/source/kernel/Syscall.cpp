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
#include "ArchMemory.h"

size_t
Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
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
        case sc_fork:
            return_value = fork();
            break;
        default:
            return_value = -1;
            kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
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

void Syscall::exit([[maybe_unused]]size_t exit_code)
{
    pthread_exit((void *) -1);
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

    //to update (freeing resources, joining etc)
    UserThread *currThread = (UserThread *) currentThread;
    //currThread->process_->unmapPage();
    currThread->kill();
}

size_t Syscall::pthread_cancel(size_t thread_id)
{
    debug(SYSCALL, "Syscall::pthread_cancel is being called with the following arguments: %zu \n", thread_id);
    // Get Current User Process by casting current user thread into our custom user thread object and calling getProcess() on the object
    UserProcess *currentUserProcess = reinterpret_cast<UserThread *>(currentThread)->getProcess();

    //Lock The thread table
    debug(CANCEL_INFO, "Syscall::pthread_cancel has locked threads map\n");
    currentUserProcess->threads_lock_.acquire();
    // get our thread from the thread table/map
    auto thread_map_entry = currentUserProcess->threads_map_.find(thread_id);
    if (thread_map_entry != currentUserProcess->threads_map_.end())
    {
        // get the thread from table with iterator
        UserThread *cancelled_thread = reinterpret_cast<UserThread *>(thread_map_entry->second);

        //cancle thread ??
        cancelled_thread->thread_cancellation_state_ = UserThread::ISCANCELED;
        debug(CANCEL_SUCCESS, "Syscall::pthread_cancel-> %zu thread has been flagged for cancellation\n",
              cancelled_thread->getTID());

        // unlock threads map, ret success -> (0)
        currentUserProcess->threads_lock_.release();
        debug(CANCEL_INFO, "Syscall::pthread_cancel has unlocked threads map\n");
        return 0;
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

    return 0;
}

size_t Syscall::pthread_setcanceltype(size_t type, size_t *oldtype)
{
    debug(CANCEL_INFO, "pthread_setcancletype is being called with the following arguments: %zu %zu \n", type,
          *oldtype);

    return 0;
}

size_t Syscall::fork()
{
    debug(SYSCALL, "Syscall::fork\n");
    return ProcessRegistry::instance()->fork();
}


