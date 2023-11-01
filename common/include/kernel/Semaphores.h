#pragma once
#include "types.h"
#include "ScopeLock.h"
#include "Lock.h"
#include "Mutex.h"
class Thread;

class Semaphore: public Lock
{
    friend class Scheduler;
    friend class Condition;

public:
    Semaphore(const char *name1, int initialValue);
    void wait();
    void signal();


private:

    size_t semaphore_;
    Mutex lock_;

};
