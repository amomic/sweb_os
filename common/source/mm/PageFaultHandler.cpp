#include "PageFaultHandler.h"
#include "kprintf.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "offsets.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Syscall.h"
#include "ArchThreads.h"
#include "UserThread.h"
#include "paging-definitions.h"
#include "PageManager.h"

extern "C" void arch_contextSwitch();

const size_t PageFaultHandler::null_reference_check_border_ = PAGE_SIZE;

inline bool PageFaultHandler::checkPageFaultIsValid(size_t address, bool user,
                                                    bool present, bool switch_to_us)
{
  assert((user == switch_to_us) && "Thread is in user mode even though is should not be.");
  assert(!(address < USER_BREAK && currentThread->loader_ == 0) && "Thread accesses the user space, but has no loader.");
  assert(!(user && currentThread->user_registers_ == 0) && "Thread is in user mode, but has no valid registers.");

  if(address < null_reference_check_border_)
  {
    debug(PAGEFAULT, "Maybe you are dereferencing a null-pointer.\n");
  }
  else if(!user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing an invalid kernel address.\n");
  }
  else if(user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing a kernel address in user-mode.\n");
  }
  else if(present)
  {
    debug(PAGEFAULT, "You got a pagefault even though the address is mapped.\n");
    //because of cow, this might be valid now
    return true;
  }
  else
  {
    // everything seems to be okay
    return true;
  }
  return false;
}

inline void PageFaultHandler::handlePageFault(size_t address, bool user,
                                          bool present, bool writing,
                                          bool fetch, bool switch_to_us)
{
  if (PAGEFAULT & OUTPUT_ENABLED)
    kprintfd("\n");
  debug(PAGEFAULT, "Address: %18zx - Thread %zu: %s (%p)\n",
        address, currentThread->getTID(), currentThread->getName(), currentThread);
  debug(PAGEFAULT, "Flags: %spresent, %s-mode, %s, %s-fetch, switch to userspace: %1d\n",
        present ? "    " : "not ",
        user ? "  user" : "kernel",
        writing ? "writing" : "reading",
        fetch ? "instruction" : "    operand",
        switch_to_us);

  ArchThreads::printThreadRegisters(currentThread, false);

    UserThread *pUserThread = reinterpret_cast<UserThread *>(currentThread);
    if (pUserThread->getThreadCancellationState() == UserThread::ISCANCELED &&
        pUserThread->getThreadCancelState() == UserThread::ENABLED &&
        pUserThread->getThreadCancelType() == UserThread::DEFERRED)
    {
        Syscall::pthread_exit(reinterpret_cast<void *>(-1ULL));
    }

    ustl::map<size_t, bool> pages;

    //4 to handle reserved stack pages
    for(int i = 0; i < 4; i++)
    {
        uint32 pos = PageManager::instance()->allocPPN();
        pages[pos] = false;
    }

    if (checkPageFaultIsValid(address, user, present, switch_to_us))
    {
        debug(USERPROCESS, "%18zx", address);

        //-----------------------------------------COW--------------------------------------------------------------------------
        if(currentThread->loader_->arch_memory_.isCowSet(address))
        {
            debug(PAGEFAULT, "[COW] Pagefault happened, cow detected!");

            currentThread->loader_->arch_memory_.cowPageCopy(address, &pages);

            debug(PAGEFAULT, "[COW] Page copied!");
        }
//----------------------------------------------------------------------------------------------------------------------


        if (address > STACK_POS)
        {
            if (((UserThread *) currentThread)->process_->CheckStack(address))
            {
                debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
                return;
            } else
            {
                // the page-fault seems to be faulty, print out the thread stack traces
                ArchThreads::printThreadRegisters(currentThread, true);
                currentThread->printBacktrace(true);
                if (currentThread->loader_)
                {
                    Syscall::exit(9999);
                } else
                    currentThread->kill();
            }
        } else
            currentThread->loader_->loadPage(address);
  }
  else
  {
    // the page-fault seems to be faulty, print out the thread stack traces
    ArchThreads::printThreadRegisters(currentThread, true);
    currentThread->printBacktrace(true);
    if (currentThread->loader_)
    {
        Syscall::exit(9999);
    }
    else
      currentThread->kill();
  }

    if (pUserThread->getThreadCancellationState() == UserThread::ISCANCELED &&
        pUserThread->getThreadCancelState() == UserThread::ENABLED &&
        pUserThread->getThreadCancelType() == UserThread::DEFERRED)
    {
        Syscall::pthread_exit(reinterpret_cast<void *>(-1ULL));
    }

    debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
}

void PageFaultHandler::enterPageFault(size_t address, bool user,
                                      bool present, bool writing,
                                      bool fetch)
{
  assert(currentThread && "You have a pagefault, but no current thread");
  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;

  currentThread->switch_to_userspace_ = 0;
  currentThreadRegisters = currentThread->kernel_registers_;
  ArchInterrupts::enableInterrupts();

  handlePageFault(address, user, present, writing, fetch, saved_switch_to_userspace);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
    currentThreadRegisters = currentThread->user_registers_;
}
