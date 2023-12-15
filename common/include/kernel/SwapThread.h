#pragma once
#include "ArchMemory.h"
#include "BDVirtualDevice.h"
#include "umap.h"
#include "types.h"
#include "mm/PageManager.h"
#include "Condition.h"
#include "debug.h"
#include "Thread.h"
#include "uqueue.h"
#include "Bitmap.h"
#include "BDManager.h"

struct SwapRequest;
class SwapThread : public Thread {
    friend class UserThread;

public:
    SwapThread();
    ~SwapThread();
    void Run() override;
    static SwapThread *instance();
    size_t pages_number_;
    Bitmap* bitmap_;
    BDVirtualDevice *device_;
    size_t block_;
    size_t SwapOut(SwapRequest* request);
    size_t SwapIn(SwapRequest* request);
    ustl::queue <SwapRequest*> swap_request_map_;
    Mutex swap_lock_;
    Condition swap_wait;
    size_t lowest_unreserved_page_;

    size_t addCond(size_t found);
    static size_t randomPRA();
    size_t number_of_blocks_;

     bool schedulable() override;
    Mutex request_lock_;
    Condition request_cond_;

    size_t WaitForSwapIn(size_t vpn);
    Mutex disc_alloc_lock_;

private:
    static SwapThread *instance_;


    bool reserveBlock(uint32 block, uint32 num);
};
struct SwapRequest{
    size_t swap_type_;
    size_t ppn_;
    size_t vpn_;
    size_t block_number_;
    UserProcess *user_process;
    bool is_done = false;

    void initDevice(){

        debug(SWAP_THREAD, "constructor of swap thread\n");
        SwapThread::instance()->pages_number_ = PageManager::instance()->getTotalNumPages();
        debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->device_ = BDManager::getInstance()->getDeviceByNumber(3);
        debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->device_->setBlockSize(PAGE_SIZE);
        debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->bitmap_ = new Bitmap(SwapThread::instance()->device_->getNumBlocks() - SwapThread::instance()->pages_number_);
        SwapThread::instance()->block_ = 1;
        SwapThread::instance()->lowest_unreserved_page_ = 0;
        debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->number_of_blocks_ = SwapThread::instance()->device_->getNumBlocks();
        debug(SWAP_THREAD, "ended the construction\n");
    }
    SwapRequest(size_t sw_type, size_t ppn, size_t vpn, size_t block_number, [[maybe_unused]]Mutex* swap_lock_) :
            swap_type_(sw_type), ppn_(ppn), vpn_(vpn), block_number_(block_number){
        if(SwapThread::instance()->device_ == nullptr)
            initDevice();
    };

};

