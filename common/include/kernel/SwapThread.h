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

enum PRA {RANDOM_PRA, SC_PRA, AGING_PRA};
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

    bool SwapIn(SwapRequest* request);
    ustl::vector <SwapRequest* > swap_request_map_;
    Mutex swap_lock_;
    Condition swap_wait;
    size_t lowest_unreserved_page_;

    size_t addCond(size_t found);
    static size_t randomPRA();
    size_t number_of_blocks_;

     bool schedulable() override;
    Mutex request_lock_;

    size_t WaitForSwapIn(size_t vpn, ArchMemoryMapping &m);
    Mutex disc_alloc_lock_;

    PRA active_pra_ = RANDOM_PRA;

    ustl::map<size_t, bool> sc_references;
    ustl::map<size_t, size_t> aging_references;

    void age();

    size_t getPagesNumber() const;

    void setPagesNumber(size_t pagesNumber);

    Bitmap *getBitmap() const;

    void setBitmap(Bitmap *bitmap);

    BDVirtualDevice *getDevice() const;

    void setDevice(BDVirtualDevice *device);

    size_t getBlock() const;

    void setBlock(size_t block);

    const ustl::vector<SwapRequest *> &getSwapRequestMap() const;

    void setSwapRequestMap(const ustl::vector<SwapRequest *> &swapRequestMap);

    const Mutex &getSwapLock() const;

    void setSwapLock(const Mutex &swapLock);

private:
    static SwapThread *instance_;
    bool reserveBlock(uint32 block, uint32 num);

    size_t scPRA();

    size_t aging_PRA();
};
struct SwapRequest {
    size_t swap_type_;
    size_t ppn_;
    size_t vpn_;
    size_t block_number_;
    UserProcess *user_process;
    bool is_done = false;
    bool duplicate_ = false;
    Condition request_cond_;
    [[maybe_unused]]void initDevice(){

        debug(SWAP_THREAD, "constructor of swap thread\n");
        SwapThread::instance()->pages_number_ = PageManager::instance()->getTotalNumPages();
        //debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->device_ = BDManager::getInstance()->getDeviceByNumber(3);
      //  debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->device_->setBlockSize(PAGE_SIZE);
      //  debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->bitmap_ = new Bitmap(SwapThread::instance()->device_->getNumBlocks() - SwapThread::instance()->pages_number_);
        SwapThread::instance()->block_ = 1;
        SwapThread::instance()->lowest_unreserved_page_ = 0;
        //debug(SWAP_THREAD, "ended the construction\n");

        SwapThread::instance()->number_of_blocks_ = SwapThread::instance()->device_->getNumBlocks();
       // debug(SWAP_THREAD, "ended the construction\n");
    }
    SwapRequest(size_t sw_type, size_t ppn, size_t vpn, size_t block_number, UserProcess* process ,[[maybe_unused]]Mutex* swap_lock_) :
            swap_type_(sw_type), ppn_(ppn), vpn_(vpn), block_number_(block_number), user_process(process),  request_cond_(&SwapThread::instance()->request_lock_, "Condition::request_cond_"){
        if(SwapThread::instance()->device_ == nullptr)
            initDevice();
    };

};

