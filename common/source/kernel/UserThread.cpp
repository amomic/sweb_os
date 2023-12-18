#include "Thread.h"
#include "Syscall.h"
#include "PageManager.h"
#include "Loader.h"
#include "UserProcess.h"
#include "ArchThreads.h"
#include "Console.h"
#include "paging-definitions.h"
#include "ustring.h"
#include "UserThread.h"
#include "FileSystemInfo.h"
#include "Scheduler.h"
#include "IPT.h"

//constructor
UserThread::UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
                       [[maybe_unused]]void *(*start_routine)(void *), void *wrapper, size_t tid, [[maybe_unused]]void* argc, [[maybe_unused]]size_t args) :
        Thread(fs_info, filename, Thread::USER_THREAD),
        process_(userProcess),
        tid_(tid),
        state_join_lock_("UserThread::state_join_lock_"),
        join_condition_(&userProcess->return_val_lock_, "UserThread::join_condition_"),
        swap_condition_(&state_join_lock_, "UserTHread::swap_cond"),
        terminal_number_(terminal_number)
{
    start_clock_tcs_ = ArchThreads::rdtsc();
    wrapper_ = wrapper;
    loader_ = userProcess->getLoader();

    size_t stack_ppn= PageManager::instance()->allocPPN();

    this->setTID(tid);

    debug(USERTHREAD, "Thread ID is: %lu \n", tid);

    debug(USERTHREAD, "Before VPN_MAPPED\n");

    bool vpn_mapped = -1;



    loader_->arch_memory_.arch_mem_lock.acquire();
    stack_start =  (USER_BREAK - PAGE_SIZE * tid * STACK_SIZE);
    // UB FF
    // UB - 13
    // UB -26
    stack_end = stack_start - PAGE_SIZE*(STACK_SIZE-1);
    // UB -12
    // UB - 25
    size_t virtual_page = (stack_start/ PAGE_SIZE)-1;
    virtual_pages_.push_back(virtual_page);
    vpn_mapped = loader_->arch_memory_.mapPage(virtual_page, stack_ppn , 1);
    loader_->arch_memory_.arch_mem_lock.release();

    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
    debug(USERTHREAD, "After VPN_MAPPED\n");
    ArchThreads::createUserRegisters(user_registers_, wrapper,
                                     (void *)(stack_start - sizeof (pointer)),
                                     getKernelStackStartPointer());

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    if (start_routine != NULL)
    {
        user_registers_->rdi = reinterpret_cast<size_t>(start_routine);
        user_registers_->rsi = reinterpret_cast<size_t>(argc);
    } else {
        user_registers_->rdi = reinterpret_cast<uint64>(argc);
        user_registers_->rsi = reinterpret_cast<size_t>(args);
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
        swap_condition_(&state_join_lock_, "UserTHread::swap_cond"),
        stack_start(process_thread_pointer.stack_start),
        stack_end(process_thread_pointer.stack_end),
        terminal_number_(terminal_number){

    loader_ = process_->getLoader();

    this->setTID(thread_id);

    debug(USERTHREAD, "Thread ID is: %lu \n", tid_);
    virtual_pages_ = process_thread_pointer.virtual_pages_;

    ArchThreads::createUserRegisters(user_registers_, process_thread_pointer.wrapper_,
                                     (void *)(stack_start-sizeof (pointer)),
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

    start_clock_tcs_ = ArchThreads::rdtsc();

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

uint64 UserThread::getStartClockTcs() const
{
    return start_clock_tcs_;
}

void UserThread::setStartClockTcs(uint64 startClockTime)
{
    start_clock_tcs_ = startClockTime;
}

void UserThread::setProcess(UserProcess *process)
{
    process_ = process;
}

size_t UserThread::getTid() const
{
    return tid_;
}

void UserThread::setTid(size_t tid)
{
    tid_ = tid;
}

size_t UserThread::getOffset() const
{
    return offset_;
}

void UserThread::setOffset(size_t offset)
{
    offset_ = offset;
}

void *UserThread::getWrapper() const
{
    return wrapper_;
}

void UserThread::setWrapper(void *wrapper)
{
    wrapper_ = wrapper;
}

UserThread::THREAD_CANCEL_TYPE UserThread::getThreadCancelType() const
{
    return thread_cancel_type_;
}

void UserThread::setThreadCancelType(UserThread::THREAD_CANCEL_TYPE threadCancelType)
{
    thread_cancel_type_ = threadCancelType;
}

UserThread::THREAD_CANCEL_STATE UserThread::getThreadCancelState() const
{
    return thread_cancel_state_;
}

void UserThread::setThreadCancelState(UserThread::THREAD_CANCEL_STATE threadCancelState)
{
    thread_cancel_state_ = threadCancelState;
}

UserThread::THREAD_CANCELATION UserThread::getThreadCancellationState() const
{
    return thread_cancellation_state_;
}

void UserThread::setThreadCancellationState(UserThread::THREAD_CANCELATION threadCancellationState)
{
    thread_cancellation_state_ = threadCancellationState;
}

const Mutex &UserThread::getStateJoinLock() const
{
    return state_join_lock_;
}

UserThread::DETATCH_STATE UserThread::getTypeOfJoin() const
{
    return type_of_join_;
}

void UserThread::setTypeOfJoin(UserThread::DETATCH_STATE typeOfJoin)
{
    type_of_join_ = typeOfJoin;
}

UserThread *UserThread::getWaitingFor() const
{
    return waiting_for_;
}

void UserThread::setWaitingFor(UserThread *waitingFor)
{
    waiting_for_ = waitingFor;
}

UserThread *UserThread::getWaitedBy() const
{
    return waited_by_;
}

void UserThread::setWaitedBy(UserThread *waitedBy)
{
    waited_by_ = waitedBy;
}

const Condition &UserThread::getJoinCondition() const
{
    return join_condition_;
}

const ustl::vector<size_t> &UserThread::getVirtualPages() const
{
    return virtual_pages_;
}

void UserThread::setVirtualPages(const ustl::vector<size_t> &virtualPages)
{
    virtual_pages_ = virtualPages;
}

size_t UserThread::getStackStart() const
{
    return stack_start;
}

void UserThread::setStackStart(size_t stackStart)
{
    stack_start = stackStart;
}

size_t UserThread::getStackEnd() const
{
    return stack_end;
}

void UserThread::setStackEnd(size_t stackEnd)
{
    stack_end = stackEnd;
}

const ustl::string &UserThread::getFilename() const
{
    return filename_;
}

void UserThread::setFilename(const ustl::string &filename)
{
    filename_ = filename;
}

FileSystemInfo *UserThread::getFsInfo() const
{
    return fs_info_;
}

void UserThread::setFsInfo(FileSystemInfo *fsInfo)
{
    fs_info_ = fsInfo;
}

uint32 UserThread::getTerminalNumber() const
{
    return terminal_number_;
}

void UserThread::setTerminalNumber(uint32 terminalNumber)
{
    terminal_number_ = terminalNumber;
}

size_t UserThread::getJoinTid() const
{
    return join_TID;
}

void UserThread::setJoinTid(size_t joinTid)
{
    join_TID = joinTid;
}
