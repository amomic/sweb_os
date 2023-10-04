#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "Thread.h"
#include "Console.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "ArchThreads.h"
#include "offsets.h"
#include "Scheduler.h"
#include "UserThread.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) :
        threads_lock_("UserProcess::threads_lock_"), pages_lock_("UserProcess::pages_lock_"),
        fd_(VfsSyscall::open(filename, O_RDONLY)), filename_(filename),fs_info_(fs_info), terminal_number_(terminal_number)
{
    ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

    if (fd_ >= 0)
        loader_ = new Loader(fd_);

    if (!loader_ || !loader_->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
        delete this;
        return;
    }


    UserThread *user_thread = new UserThread(filename,fs_info,terminal_number, this, NULL, loader_->getEntryFunction(), 0, 0, NULL);

    assert(user_thread && "thread creation failed\n");
    Scheduler::instance()->addNewThread(user_thread);
    threads_map_.push_back(ustl::make_pair(0,user_thread));


}

UserProcess::~UserProcess()
{
    assert(Scheduler::instance()->isCurrentlyCleaningUp());
    delete loader_;
    loader_ = 0;

    if (fd_ > 0)
        VfsSyscall::close(fd_);

    delete working_dir_;
    working_dir_ = 0;

    ProcessRegistry::instance()->processExit();
}

void UserProcess::Run()
{
    debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
    assert(false);
}

Loader* UserProcess::getLoader()
{
    return loader_;
}

UserThread* UserProcess::createThread(size_t* thread, [[maybe_unused]]size_t *attr, void*(*start_routine)(void*), void *wrapper,uint64 argc,size_t args)
{
    debug(USERPROCESS, "New thread creation\n");

    threads_lock_.acquire();
    if(!start_routine){
        wrapper = loader_->getEntryFunction();
    }
    threads_counter_for_id_++;
    *thread = threads_counter_for_id_;
    process_ = this;

    debug(USERPROCESS, "Calling UserThread constructor!\n");
    Thread *new_thread = new UserThread(filename_, fs_info_, terminal_number_, process_, start_routine, wrapper,
                                        threads_counter_for_id_, (void*)argc, args);
    assert(new_thread && "Failed to create new thread\n");

    Scheduler::instance()->addNewThread(new_thread);
    threads_map_.push_back(ustl::make_pair(threads_counter_for_id_,new_thread));
    threads_alive_.fetch_add(1);
    threads_lock_.release();
    return (UserThread*)new_thread;

}


void UserProcess::unmapPage()
{
    if( loader_->arch_memory_.checkAddressValid(virtual_pages_*PAGE_SIZE))
    {
        loader_->arch_memory_.unmapPage(virtual_pages_);
    }
}