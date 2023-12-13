#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "PageManager.h"
#include "kstring.h"
#include "ArchThreads.h"
#include "Thread.h"
#include "UserThread.h"
#include "Thread.h"
#include "ProcessRegistry.h"
#include "IPT.h"
#include "SwapThread.h"

PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

ArchMemory::ArchMemory() : arch_mem_lock("arch_mem_lock")
{
    page_map_level_4_ = PageManager::instance()->allocPPN();
    PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
    memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
    memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety
}

ArchMemory::ArchMemory(ArchMemory &parent) : arch_mem_lock("arch_mem_lock")
{

    debug(A_MEMORY, "[Fork] Copy constructor in Arch Memory called!\n");

    //lock with parent lock
    arch_mem_lock.acquire();

    page_map_level_4_ = PageManager::instance()->allocPPN();
    PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
    memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
    //memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety

//----------------------------------------PML4--------------------------------------------------------------------------

    PageMapLevel4Entry  *parent_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(parent.page_map_level_4_);
    PageMapLevel4Entry  *child_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(getPML4());

    memcpy(child_pml4, parent_pml4, PAGE_SIZE);

    debug(A_MEMORY, "[Fork] Memory copy for PML4 done!\n");

    //divide by 2 to free only the lower half
    for(uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++)
    {
        if(parent_pml4[pml4i].present)
        {/*
          parent_pml4[pml4i].cow = 1;
          parent_pml4[pml4i].writeable = 0;
          child_pml4[pml4i] = parent_pml4[pml4i];

          IPT::addReference(parent_pml4[pml4i].page_ppn, this, -1, PDPT);
*/
            pdpt_ = PageManager::instance()->allocPPN();//TODO DELETE THIS
            child_pml4[pml4i].page_ppn = pdpt_;//TODO DELETE THIS
            debug(A_MEMORY, "[Fork] PML4 assigned to child. Starting PDPT!\n");

//----------------------------------------PDPT--------------------------------------------------------------------------

            PageDirPointerTableEntry *parent_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(parent_pml4[pml4i].page_ppn);
            PageDirPointerTableEntry *child_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(child_pml4[pml4i].page_ppn);

            memcpy(child_pdpt, parent_pdpt, PAGE_SIZE); //TODO DELETE THIS
            child_pml4[pml4i].present = 1; //TODO DELETE THIS

            debug(A_MEMORY, "[Fork] Memory copy for PDPT done!\n");

            for(uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
            {
                if(parent_pdpt[pdpti].pd.present)
                {/*
                  assert(parent_pdpt[pdpti].pd.size == 0);
                  parent_pdpt[pdpti].pd.cow = 1;
                  parent_pdpt[pdpti].pd.writeable = 0;
                  child_pdpt[pdpti] = parent_pdpt[pdpti];

                  IPT::addReference(parent_pdpt[pdpti].pd.page_ppn, this, -2, PAGE_DIR);
*/
                    pd_ = PageManager::instance()->allocPPN(); //TODO DELETE THIS
                    child_pdpt[pdpti].pd.page_ppn = pd_;//TODO DELETE THIS

                    debug(A_MEMORY, "[Fork] PDPT assigned to child. Starting PD!\n");

//------------------------------------------PD--------------------------------------------------------------------------

                    PageDirEntry *parent_pd = (PageDirEntry*) getIdentAddressOfPPN(parent_pdpt[pdpti].pd.page_ppn);
                    PageDirEntry *child_pd = (PageDirEntry*) getIdentAddressOfPPN(child_pdpt[pdpti].pd.page_ppn);

                    memcpy(child_pd, parent_pd, PAGE_SIZE);//TODO DELETE THIS
                    child_pdpt[pdpti].pd.present = 1;//TODO DELETE THIS

                    debug(A_MEMORY, "[Fork] Memory copy for PD done!\n");

                    for(uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
                    {
                        if(parent_pd[pdi].pt.present)
                        {/*
                          assert(parent_pd[pdi].pt.size == 0);
                          parent_pd[pdi].pt.cow = 1;
                          parent_pd[pdi].pt.writeable = 0;
                          child_pd[pdi] = parent_pd[pdi];

                          IPT::addReference(parent_pd[pdi].pt.page_ppn, this, -3, PAGE_TABLE);
*/
                            pt_ = PageManager::instance()->allocPPN();//TODO DELETE THIS
                            child_pd[pdi].pt.page_ppn = pt_;//TODO DELETE THIS

                            debug(A_MEMORY, "[Fork] PD assigned to child. Starting PT!\n");

//------------------------------------------PT--------------------------------------------------------------------------

                            PageTableEntry *parent_pt = (PageTableEntry*) getIdentAddressOfPPN(parent_pd[pdi].pt.page_ppn);
                            PageTableEntry *child_pt = (PageTableEntry*) getIdentAddressOfPPN(child_pd[pdi].pt.page_ppn);

                            memcpy(child_pt, parent_pt, PAGE_SIZE);//TODO DELETE THIS
                            child_pd[pdi].pt.present = 1;//TODO DELETE THIS

                            debug(A_MEMORY, "[Fork] Memory copy for PT done!\n");

                            for(uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
                            {
                                if(parent_pt[pti].present)
                                {
                                    parent_pt[pti].cow = 1;
                                    parent_pt[pti].writeable = 0;
                                    //child_pt[pti] = parent_pt[pti];
                                    child_pt[pti].page_ppn = parent_pt[pti].page_ppn;

                                    //assert(!parent_pt[pti].swapped && "Present bit has to be 0 if marked as swapped!");

                                    debug(A_MEMORY, "Child PPN: %d\n", child_pt[pti].page_ppn);
                                    debug(A_MEMORY, "Parent PPN: %d\n", parent_pt[pti].page_ppn);

                                    IPT::instance()->addReference(parent_pt[pti].page_ppn, this,((((((pml4i << 9) | pdpti) << 9) | pdi) << 9) | pti), PAGE);

                                    debug(A_MEMORY, "[Fork] PT assigned to child.\n");
                                }
                                else if(parent_pt[pti].swapped)
                                {
                                    parent_pt[pti].cow = 1;
                                    parent_pt[pti].writeable = 0;

                                    child_pt[pti] = parent_pt[pti];

                                    IPT::instance()->addSwappedRef(parent_pt[pti].page_ppn, this);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    arch_mem_lock.release();

    debug(A_MEMORY, "[Fork] Arch Memory copy constructor finished!\n\n");
}


template<typename T>
bool ArchMemory::checkAndRemove(pointer map_ptr, uint64 index)
{
    T* map = (T*) map_ptr;
    debug(A_MEMORY, "%s: page %p index %zx\n", __PRETTY_FUNCTION__, map, index);
    ((uint64*) map)[index] = 0;
    for (uint64 i = 0; i < PAGE_DIR_ENTRIES; i++)
    {
        if (map[i].present != 0)
            return false;
    }
    return true;
}

bool ArchMemory::unmapPage(uint64 virtual_page)
{
    ArchMemoryMapping m = resolveMapping(virtual_page);

    assert(m.page_ppn != 0 && m.page_size == PAGE_SIZE && m.pt[m.pti].present);

    m.pt[m.pti].present = 0;
    m.pt[m.pti].cow = 0;

    IPT::instance()->deleteReference(m.pt[m.pti].page_ppn, this);

    if(IPT::instance()->getRefCount(m.pt[m.pti].page_ppn) == 0)
    {
        PageManager::instance()->freePPN(m.pt[m.pti].page_ppn);
    }

    //PageManager::instance()->freePPN(m.page_ppn);
    ((uint64*)m.pt)[m.pti] = 0; // for easier debugging


    bool empty = checkAndRemove<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti);
    if (empty)
    {
        empty = checkAndRemove<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi);
        PageManager::instance()->freePPN(m.pt_ppn);
    }
    if (empty)
    {
        empty = checkAndRemove<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti);
        PageManager::instance()->freePPN(m.pd_ppn);
    }
    if (empty)
    {
        checkAndRemove<PageMapLevel4Entry>(getIdentAddressOfPPN(m.pml4_ppn), m.pml4i);
        PageManager::instance()->freePPN(m.pdpt_ppn);
    }
    return true;
}

template<typename T>
void ArchMemory::insert(pointer map_ptr, uint64 index, uint64 ppn, uint64 bzero, uint64 size, uint64 user_access,
                        uint64 writeable)
{
    assert(map_ptr & ~0xFFFFF00000000000ULL);
    T* map = (T*) map_ptr;
    debug(A_MEMORY, "%s: page %p index %zx ppn %zx user_access %zx size %zx\n", __PRETTY_FUNCTION__, map, index, ppn,
          user_access, size);
    if (bzero)
    {
        memset((void*) getIdentAddressOfPPN(ppn), 0, PAGE_SIZE);
        assert(((uint64* )map)[index] == 0);
    }
    map[index].size = size;
    map[index].writeable = writeable;
    map[index].page_ppn = ppn;
    map[index].user_access = user_access;
    map[index].present = 1;
}

bool ArchMemory::mapPage(uint64 virtual_page, uint64 physical_page, uint64 user_access)
{
    debug(A_MEMORY, "%zx %zx %zx %zx\n", page_map_level_4_, virtual_page, physical_page, user_access);
    ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);
    assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

    //assert(arch_mem_lock.isHeldBy(currentThread));
    //assert(IPT::instance()->ipt_lock_.isHeldBy(currentThread));

    if(m.pt != nullptr && (m.pt[m.pti].present || m.pt[m.pti].swapped))
    {
        return false;
    }

    if (m.pdpt_ppn == 0)
    {
        m.pdpt_ppn = PageManager::instance()->allocPPN();
        insert<PageMapLevel4Entry>((pointer) m.pml4, m.pml4i, m.pdpt_ppn, 1, 0, 1, 1);
    }

    if (m.pd_ppn == 0)
    {
        m.pd_ppn = PageManager::instance()->allocPPN();
        insert<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti, m.pd_ppn, 1, 0, 1, 1);
    }

    if (m.pt_ppn == 0)
    {
        m.pt_ppn = PageManager::instance()->allocPPN();
        insert<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi, m.pt_ppn, 1, 0, 1, 1);
    }

    if (m.page_ppn == 0)
    {
        //m.page_ppn = physical_page;

        //IPT::addReference(physical_page, this, ((((((m.pml4i << 9) | m.pdpti) << 9) | m.pdi) << 9) | m.pti), PAGE);
        debug(A_MEMORY, "PRIJE\n");
        debug(A_MEMORY, "POSLIJE\n");
        //m = resolveMapping(page_map_level_4_, virtual_page);

        insert<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti, physical_page, 0, 0, user_access, 1);
//if(currentThread->getTID() != 0)
        IPT::instance()->addReference(physical_page, this, virtual_page, PAGE);
/*
    PageManager::instance()->cow_ref_map_lock.acquire();
    debug(A_MEMORY, "\n\n\n %zu \n\n\n", m.pt_ppn);
    PageManager::instance()->cow_ref_map.push_back(ustl::make_pair((uint32)physical_page, 1)); //TODO DELETE
    PageManager::instance()->cow_ref_map_lock.release();*/
        return true;
    }

    return false;
}

ArchMemory::~ArchMemory()
{
    assert(currentThread->kernel_registers_->cr3 != page_map_level_4_ * PAGE_SIZE && "thread deletes its own arch memory");

    IPT::instance()->ipt_lock_.acquire();
    arch_mem_lock.acquire();

    PageMapLevel4Entry* pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
    for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // free only lower half
    {
        if (pml4[pml4i].present)
        {
            PageDirPointerTableEntry* pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[pml4i].page_ppn);
            for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
            {
                if (pdpt[pdpti].pd.present)
                {
                    assert(pdpt[pdpti].pd.size == 0);
                    PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[pdpti].pd.page_ppn);
                    for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
                    {
                        if (pd[pdi].pt.present)
                        {
                            assert(pd[pdi].pt.size == 0);
                            PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
                            for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
                            {
                                if (pt[pti].present)
                                {
                                    IPT::instance()->deleteReference(pt[pti].page_ppn, this);

                                    if(IPT::instance()->getRefCount(pt[pti].page_ppn) == 0)
                                    {
                                        PageManager::instance()->freePPN(pt[pti].page_ppn);
                                    }
                                }
                            }
                            pd[pdi].pt.present = 0; //TODO DELETE THIS
                            PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);//TODO DELETE THIS

                            /*
                            IPT::deleteReference(pd[pdi].pt.page_ppn, this);

                            if(IPT::getRefCount(pd[pdi].pt.page_ppn) == 0)
                            {
                                PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);
                            }
                             */
                        }
                    }
                    pdpt[pdpti].pd.present = 0;//TODO DELETE THIS
                    PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);//TODO DELETE THIS

                    /*
                    IPT::deleteReference(pdpt[pdpti].pd.page_ppn, this);

                    if(IPT::getRefCount(pdpt[pdpti].pd.page_ppn) == 0)
                    {
                        PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);
                    }*/
                }
            }
            pml4[pml4i].present = 0;//TODO DELETE THIS
            PageManager::instance()->freePPN(pml4[pml4i].page_ppn);//TODO DELETE THIS

            /*
            IPT::deleteReference(pml4[pml4i].page_ppn, this);

            if(IPT::getRefCount(pml4[pml4i].page_ppn) == 0)
            {
                PageManager::instance()->freePPN(pml4[pml4i].page_ppn);
            }*/
        }
    }
    PageManager::instance()->freePPN(page_map_level_4_);

    arch_mem_lock.release();
    IPT::instance()->ipt_lock_.release();
}

