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
#include "Syscall.h"
#include "UserThread.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) :
        threads_lock_("UserProcess::threads_lock_"), pages_lock_("UserProcess::pages_lock_"),
        return_val_lock_("UserProcess::return_val_lock_"), semaphore_init("UserProcess::semaphore_init",1),
        process_wait_cond_("Semaphore:process_wait_cond_",0), threads_alive_(0),
        fd_(VfsSyscall::open(filename, O_RDONLY)), filename_(filename),
        fs_info_(fs_info), terminal_number_(terminal_number)
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
    pid_ = process_count_;
    ProcessRegistry::instance()->process_map_.push_back(ustl::make_pair(pid_, this));
    threads_alive_++;
    threads_lock_.release();


}

//copyconst
UserProcess::UserProcess(UserProcess &parent_process, UserThread &current_thread, size_t process_id):
        threads_counter_for_id_(parent_process.threads_counter_for_id_),
        threads_lock_("UserProcess::threads_lock_"),
        pages_lock_("UserProcess::pages_lock_"),
        return_val_lock_("UserProcess::return_val_lock_"),
        semaphore_init("UserProcess::semaphore_init",1),
        process_wait_cond_("Semaphore:process_wait_cond_",0),pid_(process_id),
        fd_(VfsSyscall::open(parent_process.filename_, O_RDONLY)),
        filename_(parent_process.filename_),
        terminal_number_(parent_process.terminal_number_)

{
    debug(USERPROCESS, "Im in userprocess copy constructor!\n");

    ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

    if (fd_ >= 0)
    {
        debug(USERPROCESS, "pid -> %lu \n", parent_process.pid_);
        loader_ = new Loader(*parent_process.loader_, fd_);
    }

    if (!loader_ || !loader_->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "Error: loading %s failed!\n", filename_.c_str());
        // TODO do we need this?
        parent_process.loader_->arch_memory_.arch_mem_lock.release();
        delete this;
        return;
    }


    /*threads_lock_.acquire();
    size_t new_tid = threads_map_.size() + 1;
    while(threads_map_.find(new_tid))
    {
        new_tid = new_tid+1;
    }
    threads_lock_.release();*/

    UserThread *user_thread = new  UserThread(current_thread, this, terminal_number_, filename_, fs_info_, current_thread.tid_ );

    assert(user_thread && "thread creation failed for forked process \n");
    threads_lock_.acquire();
    Scheduler::instance()->addNewThread(user_thread);
    threads_map_.push_back(ustl::make_pair(current_thread.tid_,user_thread));
    threads_alive_++;
    threads_lock_.release();

    ProcessRegistry::instance()->process_lock_.acquire();
    ProcessRegistry::instance()->process_map_.push_back(ustl::make_pair(pid_, this));
    ProcessRegistry::instance()->process_lock_.release();


}


UserProcess::~UserProcess()
{
    assert(Scheduler::instance()->isCurrentlyCleaningUp());
    ProcessRegistry::instance()->process_lock_.acquire();
    for( auto it: ProcessRegistry::instance()->process_map_)
    {
        if(pid_ == it.second->target)
        {
            debug(USERPROCESS, "aloha \n");
            it.second->process_wait_cond_.post();
        }
    }
    ProcessRegistry::instance()->process_map_.erase(pid_);
    ProcessRegistry::instance()->process_lock_.release();
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
    ArchThreads::atomic_add(reinterpret_cast<int64 &>(threads_counter_for_id_), 1);
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
    threads_map_.erase(thread);
    if(threads_alive_ == 0)
    {
        debug(USERTHREAD, "no threads left\n");
        delete this;
    }

}

