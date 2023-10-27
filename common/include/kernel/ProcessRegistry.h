#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"
#include "UserProcess.h"
#include <uset.h>
#include <ustl/umap.h>
#include <ulist.h>

class ProcessRegistry : public Thread
{
  public:
    /**
     * Constructor
     * @param root_fs_info the FileSystemInfo
     * @param progs a string-array of the userprograms which should be executed
     */
    ProcessRegistry ( FileSystemInfo *root_fs_info, char const *progs[] );
    ~ProcessRegistry();

    /**
     * Mounts the Minix-Partition with user-programs and creates processes
     */
    virtual void Run();

    /**
     * Tells us that a userprocess is being destroyed
     */
    void processExit();

    /**
     * Tells us that a userprocess is being created due to a fork or something similar
     */
    void processStart();

    /**
     * Tells us how many processes are running
     */
    size_t processCount();

    static ProcessRegistry* instance();
    void createProcess(const char* path);
    size_t fork();
    Mutex process_lock_;

    ustl::map<size_t, UserProcess*> process_map_;
    size_t process_count_ = 0;
private:

    char const **progs_;
    uint32 progs_running_;
    Mutex counter_lock_;
    Condition all_processes_killed_;
    static ProcessRegistry* instance_;

};

