#pragma once
#include "Thread.h"
#include "ulist.h"
#include "UserProcess.h"
#include "Condition.h"

class UserThread: public Thread{
public:
        enum DETATCH_STATE {JOINABLE = 7, DETATCHED = 8};

        //constructor
        UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
        void *(*start_routine)(void *), void *wrapper, size_t tid, void*  argc, size_t args);
        //copy constructor
        //UserThread([[maybe_unused]]const UserThread &process_thread_pointer, UserProcess *parent_process,
        //  uint32 terminal_number, ustl::string filename, FileSystemInfo *fs_info, size_t thread_id);
        ~UserThread();
        void Run();
        UserProcess *getProcess();
        UserProcess* process_;
        static const size_t STACK_PAGES = 15;
        size_t tid_;
        size_t offset_{};

        void setJoinTID(size_t tid);
        size_t getJoinTID();

        // Thread attributes
        DETATCH_STATE type_of_join_ = DETATCH_STATE::JOINABLE;

        // Joining conditions
        UserThread* waiting_for_ = nullptr;
        UserThread* waited_by_ = nullptr;

        Condition join_condition_;

private:
        ustl::string filename_;
        FileSystemInfo *fs_info_{};
        uint32 terminal_number_;
        size_t virtual_pages_{};
        size_t join_TID;
};