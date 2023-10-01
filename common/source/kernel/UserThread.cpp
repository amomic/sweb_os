#include "Thread.h"
#include "Syscall.h"
#include "PageManager.h"
#include "Loader.h"
#include "UserProcess.h"
#include "ArchThreads.h"
#include "Console.h"
#include "../../include/ustl/ustring.h"
#include "../../include/kernel/UserThread.h"
#include "../../include/fs/FileSystemInfo.h"
#include "../../include/mm/PageManager.h"
#include "../../../arch/x86/64/include/ArchThreads.h"

//constructor
UserThread::UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
                       void *(*start_routine)(void *), void *wrapper, size_t tid, void *argc, size_t args) :
        Thread(fs_info, filename, Thread::USER_THREAD),
        process_(userProcess),
        tid_(tid),
        terminal_number_(terminal_number)
{
    loader_ = process_->loader_;
    size_t stack_ppn= PageManager::instance()->allocPPN();
    debug(USERPROCESS, "ok1");
    process_->pages_lock_.acquire();
    virtual_pages_+= STACK_PAGES + 1;
    size_t stack_vpn = USER_BREAK / PAGE_SIZE - virtual_pages_;
    //debug(USERPROCESS, "%10zx: adresa\n",((USER_BREAK / PAGE_SIZE - 1) - (id*(8+1) )));
    bool vpn_mapped = loader_->arch_memory_.mapPage((USER_BREAK / PAGE_SIZE - 1) , stack_ppn , 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
    debug(USERPROCESS, "ok2");
    process_->pages_lock_.release();
    size_t stack_start = (stack_vpn << 12 | 0xfff) - sizeof(pointer);
    ArchThreads::createUserRegisters(user_registers_, wrapper,
                                     (void*) stack_start,
                                     getKernelStackStartPointer());

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);


    if(!start_routine)
    {
        user_registers_->rdi = reinterpret_cast<uint64>(argc);
        user_registers_->rsi = reinterpret_cast<uint64>(args);
    }

    if(start_routine)
    {
        user_registers_->rdi = reinterpret_cast<uint64>(start_routine);
        user_registers_->rsi = reinterpret_cast<uint64>(argc);
    }

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;

}

//copy constructor
/*UserThread::UserThread([[maybe_unused]]const UserThread  &process_thread_pointer, UserProcess *parent_process, uint32 terminal_number,
                       ustl::string filename, FileSystemInfo *fs_info, size_t thread_id):
                       Thread(fs_info, filename, Thread::USER_THREAD),
                       terminal_number_(terminal_number),
                       process_(parent_process),
                       tid_(thread_id){

}*/

void UserThread::Run()
{
    assert(false);
}

UserProcess* UserThread::getProcess()
{
    return process_;
}