pointer ArchMemory::checkAddressValid(uint64 vaddress_to_check)
{
    ArchMemoryMapping m = resolveMapping(page_map_level_4_, vaddress_to_check / PAGE_SIZE);
    if (m.page != 0)
    {
        debug(A_MEMORY, "checkAddressValid %zx and %zx -> true\n", page_map_level_4_, vaddress_to_check);
        return m.page | (vaddress_to_check % m.page_size);
    }
    else
    {
        debug(A_MEMORY, "checkAddressValid %zx and %zx -> false\n", page_map_level_4_, vaddress_to_check);
        return 0;
    }
}

const ArchMemoryMapping ArchMemory::resolveMapping(uint64 vpage)
{
    return resolveMapping(page_map_level_4_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(uint64 pml4, uint64 vpage)
{
    assert((vpage * PAGE_SIZE < USER_BREAK || vpage * PAGE_SIZE >= KERNEL_START) &&
           "This is not a valid vpn! Did you pass an address to resolveMapping?");
    ArchMemoryMapping m;

    m.pti = vpage;
    m.pdi = m.pti / PAGE_TABLE_ENTRIES;
    m.pdpti = m.pdi / PAGE_DIR_ENTRIES;
    m.pml4i = m.pdpti / PAGE_DIR_POINTER_TABLE_ENTRIES;

    m.pti %= PAGE_TABLE_ENTRIES;
    m.pdi %= PAGE_DIR_ENTRIES;
    m.pdpti %= PAGE_DIR_POINTER_TABLE_ENTRIES;
    m.pml4i %= PAGE_MAP_LEVEL_4_ENTRIES;

    assert(pml4 < PageManager::instance()->getTotalNumPages());
    m.pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(pml4);
    m.pdpt = 0;
    m.pd = 0;
    m.pt = 0;
    m.page = 0;
    m.pml4_ppn = pml4;
    m.pdpt_ppn = 0;
    m.pd_ppn = 0;
    m.pt_ppn = 0;
    m.page_ppn = 0;
    m.page_size = 0;
    if (m.pml4[m.pml4i].present)
    {
        m.pdpt_ppn = m.pml4[m.pml4i].page_ppn;
        m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pml4[m.pml4i].page_ppn);
        if (m.pdpt[m.pdpti].pd.present && !m.pdpt[m.pdpti].pd.size) // 1gb page ?
        {
            m.pd_ppn = m.pdpt[m.pdpti].pd.page_ppn;
            if (m.pd_ppn > PageManager::instance()->getTotalNumPages())
            {
                debug(A_MEMORY, "%zx\n", m.pd_ppn);
            }
            assert(m.pd_ppn < PageManager::instance()->getTotalNumPages());
            m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pdpt[m.pdpti].pd.page_ppn);
            if (m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size) // 2mb page ?
            {
                m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
                assert(m.pt_ppn < PageManager::instance()->getTotalNumPages());
                m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pd[m.pdi].pt.page_ppn);
                if (m.pt[m.pti].present)
                {
                    m.page = getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
                    m.page_ppn = m.pt[m.pti].page_ppn;
                    assert(m.page_ppn < PageManager::instance()->getTotalNumPages());
                    m.page_size = PAGE_SIZE;
                }
            }
            else if (m.pd[m.pdi].page.present)
            {
                m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES;
                m.page_ppn = m.pd[m.pdi].page.page_ppn;
                m.page = getIdentAddressOfPPN(m.pd[m.pdi].page.page_ppn);
            }
        }
        else if (m.pdpt[m.pdpti].page.present)
        {
            m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES * PAGE_DIR_ENTRIES;
            m.page_ppn = m.pdpt[m.pdpti].page.page_ppn;
            assert(m.page_ppn < PageManager::instance()->getTotalNumPages());
            m.page = getIdentAddressOfPPN(m.pdpt[m.pdpti].page.page_ppn);
        }
    }
    return m;
}

