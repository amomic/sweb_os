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
    loader_ = userProcess->getLoader();
    size_t stack_ppn= PageManager::instance()->allocPPN();

    this->setTID(tid);

    debug(USERPROCESS, "Thread ID is: %lu \n", tid);

    debug(USERPROCESS, "Before VPN_MAPPED\n");
    //virtual_pages_+= STACK_PAGES + 1;
    //size_t stack_vpn = loader_->arch_memory_.getIdentAddressOfPPN(stack_ppn,4096);
    //debug(USERPROCESS, "%10zx: adresa\n",((USER_BREAK / PAGE_SIZE - 1) - (id*(8+1) )));
    bool vpn_mapped = -1;

    if(tid == 0)
    {
        vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, stack_ppn , 1);
    }
    else
    {
        vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - (PAGE_MAX * tid + 2) - 1, stack_ppn , 1);
    }

    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
    debug(USERPROCESS, "After VPN_MAPPED\n");
    //size_t stack_start = (stack_vpn << 12 | 0xfff) - sizeof(pointer);
    ArchThreads::createUserRegisters(user_registers_, wrapper,
                                     (void*) (USER_BREAK - sizeof(pointer)),
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

