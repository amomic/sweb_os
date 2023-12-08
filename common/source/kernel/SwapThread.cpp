#include "../../include/kernel/SwapThread.h"
#include "Scheduler.h"
#include "BDManager.h"
#include "PageManager.h"
#include "Bitmap.h"

SwapThread swap_thread;
SwapThread *SwapThread::instance_ = nullptr;

SwapThread::SwapThread() : Thread(nullptr, "SwapThread", Thread::KERNEL_THREAD)
{
    assert(!instance_);
    instance_ = this;
    debug(SWAP_THREAD, "constructor of swap thread\n");
    pages_number_ = PageManager::instance()->getTotalNumPages();
    block_ = 1;
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

    device_ = BDManager::getInstance()->getDeviceByNumber(3);
    device_->setBlockSize(PAGE_SIZE);
    bitmap_ = new Bitmap(device_->getNumBlocks() - pages_number_);

    //make swaprequest map and check for it always

   /* while(1)
    {
        //swap request check

    }*/

}

size_t SwapThread::SwapInRequest()
{

    return 1;
}

size_t SwapThread::SwapOutRequest()
{
    return 1;
}


void SwapThread::SwapOut(PageTableEntry* pt, size_t pti)
{
    size_t ppn = block_*device_->getBlockSize();
    size_t ret = device_->writeData(ppn, PAGE_SIZE,
                       reinterpret_cast<char *>(ArchMemory::getIdentAddressOfPPN(pt[pti].page_ppn)));
    assert(ret);
    PageManager::instance()->freePPN(pt[pti].page_ppn);
    pt[pti].page_ppn = ppn;
    pt[pti].swapped = 1;
    pt[pti].present = 0;
    block_++;

}


void SwapThread::SwapIn(PageTableEntry* pt, size_t pti)
{
    size_t ppn = PageManager::instance()->allocPPN();
    size_t ret = device_->readData(pt[pti].page_ppn, PAGE_SIZE,
                       reinterpret_cast<char *>(ArchMemory::getIdentAddressOfPPN(ppn)));
    assert(ret);
    PageManager::instance()->freePPN(pt[pti].page_ppn);
    pt[pti].page_ppn = ppn;
    pt[pti].swapped = 0;
    pt[pti].present = 1;
}