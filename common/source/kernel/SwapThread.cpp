#include "../../include/kernel/SwapThread.h"
#include "Scheduler.h"
#include "BDManager.h"
#include "PageManager.h"
#include "Bitmap.h"
#include "UserThread.h"
#include "IPT.h"
#include "ArchThreads.h"

SwapThread *SwapThread::instance_ = nullptr;

SwapThread::SwapThread() : Thread(nullptr, "SwapThread", Thread::KERNEL_THREAD), swap_lock_("SwapThread::swap_lock"),
                            swap_wait(&swap_lock_, "SwapThread::swap_wait") {
    assert(!instance_);
    instance_ = this;
    debug(SWAP_THREAD, "constructor of swap thread\n");
    pages_number_ = PageManager::instance()->getTotalNumPages();
    device_ = BDManager::getInstance()->getDeviceByNumber(3);
    device_->setBlockSize(PAGE_SIZE);
    bitmap_ = new Bitmap(device_->getNumBlocks() - pages_number_);
    block_ = 1;
    lowest_unreserved_page_ = 0;
    number_of_blocks_ = device_->getNumBlocks();
}

SwapThread::~SwapThread()
{
    debug(SWAP_THREAD, "constructor of swap thread,s th went wrong\n");
    assert(false && "SwapThread destructor");
}

SwapThread *SwapThread::instance() {
    if(!instance_)
        instance_ = new SwapThread();
    return instance_;
}

void SwapThread::Run() {

    debug(SWAP_THREAD, "in run in swapping\n");

    //make swaprequest map and check for it always

    while(true)
    {

        auto request = swap_request_map_.front();
        if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_OUT)
        {
            SwapOut(request);
            request->is_done = true;
            ((UserThread*)currentThread)->state_join_lock_.acquire();
            ((UserThread*)currentThread)->swap_condition_.signal();
            ((UserThread*)currentThread)->state_join_lock_.release();

        }

        if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_IN)
        {
            //swapin
            request->is_done = true;
            swap_wait.signal();
        }
        else
        {break;}

    }

}


size_t SwapThread::SwapOut(SwapRequest* request)
{
    size_t rand_ppn = randomPRA();
    uint32 p = 0;
    debug(SWAP_THREAD, "after pra\n \n");
    uint32 found = 0;
    debug(SWAP_THREAD, "after block num\n \n");
    size_t number_of_free_blocks_ = number_of_blocks_;
    kprintf("%zu",number_of_blocks_);

    debug(SWAP_THREAD, "swap out1\n \n");
    for(p = lowest_unreserved_page_; !found && p < number_of_blocks_; ++p)
    {
        if(!bitmap_->getBit(p))
        {
            bitmap_->setBit(p);
            found = p;
        }
    }
    while(lowest_unreserved_page_ < number_of_blocks_ &&
          bitmap_->getBit(lowest_unreserved_page_))
    {
        lowest_unreserved_page_++;
    }

    debug(SWAP_THREAD, "swap out2\n \n");

    assert(found != 0 && "ERROR: OUT OF DISK MEMORY. \n");
    number_of_free_blocks_--; // One Disk Page Number allocated, reducing

    size_t disk_off =  found * device_->getBlockSize();
    request->ppn_ = rand_ppn;

    auto inv_entry = IPT::instance()->GetIPT(rand_ppn);
    if(inv_entry)
    {
        ArchMemoryMapping m = SwapThread::loader_->arch_memory_.resolveMapping(request->vpn_);
        m.pt[m.pti].present = 0;
        m.pt[m.pti].swapped = 1;
        debug(SWAP_THREAD, "ResolvedMapping -> %zu == %zu <- evicted page \n", (size_t)m.pt[m.pti].page_ppn, rand_ppn);
        assert(m.pt[m.pti].page_ppn == rand_ppn&& "This should be the ppn we are swapping. \n");
        size_t size = device_->writeData(disk_off, PAGE_SIZE,reinterpret_cast<char*>(ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn)));
        assert(size);
        m.pt[m.pti].page_ppn = (uint64)disk_off;
        IPT::instance()->swapOutRef(request->ppn_,found);// Push back the pointer
        IPT::instance()->ipt_[rand_ppn] = nullptr;
        debug(SWAP_THREAD, "Swaped out \n");
        return request->ppn_;// set it to null, it is swapped
    }
    else
    {
        debug(SWAP_THREAD, "Swap Out failed. \n");
        return -1;
    }
}


void SwapThread::SwapIn()
{

}


void SwapThread::addCond([[maybe_unused]] size_t found) {

    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_OUT, 0, 0, 0, &SwapThread::instance()->swap_lock_);
    //debug(SWAPPING, "Out of memory, swap out request added\n");

    //condition

    /*swap_lock_.acquire();
    swap_request_map_.push(request);
    swap_wait.signal();
    swap_lock_.release();
    ((UserThread*)currentThread)->state_join_lock_.acquire();
    ((UserThread*)currentThread)->swap_condition_.wait();
    ((UserThread*)currentThread)->state_join_lock_.release();*/
    debug(SWAP_THREAD, "going to swap out");
    SwapOut(request);


    found = request->ppn_;
    delete request;
}

size_t SwapThread::randomPRA()
{

    debug(SWAP_THREAD, "randompra\n \n");
    size_t ppn_to_evict_found = 0;
    size_t ppn_to_evict = 0;
    size_t total_number_of_pages = PageManager::instance()->getTotalNumPages();

    while(!ppn_to_evict_found)
    {
        debug(SWAP_THREAD, "random1\n \n");
        ppn_to_evict = ((ArchThreads::rdtsc() >> 1) % total_number_of_pages / 2) + total_number_of_pages / 2;
        debug(SWAP_THREAD, "poslije tsc\n \n");
        ppn_to_evict_found = 1;
    }

    debug(SWAP_THREAD, "return");
    return ppn_to_evict;
}