
#include "../../include/kernel/SwapThread.h"
#include "Scheduler.h"
#include "BDManager.h"
#include "PageManager.h"
#include "Bitmap.h"
#include "UserThread.h"
#include "IPT.h"
#include "ArchThreads.h"
#include "UserProcess.h"
#include "ArchMemory.h"
#include "Thread.h"

SwapThread *SwapThread::instance_ = nullptr;

SwapThread::SwapThread() : Thread(nullptr, "SwapThread", Thread::KERNEL_THREAD), swap_lock_("SwapThread::swap_lock"),
                           swap_wait(&swap_lock_, "SwapThread::swap_wait"),
                            request_lock_("SwapThread::request_lock"),
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
        swap_lock_.acquire();

        if(swap_request_map_.empty() ) {
            request_lock_.release();
            swap_wait.wait();
            swap_lock_.release();
            continue;
        }

        auto request = swap_request_map_.back();

        if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_OUT)
        {
            swap_request_map_.pop_back();
            swap_lock_.release();
            request_lock_.release();

            SwapOut(request);
            request_lock_.acquire();

            request->is_done = true;
            request->request_cond_.signal();
            request_lock_.release();

        }

        else if(request->swap_type_ == Thread::SWAP_TYPE::SWAP_IN)
        {
            swap_request_map_.pop_back();
            swap_lock_.release();
            request_lock_.release();

            SwapIn(request);
            request_lock_.acquire();

            request->is_done = true;
            request->request_cond_.signal();
            request_lock_.release();

        }
        else
            request_lock_.release();


    }

}


size_t SwapThread::SwapOut(SwapRequest* request)
{
    IPT::instance()->ipt_lock_.acquire();
    debug(SWAP_THREAD, "before pra\n \n");

    size_t rand_ppn;
    if(active_pra_ == RANDOM_PRA) {
        rand_ppn = randomPRA();
    } else if(active_pra_ == SC_PRA) {
        rand_ppn = scPRA();
    } else if(active_pra_ == AGING_PRA) {
        rand_ppn = aging_PRA();
    }
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
    size_t prev_block = inv_entry->block;
    bool code_page_ = false;
    bool clean_page_ = false;
    if(inv_entry != nullptr )
    {

        debug(SWAP_THREAD, "Swap Out vpn : %zx. \n", inv_entry->virt_page_num_);
        size_t size = 0;
        for(auto memory : inv_entry->references_list_)
        {
            ArchMemoryMapping m = memory->resolveMapping(memory->page_map_level_4_,inv_entry->virt_page_num_);

            PageTableEntry* PT_entry = (PageTableEntry*)   memory->getIdentAddressOfPPN(m.pt_ppn);

            PT_entry[m.pti].present = 0;
            if(size == 0 && PT_entry[m.pti].dirty)
            {
                size = device_->writeData(block * PAGE_SIZE, PAGE_SIZE,reinterpret_cast<char*>(ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn)));
                assert(size);

            }

            PT_entry[m.pti].swapped = 1;
            if(inv_entry->dirty == 0)
            {
                inv_entry->dirty = PT_entry[m.pti].dirty;
            }
            if(!PT_entry[m.pti].dirty && inv_entry->dirty == 0)
            {
                PT_entry[m.pti].swapped = 0;
                code_page_ = true;
            }
            m.pt[m.pti].page_ppn = (uint64)block;

            if(!PT_entry[m.pti].dirty && inv_entry->dirty != 0 && prev_block != 0)
            {
                m.pt[m.pti].page_ppn = (uint64)prev_block;

                clean_page_ = true;
            }
            PT_entry[m.pti].dirty = 0;

        }

        debug(SWAP_THREAD, "ResolvedMapping ppn is:  %zu, our ppn is  %zu  \n", block,
              rand_ppn);

        if(!code_page_ && !clean_page_) {

            IPT::instance()->swapOutRef(rand_ppn,found);
        }
        else if(clean_page_) {
            disc_alloc_lock_.acquire();
            if(found < lowest_unreserved_page_){
                lowest_unreserved_page_ = found;
            }

            for(auto i = found; i < (found + 0); ++i){
                assert(bitmap_->getBit(i) && "Double Free!\n");
                bitmap_->unsetBit(i);
            }

            disc_alloc_lock_.release();
            found = prev_block;
            IPT::instance()->swapOutRef(rand_ppn,found);
            IPT::instance()->clean_swaps_++;
            //assert(false && "Works i guess\n");

        }
        else {
            IPT::instance()->ipt_.erase(rand_ppn);
            disc_alloc_lock_.acquire();
            if(found < lowest_unreserved_page_){
                lowest_unreserved_page_ = found;
            }

            for(auto i = found; i < (found + 0); ++i){
                assert(bitmap_->getBit(i) && "Double Free!\n");
                bitmap_->unsetBit(i);
            }

            disc_alloc_lock_.release();

            IPT::instance()->dirty_swaps_++;
            //assert(false && "It worked ay\n");
        }
        request->ppn_ = rand_ppn;
        debug(SWAP_THREAD, "Swaped out %zu \n", request->ppn_);
        if(active_pra_ == SC_PRA)
        {
            sc_references.push_back(ustl::pair(request->ppn_, true));
        }
        IPT::instance()->ipt_lock_.release();

        return request->ppn_;
    }
    else
    {

        // unreserve bit

        disc_alloc_lock_.acquire();
        if(block < lowest_unreserved_page_){
            lowest_unreserved_page_ = block;
        }

        for(auto i = block; i < (block + 0); ++i){
            assert(bitmap_->getBit(i) && "Double Free!\n");
            bitmap_->unsetBit(i);
        }

        disc_alloc_lock_.release();
        debug(SWAP_THREAD, "Swap Out failed. \n");
        IPT::instance()->ipt_lock_.release();

        return -1;
    }
}


