#pragma once
#include "Thread.h"
#include "ulist.h"
#include "UserProcess.h"
#include "Condition.h"

class UserThread: public Thread{
public:
        //constructor
        UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
        void *(*start_routine)(void *), void *wrapper, size_t tid, void*  argc, size_t args);
        //copy constructor
        //UserThread([[maybe_unused]]const UserThread &process_thread_pointer, UserProcess *parent_process,
        //  uint32 terminal_number, ustl::string filename, FileSystemInfo *fs_info, size_t thread_id);
        void Run();
        UserProcess *getProcess();
        UserProcess* process_;


private:
        ustl::string filename_;
        FileSystemInfo *fs_info_;
        uint32 terminal_number_;
        size_t tid_;

};