uint64 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(size_t virtual_page, size_t *physical_page,
                                                   size_t *physical_pte_page)
{
    ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                         virtual_page);
    if (physical_page)
        *physical_page = m.page_ppn;
    if (physical_pte_page)
        *physical_pte_page = m.pt_ppn;
    return m.page_size;
}

void ArchMemory::mapKernelPage(size_t virtual_page, size_t physical_page)
{
    ArchMemoryMapping mapping = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                               virtual_page);
    PageMapLevel4Entry* pml4 = kernel_page_map_level_4;
    assert(pml4[mapping.pml4i].present);
    PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);
    assert(pdpt[mapping.pdpti].pd.present);
    PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);
    assert(pd[mapping.pdi].pt.present);
    PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);
    assert(!pt[mapping.pti].present);
    pt[mapping.pti].writeable = 1;
    pt[mapping.pti].page_ppn = physical_page;
    pt[mapping.pti].present = 1;
    asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

void ArchMemory::unmapKernelPage(size_t virtual_page)
{
    ArchMemoryMapping mapping = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                               virtual_page);
    PageMapLevel4Entry* pml4 = kernel_page_map_level_4;
    assert(pml4[mapping.pml4i].present);
    PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);
    assert(pdpt[mapping.pdpti].pd.present);
    PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);
    assert(pd[mapping.pdi].pt.present);
    PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);
    assert(pt[mapping.pti].present);
    pt[mapping.pti].present = 0;
    pt[mapping.pti].writeable = 0;
    PageManager::instance()->freePPN(pt[mapping.pti].page_ppn);
    asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

