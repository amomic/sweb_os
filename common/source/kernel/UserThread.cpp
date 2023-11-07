#include "Thread.h"
#include "Syscall.h"
#include "PageManager.h"
#include "Loader.h"
#include "UserProcess.h"
#include "ArchThreads.h"
#include "Console.h"
#include "ustring.h"
#include "UserThread.h"
#include "FileSystemInfo.h"
#include "Scheduler.h"

//constructor
UserThread::UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
                       [[maybe_unused]]void *(*start_routine)(void *), void *wrapper, size_t tid, [[maybe_unused]]void *argc, [[maybe_unused]]size_t args) :
        Thread(fs_info, filename, Thread::USER_THREAD),
        process_(userProcess),
        tid_(tid),
        state_join_lock_("UserThread::state_join_lock_"),
        join_condition_(&userProcess->return_val_lock_, "UserThread::join_condition_"),
        terminal_number_(terminal_number)
{
    wrapper_ = wrapper;
    loader_ = userProcess->getLoader();

    size_t stack_ppn= PageManager::instance()->allocPPN();

    this->setTID(tid);

    debug(USERTHREAD, "Thread ID is: %lu \n", tid);

    debug(USERTHREAD, "Before VPN_MAPPED\n");

    bool vpn_mapped = -1;

    stack_start = USER_BREAK / PAGE_SIZE - (PAGE_MAX * tid) - 1 - (STACK_SIZE*tid);
    stack_end = stack_start - STACK_SIZE;
    loader_->arch_memory_.arch_mem_lock.acquire();
    vpn_mapped = loader_->arch_memory_.mapPage(stack_start, stack_ppn , 1);
    virtual_pages_.push_back(stack_start);
    loader_->arch_memory_.arch_mem_lock.release();
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
    debug(USERTHREAD, "After VPN_MAPPED\n");
    ArchThreads::createUserRegisters(user_registers_, wrapper,
                                     (void *)(stack_start*PAGE_SIZE - sizeof (pointer)),
                                     getKernelStackStartPointer());

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    if (start_routine == NULL)
    {
        user_registers_->rdi = reinterpret_cast<uint64>(argc);
        user_registers_->rsi = reinterpret_cast<size_t>(args);
    }
    else
    {
        user_registers_->rdi = reinterpret_cast<size_t>(start_routine);
        user_registers_->rsi = reinterpret_cast<size_t>(argc);
    }


    if (main_console->getTerminal(terminal_number_))
        setTerminal(main_console->getTerminal(terminal_number_));

    switch_to_userspace_ = 1;

}

//copy constructor
UserThread::UserThread(const UserThread  &process_thread_pointer, UserProcess *parent_process, uint32 terminal_number,
                        ustl::string filename, FileSystemInfo *fs_info, size_t thread_id):
        Thread(fs_info, filename, Thread::USER_THREAD),
        process_(parent_process),
        tid_(thread_id),
        state_join_lock_("UserThread::state_join_lock_"),
        join_condition_(&parent_process->return_val_lock_, "UserThread::join_condition_"),
        stack_start(process_thread_pointer.stack_start),
        stack_end(process_thread_pointer.stack_end),
        terminal_number_(terminal_number){

    loader_ = process_->getLoader();

    this->setTID(thread_id);

    debug(USERTHREAD, "Thread ID is: %lu \n", tid_);
    virtual_pages_ = process_thread_pointer.virtual_pages_;

    ArchThreads::createUserRegisters(user_registers_, process_thread_pointer.wrapper_,
                                     (void *)(stack_start*PAGE_SIZE - sizeof (pointer)),
                                     getKernelStackStartPointer());

    memcpy(this->user_registers_, currentThread->user_registers_, sizeof(ArchThreadRegisters));

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    // do we need this?
    //process_->loader_->arch_memory_.arch_mem_lock.release();

    user_registers_->rax = 0;
    user_registers_->rsp0 = (size_t)getKernelStackStartPointer();

    if (main_console->getTerminal(terminal_number_))
        setTerminal(main_console->getTerminal(terminal_number_));

    switch_to_userspace_ = 1;

}


UserThread::~UserThread()
{
    debug(USERTHREAD, "DESTRUCTOR: UserThread %zu in UserProcess %s\n", tid_, filename_.c_str());
    process_->CleanThreads(tid_);

}

void UserThread::Run()
{
    assert(false);
}

UserProcess* UserThread::getProcess()
{
    return process_;
}

void UserThread::setJoinTID(size_t tid) {
    join_TID = tid;
}

size_t UserThread::getJoinTID()
{
    return join_TID;
}

void UserThread::makeAsynchronousCancel(){
    thread_cancel_type_ = UserThread::ASYNCHRONOUS;
    thread_cancel_state_ = UserThread::ENABLED;
    thread_cancellation_state_ = UserThread::ISCANCELED;
}