[[maybe_unused]] bool SwapThread::SwapIn(SwapRequest *request)
{
    assert(!IPT::instance()->ipt_lock_.isHeldBy(currentThread));
    size_t new_page = PageManager::instance()->allocPPN();
    debug(SWAP_THREAD, "after alloc ppn in swapin\n");
    assert(!IPT::instance()->ipt_lock_.isHeldBy(currentThread));
    if(IPT::instance()->ipt_lock_.isHeldBy(currentThread))
        IPT::instance()->ipt_lock_.release();
    IPT::instance()->ipt_lock_.acquire();

    char* iAddress = reinterpret_cast<char *>(ArchMemory::getIdentAddressOfPPN(new_page));

    //debug(SWAP_THREAD, "after iaddress  ppn in swapin\n");

    size_t the_offset = request->block_number_;

    auto swap_entry = IPT::instance()->sipt_.find(the_offset);
   // debug(SWAP_THREAD, "after swap entry\n");
    if(swap_entry == IPT::instance()->sipt_.end()){
        debug(SWAP_THREAD, "Page was already swapped!\n");
        auto m = request->user_process->getLoader()->arch_memory_.resolveMapping(request->vpn_);
        auto ret_val = true;
        if (m.pt[m.pti].swapped)
            {
                ret_val = false;
                debug(SWAP_THREAD, "STill swapped out...\n");
                PageManager::instance()->freePPN(new_page);
            }
        IPT::instance()->ipt_lock_.release();
        return ret_val;
    } else {
        auto device_ret = device_->readData(the_offset * PAGE_SIZE, PAGE_SIZE, iAddress);
        if(device_ret)
            debug(SWAP_THREAD, "Hmm... device\n");

        //bool is_mapped = request->user_process->getLoader()->arch_memory_.mapPage(request->vpn_, new_page, 1);
        // Lock this S
       // debug(SWAP_THREAD, "before for loop\n");
        for(auto memory : swap_entry->second->references_list_) {
            ArchMemoryMapping m = memory->resolveMapping(memory->page_map_level_4_,request->vpn_);

            PageTableEntry* PT_entry = (PageTableEntry*)   memory->getIdentAddressOfPPN(m.pt_ppn);
            PT_entry[m.pti].swapped = 0;
            PT_entry[m.pti].page_ppn = new_page;
            PT_entry[m.pti].present = 1;
        }
       // debug(SWAP_THREAD, "after for loop\n");



        IPT::instance()->ipt_[new_page] = swap_entry->second;
        IPT::instance()->sipt_.erase(the_offset);

        //debug(SWAP_THREAD, "after erase\n");

        disc_alloc_lock_.acquire();
        if(the_offset < lowest_unreserved_page_){
            lowest_unreserved_page_ = the_offset;
        }

        for(auto i = the_offset; i < (the_offset + 0); ++i){
            assert(bitmap_->getBit(i) && "Double Free!\n");
            bitmap_->unsetBit(i);
        }

        disc_alloc_lock_.release();

        IPT::instance()->ipt_lock_.release();
    }

    return true;
}


