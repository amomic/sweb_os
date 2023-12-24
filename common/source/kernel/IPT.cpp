
#include "../../include/kernel/IPT.h"
#include "types.h"
#include "Scheduler.h"
#include "Thread.h"
#include "debug.h"
#include "PageManager.h"
#include "IPTEntry.h"
IPT iptin_;
IPT *IPT::instance_ = nullptr;

IPT *IPT::instance() {
    if(unlikely(!instance_))
        new (&iptin_) IPT();
    return instance_;
}
IPT::IPT() : ipt_lock_("ipt_lock_") {

    for(size_t i = 0; i < 2; i++)
    {
        debug(A_MEMORY, "%zu IPT ENTRY initialisation\n", i);
        //ipt_[i] = new IPTEntry;
        //ipt_[i] = new IPTEntry(,vpn,type,1);

    }
    instance_ = this;
    debug(A_MEMORY, "IPT initialisation done \n");
}



[[maybe_unused]] void IPT::addReference(size_t ppn, ArchMemory *memory, size_t vpn, PageType type)
{
  //  assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");
    assert(ppn <= PageManager::instance()->getTotalNumPages());

    debug(A_MEMORY, "prije ifa\n");
    if(ipt_.find(ppn) == ipt_.end())
    {
        debug(A_MEMORY, "u ifu\n");
        //ipt_[ppn] = new IPTEntry;
        auto entr = new IPTEntry(memory,vpn,type,1);
        ipt_.insert(ustl::pair<size_t, IPTEntry*>(ppn,entr));
        debug(A_MEMORY, "poslije new iptentry\n");
    }
    debug(A_MEMORY, "poslije ifa\n");
    ipt_[ppn]->references_list_.push_back(memory);
    ipt_[ppn]->arch_mem_ = memory;
    ipt_[ppn]->virt_page_num_ = vpn;
    ipt_[ppn]->type_ = type;
    debug(A_MEMORY, "zavrsio\n");
}

[[maybe_unused]] void IPT::addSwappedRef(size_t block_number[[maybe_unused]], ArchMemory *memory[[maybe_unused]])
{
    assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    sipt_[block_number]->references_list_.push_back(memory);
}

void IPT::deleteReference(size_t ppn, ArchMemory *memory)
{
    assert(ipt_.find(ppn) != ipt_.end() && "IPT does not have that PPN!");
    assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    assert(!ipt_.at(ppn)->references_list_.empty() && "IPT empty!");

    if(ipt_.at(ppn)->references_list_.size() == 1)
    {
        //delete ipt_[ppn];
        ipt_.erase(ppn);
    }
    else
    {
        ipt_.at(ppn)->references_list_.remove(memory);
    }

}

[[maybe_unused]] void IPT::deleteSwappedRef(size_t block_number[[maybe_unused]], ArchMemory *memory[[maybe_unused]])
{
    assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    sipt_.at(block_number)->references_list_.remove(memory);

    if(sipt_.at(block_number)->references_list_.empty())
    {
        delete sipt_[block_number];
        sipt_.erase(block_number);
    }
}

[[maybe_unused]] size_t IPT::getRefCount(size_t page_nr)
{
    assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");
    auto ret_val = ipt_.at(page_nr)->references_list_.size();
    return ret_val;
}

[[maybe_unused]] size_t IPT::getSwappedIPTRefCount(size_t page_nr[[maybe_unused]]) {
    assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    if(sipt_.find(page_nr) == sipt_.end())
    {
        return 0;
    }
    auto ret_val = sipt_.at(page_nr)->references_list_.size();
    return ret_val;
}

[[maybe_unused]] size_t IPT::swapOutRef(size_t ppn[[maybe_unused]], size_t block_number[[maybe_unused]])
{
    //ScopeLock lock(ipt_lock_);
    debug(SWAP_THREAD, "PPN THATS SWAPPED IS %zu\n", ppn);
    sipt_[block_number] = ipt_.at(ppn);
    deleteReference(ppn, sipt_.at(block_number)->arch_mem_);
    return 0;
}

[[maybe_unused]] size_t IPT::swapInRef(size_t page_nr[[maybe_unused]], size_t block_number[[maybe_unused]])
{
    ipt_[page_nr] = sipt_.at(block_number);
    sipt_.erase(block_number);
    return 0;
}

IPTEntry* IPT::GetIPT(size_t ppn)
{
    for(auto i : ipt_)
    {
        if(i.first == ppn)
        {
            return i.second;
        }
        else
        {
            continue;
        }

    }

    return nullptr;
}

