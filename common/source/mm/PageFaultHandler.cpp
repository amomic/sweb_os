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
#include "IPT.h"

extern "C" void arch_contextSwitch();

const size_t PageFaultHandler::null_reference_check_border_ = PAGE_SIZE;

inline bool PageFaultHandler::checkPageFaultIsValid(size_t address, bool user,
                                                    bool present, bool switch_to_us, bool writing)
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
  else if(present && writing)
  {
    debug(PAGEFAULT, "You got a pagefault even though the address is mapped, but we are trying to write to it so its OK!\n");
    //because of cow, this might be valid now
    return true;
  }
  else if(present)
  {
      debug(PAGEFAULT, "You got a pagefault even though the address is mapped.\n");
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

    auto parent_ = static_cast<UserThread*>(currentThread);


    debug(SYSCALL, "1\n");


    debug(SYSCALL, "2\n");
    if (checkPageFaultIsValid(address, user, present, switch_to_us, writing))
    {
        debug(SYSCALL, "3\n");

        ustl::map<size_t, bool> pages;

        //4 to handle reserved stack pages
        for(int i = 0; i < 4; i++)
        {
            uint32 pos = PageManager::instance()->allocPPN();
            pages.push_back(ustl::pair(pos, false));
        }
        IPT::instance()->ipt_lock_.acquire();
        parent_->getProcess()->arch_mem_lock_.acquire();
        //race condition
        auto m = parent_->getProcess()->loader_->arch_memory_.resolveMapping(address / PAGE_SIZE);

        if(m.pt && m.pt[m.pti].swapped)
        {
            parent_->getProcess()->arch_mem_lock_.release();
            IPT::instance()->ipt_lock_.release();
            debug(SWAP_THREAD, "VPN: %zx \n",address );
            //assert(0 && "You need to swap this in \n");
            for(auto page : pages)
            {
                if(page.second == false) {
                    page.second = true;
                    debug(SWAP_THREAD, "FREE PPN %zu\n", page.first);
                    PageManager::instance()->freePPN(page.first);
                }
            }
            SwapThread::instance()->WaitForSwapIn(address / PAGE_SIZE, m);

            return;
        }

        //-----------------------------------------COW--------------------------------------------------------------------------
        if(currentThread->loader_->arch_memory_.isCowSet(address) && writing)
        {
            debug(PAGEFAULT, "[COW] Pagefault happened, cow detected!\n");

            currentThread->loader_->arch_memory_.cowPageCopy(address, &pages);
            parent_->getProcess()->arch_mem_lock_.release();
            IPT::instance()->ipt_lock_.release();

            debug(PAGEFAULT, "[COW] Cow pagefault handled!\n");
            for(auto page : pages)
            {
                if(!pages.empty() && page.second == false ) {
                    page.second = true;
                    debug(SWAP_THREAD, "FREE PPN %zu\n", page.first);
                    PageManager::instance()->freePPN(page.first);
                }
            }
            return;

        }

//----------------------------------------------------------------------------------------------------------------------


/*-------------------------------------------------------- HEAP ON DEMAND ----------------------------------------------*/
        currentThread->loader_->heap_mutex_.acquire();

        debug(SYSCALL, "4\n");
        if ( address <= currentThread->loader_->current_break_ && address >= currentThread->loader_->start_break_)
        {
            size_t vpn = address / PAGE_SIZE;
            debug(PAGEFAULT, "Heap adress pagefault->%zx \n", address);
            currentThread->loader_->vpns_of_heap_.push_back();
            currentThread->loader_->heap_mutex_.release();

            // No locks?
            bool is_mapped = pUserThread->getProcess()->loader_->arch_memory_.mapPage(vpn, &pages, 1);
            parent_->getProcess()->arch_mem_lock_.release();
            IPT::instance()->ipt_lock_.release();
//            currentThread->loader_->arch_memory_.arch_mem_lock.release();
            if(!is_mapped)
            {
                currentThread->loader_->vpns_of_heap_.pop_back();
            }
            for(auto page : pages)
            {
                if(!pages.empty() && page.second == false ) {
                    page.second = true;
                    debug(SWAP_THREAD, "FREE PPN %zu\n", page.first);
                    PageManager::instance()->freePPN(page.first);
                }
            }
            debug(PAGEFAULT, "Heap Address Page fault finished -> %18zx.\n", address);
            return;
        }

        if(currentThread->loader_->heap_mutex_.isHeldBy(currentThread))
             currentThread->loader_->heap_mutex_.release();

/*-------------------------------------------------------- HEAP ON DEMAND END!! ----------------------------------------------*/

        debug(USERPROCESS, "%18zx\n", address);
        debug(SYSCALL, "6\n");

        if (address > STACK_POS)
        {

            debug(SYSCALL, "7\n");

            if (((UserThread *) currentThread)->process_->CheckStack(address, &pages))
            {
                parent_->getProcess()->arch_mem_lock_.release();
                IPT::instance()->ipt_lock_.release();
                debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
                for(auto page : pages)
                {
                    if(!pages.empty() && page.second == false ) {
                        page.second = true;
                        debug(SWAP_THREAD, "FREE PPN %zu\n", page.first);
                        PageManager::instance()->freePPN(page.first);
                    }
                }
                return;
            } else
            {
                parent_->getProcess()->arch_mem_lock_.release();
                IPT::instance()->ipt_lock_.release();
                for(auto page : pages)
                {
                    if(!pages.empty() && page.second == false ) {
                        page.second = true;
                        debug(SWAP_THREAD, "FREE PPN %zu\n", page.first);
                        PageManager::instance()->freePPN(page.first);
                    }
                }
                debug(SYSCALL, "8\n");
                // the page-fault seems to be faulty, print out the thread stack traces
                ArchThreads::printThreadRegisters(currentThread, true);
                currentThread->printBacktrace(true);
                if (currentThread->loader_)
                {
                    debug(SYSCALL, "9\n");
                    Syscall::exit(9999);
                } else {
                    debug(SYSCALL, "10\n");
                    currentThread->kill();
                }
            }
        } else
        {
            debug(SYSCALL, "11\n");

            parent_->getProcess()->arch_mem_lock_.release();
            IPT::instance()->ipt_lock_.release();


            currentThread->loader_->loadPage(address, &pages);


        }

  }
  else
  {

      debug(SYSCALL, "12\n");

    // the page-fault seems to be faulty, print out the thread stack traces
    ArchThreads::printThreadRegisters(currentThread, true);
    currentThread->printBacktrace(true);
    if (currentThread->loader_)
    {
        debug(SYSCALL, "13\n");
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


    if(currentThread->loader_->heap_mutex_.isHeldBy(currentThread))
        currentThread->loader_->heap_mutex_.release();
    if(IPT::instance()->ipt_lock_.isHeldBy(currentThread))
        IPT::instance()->ipt_lock_.release();


    debug(SYSCALL,"okejjjjj");
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

    if(currentThread->loader_->arch_memory_.process_->arch_mem_lock_.isHeldBy(currentThread))
        currentThread->loader_->arch_memory_.process_->arch_mem_lock_.release();
    if(currentThread->loader_->heap_mutex_.isHeldBy(currentThread))
        currentThread->loader_->heap_mutex_.release();
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
    currentThreadRegisters = currentThread->user_registers_;
}

[[maybe_unused]]bool PageFaultHandler::handleZeroPageDeduplication(ArchMemoryMapping* m, bool write_access)
{
    debug(PAGEFAULT, "\n\n\n In handle zero page deduplication!\n\n\n");
    if(m->pt && m->pt[m->pti].page_ppn == PageManager::instance()->zeroPPN)
    {
        return true;
    }

    if(m->pt && m->page_ppn == 0 && !write_access)
    {
        debug(PAGEFAULT, "\n\n\nZero page deduplication started!\n\n\n");

        m->pt[m->pti].present = 0;
        m->pt[m->pti].page_ppn = PageManager::instance()->zeroPPN;
        m->pt[m->pti].writeable = 0;
        m->pt[m->pti].cow = 1;
        m->pt[m->pti].present = 1;

        PageManager::instance()->zero_cnt++;

        return true;
    }

    return false;
}
