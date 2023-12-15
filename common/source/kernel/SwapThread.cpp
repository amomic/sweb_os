
#include "../../include/kernel/SwapThread.h"
#include "Scheduler.h"
#include "BDManager.h"
#include "PageManager.h"
#include "Bitmap.h"
#include "UserThread.h"
#include "IPT.h"
#include "ArchThreads.h"
#include "Thread.h"

SwapThread *SwapThread::instance_ = nullptr;

SwapThread::SwapThread() : Thread(nullptr, "SwapThread", Thread::KERNEL_THREAD), swap_lock_("SwapThread::swap_lock"),
                           swap_wait(&swap_lock_, "SwapThread::swap_wait"),
                            request_lock_("SwapThread::request_lock"),
                            request_cond_(&request_lock_, "SwapThread::request_cond"),
                            disc_alloc_lock_("SwapThread::disc_alloc_lock")
                             {

    assert(!instance_);
    instance_ = this;
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
        request_lock_.acquire();

        if(swap_request_map_.empty() ) {
            request_lock_.release();
            Scheduler::instance()->yield();
            continue;
        }
        auto request = swap_request_map_.front();
        if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_OUT)
        {
            swap_request_map_.pop();
            SwapOut(request);
            request->is_done = true;
            request_cond_.signal();
            request_lock_.release();

        }

        else if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_IN)
        {
            swap_request_map_.pop();
            SwapIn(request);
            request->is_done = true;
            swap_wait.signal();
            request_lock_.release();
        }
        else
            request_lock_.release();


    }

}


size_t SwapThread::SwapOut(SwapRequest* request)
{
    ScopeLock lock(IPT::instance()->ipt_lock_);
    size_t rand_ppn = randomPRA();
    uint32 p = 0;
    debug(SWAP_THREAD, "after pra\n \n");
    uint32 found = 0;
    debug(SWAP_THREAD, "after block num\n \n");
    [[maybe_unused]]size_t number_of_free_blocks_ = number_of_blocks_;
    //kprintf("%zu",number_of_blocks_);

    for (p = lowest_unreserved_page_; !found && (p < number_of_blocks_); ++p)
    {
        if (reserveBlock(p,1))
            found = p;
    }
    while ((lowest_unreserved_page_ < number_of_blocks_) && bitmap_->getBit(lowest_unreserved_page_))
        ++lowest_unreserved_page_;

    assert(found != 0 && "ERROR: OUT OF DISK MEMORY. \n");
    number_of_free_blocks_--; // One Disk Page Number allocated, reducing

    size_t block =  found;


    auto inv_entry = IPT::instance()->GetIPT(rand_ppn);
    if(inv_entry )
    {

        debug(SWAP_THREAD, "Swap Out vpn : %zx. \n", inv_entry->virt_page_num_);
        ArchMemoryMapping m = inv_entry->arch_mem_->resolveMapping(inv_entry->virt_page_num_);
        PageTableEntry* PT_entry = (PageTableEntry*)inv_entry->arch_mem_->getIdentAddressOfPPN(m.pt_ppn);
        if(inv_entry->virt_page_num_ == 0x800003f / PAGE_SIZE)
        {
            debug(SWAP_THREAD, "Swap Out vpn : %zx. \n", inv_entry->virt_page_num_);
            debug(SWAP_THREAD, "ResolvedMapping ppn is:  %zu, our ppn is  %zu  \n", (size_t)m.pt[m.pti].page_ppn,
                  rand_ppn)
            //assert(0 && "Why\n");
        }
        PT_entry[m.pti].present = 0;
        PT_entry[m.pti].swapped = 1;
        debug(SWAP_THREAD, "ResolvedMapping ppn is:  %zu, our ppn is  %zu  \n", (size_t)m.pt[m.pti].page_ppn,
              rand_ppn);
        size_t size = device_->writeData(block * PAGE_SIZE, PAGE_SIZE,reinterpret_cast<char*>(ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn)));
        assert(size);
        m.pt[m.pti].page_ppn = (uint64)block;
        IPT::instance()->swapOutRef(rand_ppn,found);
        request->ppn_ = rand_ppn;
        debug(SWAP_THREAD, "Swaped out %zu \n", request->ppn_);
        return request->ppn_;
    }
    else
    {
        debug(SWAP_THREAD, "Swap Out failed. \n");
        return -1;
    }
}