PageMapLevel4Entry* ArchMemory::getRootOfKernelPagingStructure()
{
    return kernel_page_map_level_4;
}

uint64 ArchMemory::getPML4()
{
    return page_map_level_4_;
}

bool ArchMemory::isCowSet(uint64 virt_address)
{
    arch_mem_lock.acquire();
    ArchMemoryMapping mapping = resolveMapping(page_map_level_4_, virt_address / PAGE_SIZE);

    if(mapping.page_ppn)
    {
        debug(A_MEMORY, "[COW] Page ppn present in mapping!\n");
    }

    if(!mapping.pt)
    {
        debug(A_MEMORY, "PT null - something went wrong!\n");
        arch_mem_lock.release();
        return false;
    }
    debug(A_MEMORY, "[COW] In isCowSet function!\n");
/*
//----------------------------------------PML4--------------------------------------------------------------------------
    PageMapLevel4Entry *pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);

    if(!pml4[mapping.pml4i].present)
    {
        debug(A_MEMORY, "[COW] pml4 not present, returning false!\n");
        arch_mem_lock.release();
        return false;
    }
//----------------------------------------PDPT--------------------------------------------------------------------------
    PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);

    if(!pdpt[mapping.pdpti].pd.present)
    {
        debug(A_MEMORY, "[COW] pdpt not present, returning false!\n");
        arch_mem_lock.release();
        return false;
    }
//------------------------------------------PD--------------------------------------------------------------------------
    PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);

    if(!pd[mapping.pdi].pt.present)
    {
        debug(A_MEMORY, "[COW] pd not present, returning false!\n");
        arch_mem_lock.release();
        return false;
    }
//------------------------------------------PT--------------------------------------------------------------------------
    PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);

    if(!pt[mapping.pti].present)
    {
        debug(A_MEMORY, "[COW] pt not present, returning false!\n");
        arch_mem_lock.release();
        return false;
    }
*/
//----------------------------------------Final check-------------------------------------------------------------------
    if (mapping.pt[mapping.pti].cow == 1)
    {

        debug(A_MEMORY, "[COW] pt cow flag set, returning true!\n");
        arch_mem_lock.release();
        return true;
    }
    else
    {
        debug(A_MEMORY, "[COW] cow bit is: %d\n", mapping.pt[mapping.pti].cow);
        debug(A_MEMORY, "[COW] pt cow flag not set, returning false!\n");
        arch_mem_lock.release();
        return false;
    }
}