size_t UserProcess::joinThread(size_t thread, pointer return_val) {
    debug(SYSCALL, "joinThread started");
    kprintf("*** JOIN STARTED!\n");
    if((size_t)return_val >= USER_BREAK) // Check if return_val is valid
        return -1ULL;

    if(currentThread->getTID() == thread) // Self-join check
        return -1ULL;
    debug(SYSCALL, "Initial checks in joinThread done");

    UserThread* current_thread = reinterpret_cast<UserThread*>(currentThread);
    auto current_process = reinterpret_cast<UserThread*>(currentThread)->getProcess();

    auto thread_map_entry = threads_map_.find(thread);
    UserThread* joining_thread = reinterpret_cast<UserThread*>(thread_map_entry->second);

    current_process->threads_lock_.acquire();
    debug(SYSCALL, "After locking hreads_lock_ and before looking into map in joinThraed");

    if(threads_map_.find(thread) == threads_map_.end() || joining_thread->getJoinTID() != 0){ // Check if thread in map and if joinable
        current_process->threads_lock_.release();
        current_process->return_val_lock_.acquire();
        debug(SYSCALL, "Checking if thread is in the map - joinThraed");

        auto thread_retval_map_entry = current_process->thread_retval_map.find(thread);

        if(thread_retval_map_entry == current_process->thread_retval_map.end()){ // retval not found in list
            debug(SYSCALL, "retval not found in the map - joinThraed");
            kprintf("retval not found in list - exiting -1\n");
            current_process->return_val_lock_.release();
            return -1ULL;
        } else { // retval found in list
            debug(SYSCALL, "retval found in the map - joinThread");
            kprintf("retval found in map\n");
            if(return_val == NULL){
                thread_retval_map_entry = current_process->thread_retval_map.find(thread);
                if(thread_retval_map_entry != current_process->thread_retval_map.end())
                    current_process->thread_retval_map.erase(thread);
                current_process->return_val_lock_.release();
                kprintf("Thread already finished - returning 0\n");
                return 0;
            } else {
                if(thread_retval_map_entry != current_process->thread_retval_map.end()) {
                    *(size_t *) return_val = (size_t) thread_retval_map_entry->second;
                    current_process->thread_retval_map.erase(thread);
                    debug(SYSCALL, "retval is set - joinThraed");
                    kprintf("Thread already finished - retval is set! returning 0\n");
                }
                current_process->return_val_lock_.release();
                return 0;
            }
        }
    } else {    // Found in map
        debug(SYSCALL, "thread found in the map - joinThread");
        if (joining_thread->type_of_join_ == UserThread::DETATCH_STATE::JOINABLE){
            debug(SYSCALL, "Thread is joinable - joinThread");
            kprintf("Thread is joinable!\n");
            if (current_thread->waited_by_ == nullptr && joining_thread->waited_by_ == nullptr){    // Deadlock check
                if(joining_thread->waiting_for_ != current_thread){
                    current_thread->waiting_for_ = joining_thread;
                    joining_thread->waited_by_ = current_thread;
                    current_process->threads_lock_.release();
                } else {    // If deadlock detected
                    debug(SYSCALL, "deadlock - joinThread");
                    kprintf("ERROR - DEADLOCK! returning -1\n");
                    current_process->threads_lock_.release();
                    return -1ULL;
                }
            } else {    // Waited by some other thread
                debug(SYSCALL, "Waited by some other thread - joinThread");
                kprintf("ERROR - Waited by some other thread! - returning error -1\n");
                current_process->threads_lock_.release();
                return -1ULL;
            }
        } else {   // Is detached
            debug(SYSCALL, "Thread is detached - joinThread");
            kprintf("ERROR - thread is detached! -1\n");
            current_process->threads_lock_.release();
            return -1ULL;
        }

        joining_thread->setJoinTID(current_thread->getTID());

        current_process->return_val_lock_.acquire();                // Checking if thread is finished
        auto thread_retval_map_entry = current_process->thread_retval_map.find(thread);

        if (current_process->thread_retval_map.end() != thread_retval_map_entry){
            if (return_val != NULL){
                thread_retval_map_entry = current_process->thread_retval_map.find(thread);
                if (current_process->thread_retval_map.end() != thread_retval_map_entry){
                    *(size_t*)return_val = (size_t)thread_retval_map_entry->second;
                    current_process->thread_retval_map.erase(thread);
                }
                joining_thread->waited_by_ = nullptr;
                current_thread->waiting_for_ = nullptr;
                current_process->return_val_lock_.release();
            } else {
                if (current_process->thread_retval_map.end() != thread_retval_map_entry)
                    current_process->thread_retval_map.erase(thread);
                joining_thread->waited_by_ = nullptr;
                current_thread->waiting_for_ = nullptr;
                current_process->return_val_lock_.release();
            }               // Thread already finished
            debug(SYSCALL, "Thread already finished - joinThread");
            kprintf("Thread already finished and retval set - SUCESS! 0\n");
            return 0;
        } else {
            debug(SYSCALL, "Thread not finished - waiting - joinThread");
            kprintf("Thread not finished - waiting - joinThread\n");
            current_thread->join_condition_.wait(); // Thread not finished, waiting for finish

            if (return_val != NULL){ // Has finished
                thread_retval_map_entry = thread_retval_map.find(thread);
                if (current_process->thread_retval_map.end() != thread_retval_map_entry){
                    *(size_t*) return_val = (size_t)thread_retval_map_entry->second;
                    debug(SYSCALL, "retval is set - joinThread");
                    current_process->thread_retval_map.erase(thread);
                }
                current_process->return_val_lock_.release();
            } else {
                thread_retval_map_entry = thread_retval_map.find(thread);
                if (current_process->thread_retval_map.end() != thread_retval_map_entry){
                    current_process->thread_retval_map.erase(thread);
                }
                current_process->return_val_lock_.release();
            }
            kprintf("THREAD ALREADY FINISHED! 0\n");
            return 0;
        }
    }
}

