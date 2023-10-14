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
        threads_alive_(0), fd_(VfsSyscall::open(filename, O_RDONLY)),filename_(filename), fs_info_(fs_info),
        terminal_number_(terminal_number)
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
    threads_lock_.acquire();
    Scheduler::instance()->addNewThread(user_thread);
    threads_map_.push_back(ustl::make_pair(0,user_thread));
    threads_alive_++;
    threads_lock_.release();


}

//copyconst
UserProcess::UserProcess(UserProcess &parent_process, UserThread &current_thread, size_t process_id):
        threads_lock_("UserProcess::threads_lock_"),
        pages_lock_("UserProcess::pages_lock_"),
        pid_(process_id),
        threads_alive_(0),
        fd_(VfsSyscall::open(parent_process.filename_, O_RDONLY)), working_dir_(new FileSystemInfo(*parent_process.working_dir_)),
        filename_(parent_process.filename_),
        terminal_number_(parent_process.terminal_number_)
{
    ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

    if (fd_ >= 0)
        loader_ = new Loader(*parent_process.loader_, fd_);

    if (!loader_ || !loader_->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "Error: loading %s failed!\n", filename_.c_str());
        delete this;
        return;
    }


    UserThread *user_thread = new  UserThread(current_thread, this, terminal_number_, filename_, fs_info_, current_thread.tid_);

    assert(user_thread && "thread creation failed for forked process \n");
    threads_lock_.acquire();
    Scheduler::instance()->addNewThread(user_thread);
    threads_map_.push_back(ustl::make_pair(user_thread->tid_,user_thread));
    threads_alive_++;
    threads_lock_.release();

    ProcessRegistry::instance()->process_lock_.acquire();
    ProcessRegistry::instance()->process_map_.push_back(ustl::make_pair(pid_, this));
    ProcessRegistry::instance()->process_lock_.release();


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
    Thread *new_thread = new UserThread(filename_, fs_info_, terminal_number_, process_, start_routine, wrapper,threads_counter_for_id_, (void*)argc, args);
    assert(new_thread && "Failed to create new thread\n");

    Scheduler::instance()->addNewThread(new_thread);
    threads_alive_++;
    threads_map_.push_back(ustl::make_pair(threads_counter_for_id_,new_thread));
    threads_lock_.release();
    return (UserThread*)new_thread;

}


void UserProcess::CleanThreads(size_t thread)
{
    assert(Scheduler::instance()->isCurrentlyCleaningUp());
    threads_alive_--;
    if(threads_alive_ == 0)
    {
        debug(USERTHREAD, "no threads left\n");
        threads_map_.erase(thread);
        delete this;
    }
}