void ArchMemory::cowPageCopy([[maybe_unused]]uint64 virt_addresss, [[maybe_unused]]ustl::map<size_t, bool> *alloc_pages)
{
    debug(A_MEMORY, "[COW] Copying pages for COW!\n");

    arch_mem_lock.acquire();
    ArchMemoryMapping mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);

    //auto iterator = alloc_pages->begin();

//----------------------------------------PML4--------------------------------------------------------------------------
    if(mapping.pml4[mapping.pml4i].cow)
    {/*
        debug(A_MEMORY, "[COW] PDPT marked as COW!\n");

        //Last reference
        if(IPT::getRefCount(mapping.pml4[mapping.pml4i].page_ppn) == 1)
        {
            //Set relevant bits
            debug(A_MEMORY, "[COW] PDPT is last reference and will be writable from this point!\n");
            mapping.pml4[mapping.pml4i].cow = 0;
            mapping.pml4[mapping.pml4i].writeable = 1;
        }
        else //Not last reference
        {
            debug(A_MEMORY, "[COW] PDPT is NOT last reference and will be copied!\n");
            auto pdpt_page= (++iterator)->first;
            alloc_pages->at(iterator->first) = true;

            //Delete reference from IPT
            IPT::deleteReference(mapping.pml4[mapping.pml4i].page_ppn, this);

            //Get ppn for cow page
            size_t pdpt_cow = getIdentAddressOfPPN(mapping.pml4[mapping.pml4i].page_ppn);

            //Copy page
            memcpy((void*) getIdentAddressOfPPN(pdpt_page), (void*)pdpt_cow, PAGE_SIZE);

            //Set relevant bits and pages
            mapping.pml4[mapping.pml4i].cow = 0;
            mapping.pml4[mapping.pml4i].writeable = 1;
            mapping.pml4[mapping.pml4i].page_ppn = pdpt_page;

            //Add new reference
            IPT::addReference(mapping.pml4[mapping.pml4i].page_ppn, this, -1, PDPT);

            //Set new mapping
            mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
        }*/
    }