size_t UserProcess::detachThread(size_t thread) {
    kprintf("DETACH STARTED!\n");
    auto current_thread = reinterpret_cast<UserThread*>(currentThread);
    auto current_process = reinterpret_cast<UserThread*>(current_thread)->getProcess();

    threads_lock_.acquire();

    auto thread_map_entry = current_process->threads_map_.find(thread);

    if (threads_map_.find(thread) == threads_map_.end()){
        kprintf("Does not exist!\n");
        threads_lock_.release();
        return -1ULL;
    }

    auto detach_thread = reinterpret_cast<UserThread*>(thread_map_entry->second);

    if (detach_thread->waited_by_ != nullptr){
        detach_thread->waited_by_->join_condition_.signal();

        detach_thread->waited_by_->waiting_for_ = nullptr;
        detach_thread->waited_by_ = nullptr;
    }

    detach_thread->state_join_lock_.acquire();

    if (detach_thread->type_of_join_ == UserThread::JOINABLE){
        detach_thread->type_of_join_ = UserThread::DETATCHED;
        detach_thread->state_join_lock_.release();
        threads_lock_.release();
        kprintfd("Detached state set!\n");
        return 0;
    } else {
        detach_thread->state_join_lock_.release();
        return -1ULL;
    }
    threads_lock_.release();
    return 0;
}

void UserProcess::unmapPage() {

    auto currenThread = reinterpret_cast<UserThread*>(currentThread);
    for(auto it : currenThread->virtual_pages_)
    {
        size_t valid = currenThread->loader_->arch_memory_.checkAddressValid(it*PAGE_SIZE);
        if(valid)
            currentThread->loader_->arch_memory_.unmapPage(it);
    }
}

