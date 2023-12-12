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
    block_ = 1;
    lowest_unreserved_page_ = 0;
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
    swap_lock_.acquire();
    device_ = BDManager::getInstance()->getDeviceByNumber(3);
    device_->setBlockSize(PAGE_SIZE);
    bitmap_ = new Bitmap(device_->getNumBlocks() - pages_number_);
    swap_wait.wait();

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

        swap_wait.wait();
    }

}


size_t SwapThread::SwapOut(SwapRequest* request)
{
    size_t rand_ppn = randomPRA();
    uint32 p;
    uint32 found = 0;
    size_t number_of_blocks_ = device_->getNumBlocks();
    size_t number_of_free_blocks_ = number_of_blocks_;

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
        device_->writeData(disk_off, PAGE_SIZE,reinterpret_cast<char*>(ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn)));
        m.pt[m.pti].page_ppn = (uint64)disk_off;
        IPT::swapOutRef(request->ppn_,found);// Push back the pointer
        ipt_[rand_ppn] = nullptr;
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

bool SwapThread::checkDone([[maybe_unused]]SwapRequest* done)
{
    /*for(int i = 0; i < (int)swap_request_map_.size(); i++)
    {
        if(swap_request_map_.)
    }*/
    return true;
}

void SwapThread::addCond([[maybe_unused]] size_t found) {

    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_OUT, 0, 0, 0, &SwapThread::instance()->swap_lock_);
    //debug(SWAPPING, "Out of memory, swap out request added\n");

    //condition

    swap_lock_.acquire();
    swap_request_map_.push(request);
    swap_wait.signal();
    ((UserThread*)currentThread)->state_join_lock_.acquire();
    ((UserThread*)currentThread)->swap_condition_.wait();
    ((UserThread*)currentThread)->state_join_lock_.release();
    swap_lock_.release();

    found = request->ppn_;
    delete request;
}

size_t SwapThread::randomPRA()
{

    size_t ppn_to_evict_found = 0;
    size_t ppn_to_evict = 0;
    size_t total_number_of_pages = PageManager::instance()->getTotalNumPages();

    while(!ppn_to_evict_found)
    {
        ppn_to_evict = ((ArchThreads::rdtsc() >> 1) % total_number_of_pages / 2) + total_number_of_pages / 2;
        ppn_to_evict_found = 1;
    }

    return ppn_to_evict;
}