//----------------------------------------PDPT--------------------------------------------------------------------------
    if(mapping.pdpt[mapping.pdpti].pd.cow)
    {/*
        debug(A_MEMORY, "[COW] PD marked as COW!\n");

        //Last reference
        if(IPT::getRefCount(mapping.pdpt[mapping.pdpti].pd.page_ppn) == 1)
        {
            //Set relevant bits
            debug(A_MEMORY, "[COW] PD is last reference and will be writable from this point!\n");
            mapping.pdpt[mapping.pdpti].pd.cow = 0;
            mapping.pdpt[mapping.pdpti].pd.writeable = 1;
        }
        else //Not last reference
        {
            debug(A_MEMORY, "[COW] PD is NOT last reference and will be copied!\n");
            auto pd_page= (++iterator)->first;
            alloc_pages->at(iterator->first) = true;

            //Delete reference from IPT
            IPT::deleteReference(mapping.pdpt[mapping.pdpti].pd.page_ppn, this);

            //Get ppn for cow page
            size_t pd_cow = getIdentAddressOfPPN(mapping.pdpt[mapping.pdpti].pd.page_ppn);

            //Copy page
            memcpy((void*) getIdentAddressOfPPN(pd_page), (void*)pd_cow, PAGE_SIZE);

            //Set relevant bits and pages
            mapping.pdpt[mapping.pdpti].pd.cow = 0;
            mapping.pdpt[mapping.pdpti].pd.writeable = 1;
            mapping.pdpt[mapping.pdpti].pd.page_ppn = pd_page;

            //Add new reference
            IPT::addReference(mapping.pdpt[mapping.pdpti].pd.page_ppn, this, -2, PAGE_DIR);

            //Set new mapping
            mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
        }*/
    }