size_t UserProcess::exec(char* path, char* const* argv){

    size_t args_num = checkExecArgs(argv);

    if (args_num == (size_t)-1)
        return -1;

    debug(USERPROCESS, "[Exec] In UserProcess!");

    size_t path_len = strlen(path);

    if((size_t)path == NULL ||
       (size_t)path >= USER_BREAK)
    {
        debug(USERPROCESS, "[Exec] Path wrong in UserProcess!");
        return -1;
    }

    if(path_len >= 256)
    {
        debug(USERPROCESS, "[Exec] Path len too big!");
        return -1;
    }

    //Copy pathname
    char copy_array[256];
    memcpy(copy_array, path, path_len);
    copy_array[path_len] = 0;

    char* kernel_path = new char[path_len + 1];
    memcpy(kernel_path, copy_array, path_len);
    kernel_path[path_len] = 0;

    //Get a thread
    UserThread* user_thread = (UserThread*)currentThread;

    //Set old and new fd
    int32 new_fd = VfsSyscall::open(kernel_path, O_RDONLY);
    int32 old_fd = fd_;

    if(new_fd == -1)
    {
        debug(USERPROCESS, "[Exec] Couldn't open new fd!");
        VfsSyscall::close(new_fd);
        delete[] kernel_path;
        return -1U;
    }

    //Set old and new loader
    Loader* old_loader = loader_;
    Loader* new_loader = new Loader(new_fd);

    if(!new_loader->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "[Exec] Creation of new loader failed!");
        VfsSyscall::close(new_fd);
        delete[] kernel_path;
        delete new_loader;
        return -1U;
    }

    if(args_num != 0){
        // Allocate a physical page for arguments
        uint64 args_page = PageManager::instance()->allocPPN();

        // Map the virtual page to the physical page
        uint64 virtual_page = 0;
        bool vpn_map = new_loader->arch_memory_.mapPage(virtual_page, args_page, 1);
        assert(vpn_map && "DOES NOT MAP ARGS PAGE!");

        // Calculate the ident address
        uint64 ident_addr = loader_->arch_memory_.getIdentAddressOfPPN(args_page, PAGE_SIZE);
        size_t num_of_args = args_num + 1;

        // Initialize virtual and physical addresses
        uint64 start_p = (num_of_args * sizeof(char*)) + ident_addr;
        uint64 start_v = (num_of_args * sizeof(char*));

        size_t i = 0, j = 0;

        char* const* args = argv;

        for (i = 0;i < args_num; i++) {
            *reinterpret_cast<uint64*>(ident_addr) = start_v;
            ident_addr += sizeof(pointer);
            j = 0;
            while (args[i][j] != '\0') {
                uint64 tmp_addr = start_p + j;
                *reinterpret_cast<char*>(tmp_addr) = args[i][j];
                j++;
            }

            *reinterpret_cast<char*>(start_p + j) = 0;

            uint64 increment = sizeof(char) * (j + 1);
            start_v += increment;
            start_p += increment;
        }
    }

    deleteAllThreadsExceptCurrent(user_thread);

    while(threads_alive_ > 1) //TODO check if we decrease this number anywhere?
    {
        Scheduler::instance()->yield();
    }

    //TODO delete this if
    if(old_fd == NULL || old_loader == NULL || user_thread == NULL)
    {
        debug(USERPROCESS, "This is just to avoid 'unused variable' error");
    }

    loader_ = new_loader;
    currentThread->loader_ = new_loader;

    ArchThreads::setAddressSpace(currentThread, loader_->arch_memory_);
    Scheduler::instance()->yield();

    fd_ = new_fd;
    VfsSyscall::close(old_fd);
    delete old_loader;

    //Clear retval map
    threads_lock_.acquire();
    thread_retval_map.clear();
    threads_lock_.release();

    //set new tid
    threads_lock_.acquire();
    ArchThreads::atomic_add(reinterpret_cast<int64 &>(threads_counter_for_id_), 1);
    size_t new_tid = threads_counter_for_id_;
    threads_lock_.release();

    //TODO set RDI, RSI registers

    //new thread
    UserThread* new_user_thread = new UserThread(filename_, fs_info_, terminal_number_, this,
                                                 NULL, loader_->getEntryFunction(), new_tid, (void*)args_num, NULL);

    debug(USERPROCESS, "[Exec] Created a new thread!");

    // set new thread tid
    new_user_thread->setTID(new_tid);

    //add new thread and tid to map
    threads_lock_.acquire();
    threads_map_.push_back(ustl::make_pair(new_tid, new_user_thread));
    threads_alive_++;
    threads_lock_.release();

    debug(USERPROCESS, "[Exec] New process created!");

    //add new thread
    Scheduler::instance()->addNewThread(new_user_thread);

    //delete old kernel path and kill old thread
    delete[] kernel_path;
    threads_lock_.acquire();
    user_thread->makeAsynchronousCancel();
    threads_lock_.release();
    return 0;
}

void UserProcess::deleteAllThreadsExceptCurrent(UserThread* current_thread)
{
    auto calling_thread = reinterpret_cast<UserThread*>(current_thread);
    auto calling_process = calling_thread->getProcess();

    ustl::map<size_t, Thread*>::iterator it;

    calling_process->threads_lock_.acquire();

    for(it = calling_process->threads_map_.begin(); it !=calling_process->threads_map_.end(); it++){
        if(it->second == calling_thread)
            continue;
        auto handler = reinterpret_cast<UserThread*>(it->second);
        handler->makeAsynchronousCancel();
    }
    calling_process->threads_lock_.release();
}

