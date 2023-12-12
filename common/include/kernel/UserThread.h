#pragma once

#include "Thread.h"
#include "ulist.h"
#include "UserProcess.h"
#include "Condition.h"
#define STACK_SIZE 13
class UserThread : public Thread
{
public:
    enum DETATCH_STATE {
        JOINABLE = 7, DETATCHED = 8
    };

    //constructor
    UserThread(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number, UserProcess *userProcess,
               void *(*start_routine)(void *), void *wrapper, size_t tid, void* argc, size_t args);

    //copy constructor
    UserThread([[maybe_unused]]const UserThread &process_thread_pointer, UserProcess *parent_process,
               uint32 terminal_number, ustl::string filename, FileSystemInfo *fs_info, size_t thread_id);

    ~UserThread();

    void Run();

    UserProcess *getProcess();

    UserProcess *process_;
    static const size_t STACK_PAGES = 15;
    size_t tid_;
    size_t offset_{};
    void* wrapper_;
    // Custom Enums
    enum THREAD_CANCEL_STATE {
        ENABLED = 1, DISABLED = 0
    };
    enum THREAD_CANCEL_TYPE {
        ASYNCHRONOUS = 0, DEFERRED = 1
    };
    enum THREAD_CANCELATION {
        ISCANCELED = 1, NOTCANCELED = 0
    };
    void setProcess(UserProcess *process);

    size_t getTid() const;

    void setTid(size_t tid);

    size_t getOffset() const;

    void setOffset(size_t offset);

    void *getWrapper() const;

    void setWrapper(void *wrapper);

    THREAD_CANCEL_TYPE getThreadCancelType() const;

    void setThreadCancelType(THREAD_CANCEL_TYPE threadCancelType);

    THREAD_CANCEL_STATE getThreadCancelState() const;

    void setThreadCancelState(THREAD_CANCEL_STATE threadCancelState);

    THREAD_CANCELATION getThreadCancellationState() const;

    void setThreadCancellationState(THREAD_CANCELATION threadCancellationState);

    const Mutex &getStateJoinLock() const;

    void setStateJoinLock(const Mutex &stateJoinLock);

    DETATCH_STATE getTypeOfJoin() const;

    void setTypeOfJoin(DETATCH_STATE typeOfJoin);

    UserThread *getWaitingFor() const;

    void setWaitingFor(UserThread *waitingFor);

    UserThread *getWaitedBy() const;

    void setWaitedBy(UserThread *waitedBy);

    const Condition &getJoinCondition() const;

    void setJoinCondition(const Condition &joinCondition);

    const ustl::vector<size_t> &getVirtualPages() const;

    void setVirtualPages(const ustl::vector<size_t> &virtualPages);

    size_t getStackStart() const;

    void setStackStart(size_t stackStart);

    size_t getStackEnd() const;

    void setStackEnd(size_t stackEnd);

    const ustl::string &getFilename() const;

    void setFilename(const ustl::string &filename);

    FileSystemInfo *getFsInfo() const;

    void setFsInfo(FileSystemInfo *fsInfo);

    uint32 getTerminalNumber() const;

    void setTerminalNumber(uint32 terminalNumber);

    size_t getJoinTid() const;

    void setJoinTid(size_t joinTid);

    // Thread Cancle Attr
    THREAD_CANCEL_TYPE thread_cancel_type_ = THREAD_CANCEL_TYPE::DEFERRED;
    THREAD_CANCEL_STATE thread_cancel_state_ = THREAD_CANCEL_STATE::ENABLED;
    THREAD_CANCELATION thread_cancellation_state_ = THREAD_CANCELATION::NOTCANCELED;

    void setJoinTID(size_t tid);

    size_t getJoinTID();

    Mutex state_join_lock_;

    // Thread attributes
    DETATCH_STATE type_of_join_ = DETATCH_STATE::JOINABLE;

    // Joining conditions
    UserThread *waiting_for_ = nullptr;
    UserThread *waited_by_ = nullptr;

    Condition join_condition_;
    Condition swap_condition_;
    ustl::vector<size_t> virtual_pages_;

    void makeAsynchronousCancel();

    uint64 getStartClockTcs() const;

    void setStartClockTcs(uint64 startClockTime);
    size_t stack_start;
    size_t stack_end;


private:
    ustl::string filename_;
    FileSystemInfo *fs_info_{};
    uint32 terminal_number_;
    size_t join_TID;
    uint64 start_clock_tcs_;

};