//------------------------------------------PD--------------------------------------------------------------------------
    if(mapping.pd[mapping.pdi].pt.cow)
    {/*
        debug(A_MEMORY, "[COW] PT marked as COW!\n");

        //Last reference
        if(IPT::getRefCount(mapping.pd[mapping.pdi].pt.page_ppn) == 1)
        {
            //Set relevant bits
            debug(A_MEMORY, "[COW] PT is last reference and will be writable from this point!\n");
            mapping.pd[mapping.pdi].pt.cow = 0;
            mapping.pd[mapping.pdi].pt.writeable = 1;
        }
        else //Not last reference
        {
            debug(A_MEMORY, "[COW] PT is NOT last reference and will be copied!\n");
            auto pt_page= (++iterator)->first;
            alloc_pages->at(iterator->first) = true;

            //Delete reference from IPT
            IPT::deleteReference(mapping.pd[mapping.pdi].pt.page_ppn, this);

            //Get ppn for cow page
            size_t pt_cow = getIdentAddressOfPPN(mapping.pd[mapping.pdi].pt.page_ppn);

            //Copy page
            memcpy((void*) getIdentAddressOfPPN(pt_page), (void*)pt_cow, PAGE_SIZE);

            //Set relevant bits and pages
            mapping.pd[mapping.pdi].pt.cow = 0;
            mapping.pd[mapping.pdi].pt.writeable = 1;
            mapping.pd[mapping.pdi].pt.page_ppn = pt_page;

            //Add new reference
            IPT::addReference(mapping.pd[mapping.pdi].pt.page_ppn, this, -3, PAGE_TABLE);

            //Set new mapping
            mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
        }*/
    }

    debug(A_MEMORY, "Child PPN is : %d\n", mapping.pt[mapping.pti].page_ppn);
    size_t cow_page = getIdentAddressOfPPN(mapping.pt[mapping.pti].page_ppn);

//------------------------------------------PT--------------------------------------------------------------------------
    if(mapping.pt[mapping.pti].cow)
    {
        debug(A_MEMORY, "[COW] Page marked as cow and will be copied!\n");

        debug(A_MEMORY, "[COW] Cow called in process with PID: %zu\n ",ProcessRegistry::instance()->process_count_);

        //Last reference
        if(IPT::instance()->getRefCount(mapping.pt[mapping.pti].page_ppn) == 1)
        {
            //Set relevant bits
            debug(A_MEMORY, "[COW] Page is last reference and will be writable from this point!\n");
            mapping.pt[mapping.pti].cow = 0;
            mapping.pt[mapping.pti].writeable = 1;
        }
        else //Not last reference
        {
            debug(A_MEMORY, "[COW] Page is NOT last reference and will be copied!\n");
            auto cow_copy_page= alloc_pages->front().first;
            alloc_pages->front().second = true;

            //Delete reference from IPT
            IPT::instance()->deleteReference(mapping.pt[mapping.pti].page_ppn, this);

            //Copy page
            memcpy((void*) getIdentAddressOfPPN(cow_copy_page), (void*)cow_page, PAGE_SIZE);

            //Set relevant bits and pages
            mapping.pt[mapping.pti].cow = 0;
            mapping.pt[mapping.pti].writeable = 1;
            mapping.pt[mapping.pti].page_ppn = cow_copy_page;

            //Add new reference
            IPT::instance()->addReference(mapping.pt[mapping.pti].page_ppn, this, ((((((mapping.pml4i << 9) | mapping.pdpti) << 9) | mapping.pdi) << 9) | mapping.pti), PAGE);
        }
    }
    arch_mem_lock.release();
}