size_t SwapThread::addCond([[maybe_unused]] size_t found) {


    if(currentThread == this)
    {
        auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_OUT, 0, 0, 0, 0, &SwapThread::instance()->swap_lock_);
        auto ppn = SwapOut(request);
        delete request;
        return ppn;
    }

   // debug(SYSCALL, "in add cond before request\n");
    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_OUT, 0, 0, 0, 0, &SwapThread::instance()->swap_lock_);
    //debug(SWAPPING, "Out of memory, swap out request added\n");
   // debug(SYSCALL, "in add cond after request\n");
    //condition
    //debug(SYSCALL, "ughhhhhhhhhhhhhhhhhhn");
    //debug(SYSCALL, "in add cond before aquire\n");
    request_lock_.acquire();

    swap_lock_.acquire();
 //   debug(SYSCALL, "in add cond after aquire\n");
    swap_request_map_.push_back(request);
    //currentThread->loader_->arch_memory_.releasearchmemLocks();

   // debug(SYSCALL, "in add cond after push\n");
    swap_wait.signal();
    swap_lock_.release();
    if(request->is_done == false)
        request->request_cond_.wait();
    request_lock_.release();
  //  debug(SWAP_THREAD, "cond added going to swap out");
    //SwapOut(request);


    found = request->ppn_;
    delete request;
    return found;
}

size_t SwapThread::WaitForSwapIn(size_t vpn, ArchMemoryMapping& m){
    auto request = new SwapRequest(Thread::SWAP_TYPE::SWAP_IN, 0, vpn, m.pt[m.pti].page_ppn, ((UserThread*)currentThread)->getProcess() ,&SwapThread::instance()->swap_lock_);


    if(currentThread->loader_->heap_mutex_.isHeldBy(currentThread))
        currentThread->loader_->heap_mutex_.release();
    request_lock_.acquire();

    swap_lock_.acquire();
    swap_request_map_.push_back(request);
    swap_wait.signal();
    swap_lock_.release();
    if(!request->is_done)
        request->request_cond_.wait();
    //m.pt[m.pti].page_ppn = request->ppn_;

    request_lock_.release();

    delete request;
    return 0;
}

bool SwapThread::schedulable() {

  return (getState() == Running);
}

size_t SwapThread::randomPRA() {

    debug(SWAP_THREAD, "randompra\n \n");
    size_t ppn_to_evict_found = 0;
    size_t ppn_to_evict = 0;
    size_t total_number_of_pages = PageManager::instance()->getTotalNumPages();

    while (!ppn_to_evict_found) {
        //debug(SWAP_THREAD, "random in while loop\n \n");
        ppn_to_evict = ((ArchThreads::rdtsc() >> 1) % total_number_of_pages / 2) + total_number_of_pages / 2;

        auto entry = IPT::instance()->ipt_.find(ppn_to_evict);
        if (entry == IPT::instance()->ipt_.end() || entry->second->type_ != PAGE) {
         //   debug(SWAP_THREAD, "\n random in if  \n");
            continue;
        } else {
            ppn_to_evict_found = 1;
        }

    }

   // debug(SWAP_THREAD, "return in random");
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

size_t SwapThread::scPRA() {
    bool found = false;
    size_t ppn;
    while(!found) {
        auto it = sc_references.begin();
        //debug(SWAP_THREAD , "Here 1\n");

        for(  ; it != sc_references.end(); it++)
        {
            ppn = it->first;
            bool referenced = it->second;
            //debug(SWAP_THREAD , "Here 2\n");

            auto entry = IPT::instance()->ipt_.find(ppn)->second;
            //debug(SWAP_THREAD , "Here 3\n");

            if(!referenced && entry->type_ == PAGE)
            {
                sc_references.erase(it);
                //sc_references.push_back(ustl::pair(ppn, true));
                found = true;
                break;
            }
            else {
                it->second = false;
            }
            //debug(SWAP_THREAD , "Here 4\n");
        }
    }
    debug(SWAP_THREAD, "Found a page\n");
    return ppn;
}

size_t SwapThread::aging_PRA() {
    size_t ppn = 0;
    size_t least_used = -1;
    for(auto it : aging_references) {
        auto entry = IPT::instance()->ipt_.find(ppn)->second;
        if(entry->type_ == PAGE && it.second < least_used)
        {
            least_used = it.second;
            ppn = it.first;
        }
    }
    return ppn;
}

void SwapThread::age() {
    for(auto it : aging_references) {
        it.second = it.second >> 1;
    }
}