size_t UserProcess::checkExecArgs(char *const *args) {
    size_t check_arg_size = 0;
    size_t number_of_args = 0;
    size_t itterator;

    if (args == nullptr) {
        return 0;
    }

    for (itterator = 0; args[itterator] != nullptr; itterator++) {
        if (reinterpret_cast<size_t>(args[itterator]) >= USER_BREAK)
            return -1;

        size_t tmp_size = 0;
        while (args[itterator][tmp_size] != '\0') {
            tmp_size++;
        }

        if(strlen(args[itterator]) > 200)
            return -1;

        if (reinterpret_cast<size_t>(args[itterator] + tmp_size + 1) >= USER_BREAK)
            return -1;

        check_arg_size += tmp_size + 1;
    }

    number_of_args = itterator;
    check_arg_size += itterator * sizeof(size_t);

    if (check_arg_size >= PAGE_SIZE)
        return static_cast<size_t>(-1);

    if(number_of_args > 16)
        return -1;

    return number_of_args;
}

bool UserProcess::CheckStack(size_t pos)
{
    auto thread = ((UserThread*)currentThread);
    for (auto it: threads_map_)
    {
        debug(USERPROCESS , "%18zx", pos);
        debug(USERPROCESS , "%18zx", thread->stack_start);
        debug(USERPROCESS , "%18zx", thread->stack_end);
        if(pos/PAGE_SIZE <= thread->stack_start && pos/PAGE_SIZE > thread->stack_end)
        {
            debug(USERPROCESS , "%18zx", pos);
            debug(USERPROCESS , "%18zx", thread->stack_start);
            debug(USERPROCESS , "%18zx", thread->stack_end);

            size_t ppn = PageManager::instance()->allocPPN();
             bool mapped = it.second->loader_->arch_memory_.mapPage(pos/PAGE_SIZE, ppn, true);
            if(!mapped)
            {
                PageManager::instance()->freePPN(ppn);
            }
            else
            {
                thread->virtual_pages_.push_back(pos/PAGE_SIZE);
            }
            return true;
        }
    }


    return false;
}

pid_t UserProcess::waitpid(pid_t pid, int *status, [[maybe_unused]] int options)
{
    ProcessRegistry::instance()->process_lock_.acquire();
    debug(USERPROCESS, "first msg\n");
    auto target1 = ProcessRegistry::instance()->process_map_.find(pid);
    auto caller = ((UserThread*)currentThread)->getProcess();
    debug(USERPROCESS, "before if\n");
    if (caller->pid_ == (size_t)pid)
    {
        ProcessRegistry::instance()->process_lock_.release();
       debug(USERPROCESS, "im in if\n");
        return -1;
    }

    debug(USERPROCESS, "im after if\n");
    if (target1 == ProcessRegistry::instance()->process_map_.end())
    {
        debug(USERPROCESS, "No process, maybe already terminated or didn't exist\n");
        auto retval = process_retval_map_.find(pid);
        if (!retval)
        {
            *status = -1;
            ProcessRegistry::instance()->process_lock_.release();
            return -1;
        }
        else
        {
            *status = retval->second;
            process_retval_map_.erase(pid);
            ProcessRegistry::instance()->process_lock_.release();
            return pid;
        }
    }

    target = pid;

        // Wait for the process to terminate using the condition variable

            debug(USERPROCESS, "Heloooo");
            ProcessRegistry::instance()->process_lock_.release();

            process_wait_cond_.wait();
            debug(USERPROCESS, "hi");

            ProcessRegistry::instance()->process_lock_.acquire();




    auto retval = process_retval_map_.find(pid);
    if (!retval)
    {
        *status = -1;
        ProcessRegistry::instance()->process_lock_.release();
        return -1;
    }
    else
    {
        *status = retval->second;
        process_retval_map_.erase(pid);
        ProcessRegistry::instance()->process_lock_.release();
        return pid;
    }
}
