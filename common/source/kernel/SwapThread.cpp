#include "../../include/kernel/SwapThread.h"
#include "Scheduler.h"
#include "BDManager.h"

SwapThread swap_thread;
SwapThread *SwapThread::instance_ = nullptr;

SwapThread::SwapThread() : Thread(nullptr, "SwapThread", Thread::KERNEL_THREAD)
{
    assert(!instance_);
    instance_ = this;
    debug(SWAP_THREAD, "constructor of swap thread\n");
}

SwapThread::~SwapThread()
{
    assert(false && "SwapThread destructor");
}

SwapThread *SwapThread::instance() {
    if(!instance_)
        instance_ = new SwapThread();
    return instance_;
}

void SwapThread::Run() {

}