size_t SwapThread::SwapIn(SwapRequest *request)
{
    size_t new_page = PageManager::instance()->allocPPN();

    swap_lock_.acquire();

    char* iAddress = reinterpret_cast<char *>(ArchMemory::getIdentAddressOfPPN(new_page));

    ArchMemoryMapping am_map = request->user_process->getLoader()->arch_memory_.resolveMapping(request->vpn_);

    size_t the_offset = am_map.pt[am_map.pti].page_ppn;

    auto swap_entry = IPT::instance()->sipt_.find(the_offset);

    if(swap_entry == IPT::instance()->sipt_.end()){
        debug(SWAP_THREAD, "Page was already swapped!\n");
        swap_lock_.release();
        return 1;
    } else {
        auto device_ret = device_->readData(the_offset * PAGE_SIZE, PAGE_SIZE, iAddress);
        if(device_ret)
            debug(SWAP_THREAD, "Hmm... device\n");

        bool is_mapped = request->user_process->getLoader()->arch_memory_.mapPage(request->vpn_, new_page, 1);

        if(!is_mapped){
            assert(is_mapped && "Swap in error while mapping!\n");
            PageManager::instance()->freePPN(new_page);
        }

        am_map.pt[am_map.pti].swapped = 0;
        am_map.pt[am_map.pti].present = 1;
        am_map.pt[am_map.pti].page_ppn = new_page;

        IPT::instance()->ipt_[new_page] = swap_entry->second;
        IPT::instance()->sipt_.erase(the_offset);

        the_offset = the_offset / device_->getBlockSize();

        disc_alloc_lock_.acquire();
        if(the_offset < lowest_unreserved_page_){
            lowest_unreserved_page_ = the_offset;
        }

        for(auto i = the_offset; i < (the_offset + 1); ++i){
            assert(bitmap_->getBit(i) && "Double Free!\n");
            bitmap_->unsetBit(i);
        }
        disc_alloc_lock_.release();

        swap_lock_.release();

        return 0;
    }
}


size_t SwapThread::addCond([[maybe_unused]] size_t found) {

    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_OUT, 0, 0, 0, &SwapThread::instance()->swap_lock_);
    //debug(SWAPPING, "Out of memory, swap out request added\n");

    //condition
    SwapThread::loader_->arch_memory_.arch_mem_lock.release();
    swap_lock_.acquire();
    swap_request_map_.push(request);
    swap_wait.signal();
    swap_lock_.release();
    request_lock_.acquire();
    if(request->is_done == false)
        request_cond_.wait();
    request_lock_.release();
    debug(SWAP_THREAD, "cond added going to swap out");
    //SwapOut(request);


    found = request->ppn_;
    delete request;
    return found;
}

size_t SwapThread::WaitForSwapIn(size_t vpn){
    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_IN, 0, vpn, 0, &SwapThread::instance()->swap_lock_);

    swap_lock_.acquire();
    swap_request_map_.push(request);
    swap_wait.signal();
    swap_lock_.release();
    request_lock_.acquire();
    if(!request->is_done)
        request_cond_.wait();
    request_lock_.release();

    delete request;
    return 0;
}

bool SwapThread::schedulable() {

    if(swap_request_map_.empty())
    {
        return false;
    }
    else
    {
        return true;
    }
    //return (getState() == Running);
}

size_t SwapThread::randomPRA() {

    debug(SWAP_THREAD, "randompra\n \n");
    size_t ppn_to_evict_found = 0;
    size_t ppn_to_evict = 0;
    size_t total_number_of_pages = PageManager::instance()->getTotalNumPages();

    while (!ppn_to_evict_found) {
        debug(SWAP_THREAD, "random in while loop\n \n");
        ppn_to_evict = ((ArchThreads::rdtsc() >> 1) % total_number_of_pages / 2) + total_number_of_pages / 2;

        auto entry = IPT::instance()->ipt_.find(ppn_to_evict);
        if (entry == IPT::instance()->ipt_.end() || entry->second->virt_page_num_ < 0x9000f1f / PAGE_SIZE ||
            entry->second->virt_page_num_ > STACK_POS / PAGE_SIZE) {
            debug(SWAP_THREAD, "\n random in if  \n");
            continue;
        } else {
            ppn_to_evict_found = 1;
        }

    }

    debug(SWAP_THREAD, "return in random");
    return ppn_to_evict;
}

bool SwapThread::reserveBlock(uint32 block,uint32 num)
{
    assert(IPT::instance()->ipt_lock_.heldBy() == currentThread);
    if (block < number_of_blocks_ && !bitmap_->getBit(block))
    {
        if (num == 1 || reserveBlock(block+ 1, num - 1))
        {
            bitmap_->setBit(block);
            return true;
        }
    }
    return false;
}