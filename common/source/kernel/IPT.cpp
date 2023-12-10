//
// Created by amar on 12/10/23.
//

#include "../../include/kernel/IPT.h"
#include "types.h"
#include "Scheduler.h"
#include "Thread.h"
#include "debug.h"
#include "PageManager.h"

IPT *IPT::instance_ = nullptr;

IPT *IPT::instance() {
    instance_ = new IPT();
    return instance_;
}

Mutex IPT::ipt_lock_("ipt_lock_");

ustl::map<size_t, IPTEntry*> ipt_;
ustl::map<size_t, IPTEntry*> sipt_;

[[maybe_unused]] void IPT::addReference(size_t ppn, ArchMemory *memory, size_t vpn, PageType type)
{
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");
    assert(ppn <= PageManager::instance()->getTotalNumPages());

    if(ipt_.find(ppn) == ipt_.end())
    {
        ipt_[ppn] = new IPTEntry;
    }

    ipt_[ppn]->references_list_.push_back(memory);
    ipt_[ppn]->virt_page_num_ = vpn;
    ipt_[ppn]->type_ = type;
}

[[maybe_unused]] void IPT::addSwappedRef(size_t block_number[[maybe_unused]], ArchMemory *memory[[maybe_unused]])
{
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    sipt_[block_number]->references_list_.push_back(memory);
}

void IPT::deleteReference(size_t ppn, ArchMemory *memory)
{
    assert(ipt_.find(ppn) != ipt_.end() && "IPT does not have that PPN!");
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");
    assert(!ipt_.at(ppn)->references_list_.empty() && "IPT empty!");

   ipt_.at(ppn)->references_list_.remove(memory);

}

[[maybe_unused]] void IPT::deleteSwappedRef(size_t block_number[[maybe_unused]], ArchMemory *memory[[maybe_unused]])
{
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    sipt_.at(block_number)->references_list_.remove(memory);

    if(sipt_.at(block_number)->references_list_.empty())
    {
        delete sipt_[block_number];
        sipt_.erase(block_number);
    }
}

[[maybe_unused]] size_t IPT::getRefCount(size_t page_nr)
{
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    auto ret_val = ipt_.at(page_nr)->references_list_.size();
    return ret_val;
}

[[maybe_unused]] size_t IPT::getSwappedIPTRefCount(size_t page_nr[[maybe_unused]]) {
    assert(ipt_lock_.isHeldBy(currentThread) && "IPT lock!");

    if(sipt_.find(page_nr) == sipt_.end())
    {
        return 0;
    }
    auto ret_val = sipt_.at(page_nr)->references_list_.size();
    return ret_val;
}

[[maybe_unused]] size_t IPT::swapOutRef(size_t ppn[[maybe_unused]], size_t block_number[[maybe_unused]])
{
    sipt_[block_number] = ipt_.at(ppn);
    ipt_.erase(ppn);
    return 0;
}

[[maybe_unused]] size_t IPT::swapInRef(size_t page_nr[[maybe_unused]], size_t block_number[[maybe_unused]])
{
    ipt_[page_nr] = sipt_.at(block_number);
    sipt_.erase(block_number);
    return 0;
}

