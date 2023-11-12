#pragma once

#include "Thread.h"
#include "UserThread.h"
#include "umap.h"
#include "types.h"
#include "Semaphores.h"
#include "Condition.h"

class UserThread;
class UserProcess
{
public:
    /**
     * Constructor
     * @param minixfs_filename filename of the file in minixfs to execute
     * @param fs_info filesysteminfo-object to be used
     * @param terminal_number the terminal to run in (default 0)
     *
     */
    Loader* loader_;
    UserProcess(ustl::string minixfs_filename, FileSystemInfo *fs_info, uint32 terminal_number = 0);
    UserProcess(UserProcess &parent_process, UserThread &current_thread, size_t process_id);
    virtual ~UserProcess();

    virtual void Run(); // not used
    UserThread *createThread(size_t *thread, size_t *attr, void *(*start_routine)(void *), void *wrapper, uint64 argc, size_t args);
    size_t joinThread(size_t thread, pointer return_val);
    size_t detachThread(size_t thread);
    size_t exec(char *path, char *const *argv);

    ustl::atomic<size_t> threads_counter_for_id_ = 0;
    UserProcess* process_;
    ustl::map<size_t, UserThread *> threads_map_;
    ustl::map<size_t, void*> thread_retval_map;

    Mutex threads_lock_;
    Mutex pages_lock_;
    Mutex return_val_lock_;
    Semaphore semaphore_init;
    Semaphore process_wait_cond_;

    size_t pid_;

    size_t virtual_pages_;
    ustl::atomic<size_t >threads_alive_;
    size_t target;
    size_t exit_code_{0};

    Loader* getLoader();

    void CleanThreads(size_t thread);
    void unmapPage();
    void deleteAllThreadsExceptCurrent(UserThread* current_thread);
    bool CheckStack(size_t pos);
    size_t checkExecArgs(char *const *args);

    pid_t waitpid(pid_t pid, int *status, int options);



private:
    int32 fd_;
    FileSystemInfo *working_dir_;
    ustl::string filename_;
    FileSystemInfo *fs_info_;
    uint32 terminal_number_;



};

