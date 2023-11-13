#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "PageManager.h"
#include "kstring.h"
#include "ArchThreads.h"
#include "Thread.h"
#include "UserThread.h"
#include "Thread.h"

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
      {
          //again, will be needed for A2
          //set cow bit
          //parent_pml4[pml4i].cow = 1;
          //set writable bit
          //parent_pml4[pml4i].writeable = 0;

          //TODO add a cow reference!

          //arch_mem_lock.release();
          pdpt_ = PageManager::instance()->allocPPN();
          //arch_mem_lock.acquire();
          child_pml4[pml4i].page_ppn = pdpt_;
          debug(A_MEMORY, "[Fork] PML4 assigned to child. Starting PDPT!\n");

//----------------------------------------PDPT--------------------------------------------------------------------------

          PageDirPointerTableEntry *parent_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(parent_pml4[pml4i].page_ppn);
          PageDirPointerTableEntry *child_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(child_pml4[pml4i].page_ppn);

          memcpy(child_pdpt, parent_pdpt, PAGE_SIZE);
          child_pml4[pml4i].present = 1;

          debug(A_MEMORY, "[Fork] Memory copy for PDPT done!\n");

          for(uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
          {
              if(parent_pdpt[pdpti].pd.present)
              {
                  //again, will be needed for A2
                  //set cow bit
                  //parent_pdpt[pdpti].pd.cow = 1;
                  //set writable bit
                  //parent_pdpt[pdpti].pd.writeable = 0;

                  //TODO add a cow reference!
                  //TODO delete memcpy's!

                  //arch_mem_lock.release();
                  pd_ = PageManager::instance()->allocPPN();
                  //arch_mem_lock.acquire();
                  child_pdpt[pdpti].pd.page_ppn = pd_;

                  debug(A_MEMORY, "[Fork] PDPT assigned to child. Starting PD!\n");

//------------------------------------------PD--------------------------------------------------------------------------

                  PageDirEntry *parent_pd = (PageDirEntry*) getIdentAddressOfPPN(parent_pdpt[pdpti].pd.page_ppn);
                  PageDirEntry *child_pd = (PageDirEntry*) getIdentAddressOfPPN(child_pdpt[pdpti].pd.page_ppn);

                  memcpy(child_pd, parent_pd, PAGE_SIZE);
                  child_pdpt[pdpti].pd.present = 1;

                  debug(A_MEMORY, "[Fork] Memory copy for PD done!\n");

                  for(uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
                  {
                      if(parent_pd[pdi].pt.present)
                      {
                          //again, will be neded for A2
                          //set cow bit
                          //parent_pd[pdi].pt.cow = 1;
                          //set writable bit
                          //parent_pd[pdi].pt.writeable = 0;

                          //TODO add a cow reference!
                          //TODO delete memcpy's!

                          //arch_mem_lock.release();
                          pt_ = PageManager::instance()->allocPPN();
                          //arch_mem_lock.acquire();
                          child_pd[pdi].pt.page_ppn = pt_;
                          debug(A_MEMORY, "[Fork] PD assigned to child. Starting PT!\n");

//------------------------------------------PT--------------------------------------------------------------------------

                          PageTableEntry *parent_pt = (PageTableEntry*) getIdentAddressOfPPN(parent_pd[pdi].pt.page_ppn);
                          PageTableEntry *child_pt = (PageTableEntry*) getIdentAddressOfPPN(child_pd[pdi].pt.page_ppn);

                          memcpy(child_pt, parent_pt, PAGE_SIZE);
                          child_pd[pdi].pt.present = 1;


                          debug(A_MEMORY, "[Fork] Memory copy for PT done!\n");

                          for(uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
                          {
                              if(parent_pt[pti].present)
                              {
                                  //again, will be needed for A2
                                  //set cow bit
                                  parent_pt[pti].cow = 1;
                                  child_pt[pti].cow = 1;
                                  //set writable bit
                                  parent_pt[pti].writeable = 0;
                                  child_pt[pti].writeable = 0;

                                  //TODO add a cow reference, for the first time increase it by 2!

                                  //end_level_ = PageManager::instance()->allocPPN(PAGE_SIZE);
                                  //child_pt[pti].page_ppn = end_level_;

                                  //void*parent_ = (void*)getIdentAddressOfPPN(parent_pt[pti].page_ppn);
                                  //void* child_ = (void*) getIdentAddressOfPPN(child_pt[pti].page_ppn);
                                  //memcpy(child_,parent_, PAGE_SIZE);

                                  child_pt[pti].present = 1;
                                  parent_pt[pti].present = 1;

                                  PageManager::instance()->cow_ref_map.find(parent_pt[pti].page_ppn)->second++;

                                  //debug(A_MEMORY, "Reference count is: %zu\n", ref_count);

                                  debug(A_MEMORY, "[Fork] PT assigned to child.\n");
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
  PageManager::instance()->freePPN(m.page_ppn);
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
    insert<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti, physical_page, 0, 0, user_access, 1);

    PageManager::instance()->cow_ref_map.push_back(ustl::make_pair((uint32)m.page_ppn, 0));
    return true;
  }

  return false;
}

ArchMemory::~ArchMemory()
{
  assert(currentThread->kernel_registers_->cr3 != page_map_level_4_ * PAGE_SIZE && "thread deletes its own arch memory");

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
                  PageManager::instance()->cow_ref_map_lock.acquire();
                  PageManager::instance()->cow_ref_map.find(pt[pti].page_ppn)->second--;

                  if(PageManager::instance()->cow_ref_map.find(pt[pti].page_ppn)->second == 0)
                  {
                      PageManager::instance()->iterator_cow_ref_map = PageManager::instance()->cow_ref_map.find(pt[pti].page_ppn);
                      PageManager::instance()->cow_ref_map.erase(PageManager::instance()->iterator_cow_ref_map);

                      debug(A_MEMORY, "Ref count is 0, freeing the page!\n");
                      pt[pti].present = 0;
                      PageManager::instance()->freePPN(pt[pti].page_ppn);
                  }

                  PageManager::instance()->cow_ref_map_lock.release();

                  debug(A_MEMORY, "Ref count is not 0, reference deleted!\n");
                }
              }
              pd[pdi].pt.present = 0;
              PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);
            }
          }
          pdpt[pdpti].pd.present = 0;
          PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);
        }
      }
      pml4[pml4i].present = 0;
      PageManager::instance()->freePPN(pml4[pml4i].page_ppn);
    }
  }
  PageManager::instance()->freePPN(page_map_level_4_);

  arch_mem_lock.release();
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
    ArchMemoryMapping mapping = resolveMapping(page_map_level_4_, virt_address / PAGE_SIZE);

    if(mapping.page_ppn)
    {
        debug(A_MEMORY, "[COW] Page ppn present in mapping!\n");
    }

    if(!mapping.pt)
    {
        debug(A_MEMORY, "PT null - something went wrong!\n");
        return false;
    }
    debug(A_MEMORY, "[COW] In isCowSet function!\n");

    /*
//----------------------------------------PML4--------------------------------------------------------------------------
    PageMapLevel4Entry *pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);

    if(!pml4[mapping.pml4i].present)
    {
        debug(A_MEMORY, "[COW] pml4 not present, returning false!\n");
        return false;
    }
//----------------------------------------PDPT--------------------------------------------------------------------------
    PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);

    if(!pdpt[mapping.pml4i].pd.present)
    {
        debug(A_MEMORY, "[COW] pdpt not present, returning false!\n");
        return false;
    }
//------------------------------------------PD--------------------------------------------------------------------------
    PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);

    if(!pd[mapping.pdi].pt.present)
    {
        debug(A_MEMORY, "[COW] pd not present, returning false!\n");
        return false;
    }
//------------------------------------------PT--------------------------------------------------------------------------
    PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);

    if(!pt[mapping.pti].present)
    {
        debug(A_MEMORY, "[COW] pt not present, returning false!\n");
        return false;
    }
     */
//----------------------------------------Final check-------------------------------------------------------------------
    if (mapping.pt[mapping.pti].cow == 1)
    {

        debug(A_MEMORY, "[COW] pt cow flag set, returning true!\n");
        return true;
    }
    else
    {
        debug(A_MEMORY, "[COW] cow bit is: %d\n", mapping.pt[mapping.pti].cow);
        debug(A_MEMORY, "[COW] pt cow flag not set, returning false!\n");
        return false;
    }
}


void ArchMemory::cowPageCopy([[maybe_unused]]uint64 virt_addresss, [[maybe_unused]]ustl::map<size_t, bool> *alloc_pages)
{
    debug(A_MEMORY, "[COW] Copying pages for COW!\n");

    ArchMemoryMapping mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);

    //TODO add an iterator for the allocated pages?
    //auto iterator = alloc_pages->begin();

//----------------------------------------PML4--------------------------------------------------------------------------
    if(mapping.pml4[mapping.pml4i].cow)
    {
        debug(A_MEMORY, "[COW] PML4 marked as COW, for A1 - this should not happen!\n");

        /* Tutors input - we don't need cow for all levels for A1, only the last level is needed (the actual page)
         * this code will come in handy for A2 then :)
         *
        debug(A_MEMORY, "[COW] PDPT marked as cow and will be copied!\n");

        // need to check the reference count of this level
        // if it's the last cow reference set the two lines below
        //debug(A_MEMORY, "[COW] Last reference of PDPT -> will be writable from now on!\n");
        //mapping.pml4[mapping.pml4i].cow = 0;
        //mapping.pml4[mapping.pml4i].writeable = 1;

        // else copy the pdpt? somehow
        debug(A_MEMORY, "[COW] PDPT is not the last reference, writable stays 0, page is copied!\n");
        auto page_pdpt = (++iterator)->first;
        alloc_pages->at(iterator->first) = true;

        // copying the page
        size_t cow_pdpt = getIdentAddressOfPPN(mapping.pml4[mapping.pml4i].page_ppn);
        memcpy((void*) getIdentAddressOfPPN(page_pdpt), (void*)cow_pdpt, PAGE_SIZE);

        mapping.pml4[mapping.pml4i].page_ppn = page_pdpt;
        mapping.pml4[mapping.pml4i].writeable = 1;
        mapping.pml4[mapping.pml4i].cow = 0;

        mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
        */
    }
//----------------------------------------PDPT--------------------------------------------------------------------------
    if(mapping.pdpt[mapping.pdpti].pd.cow)
    {
        debug(A_MEMORY, "[COW] PDPT marked as COW, for A1 - this should not happen!\n");

        /* Tutors input - we don't need cow for all levels for A1, only the last level is needed (the actual page)
         * this code will come in handy for A2 then :)
         *
        debug(A_MEMORY, "[COW] PD marked as cow and will be copied!\n");

        //TODO need to check the reference count of this level
        //TODO if its the last cow reference set the two lines below
        //debug(A_MEMORY, "[COW] Last reference of PD -> will be writable from now on!\n");
        //mapping.pdpt[mapping.pdpti].pd.cow = 0;
        //mapping.pdpt[mapping.pdpti].pd.writeable = 1;

        //TODO else copy the pd?
        debug(A_MEMORY, "[COW] PD is not the last reference, writable stays 0, page is copied!\n");
        auto page_pd = (++iterator)->first;
        alloc_pages->at(iterator->first) = true;

        // copying the page
        size_t cow_pd = getIdentAddressOfPPN(mapping.pdpt[mapping.pdpti].pd.page_ppn);
        memcpy((void*) getIdentAddressOfPPN(page_pd), (void*)cow_pd, PAGE_SIZE);

        mapping.pdpt[mapping.pdpti].pd.page_ppn = page_pd;
        mapping.pdpt[mapping.pdpti].pd.writeable = 1;
        mapping.pdpt[mapping.pdpti].pd.cow = 0;

        mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
         */
    }
//------------------------------------------PD--------------------------------------------------------------------------
    if(mapping.pd[mapping.pdi].pt.cow)
    {
        debug(A_MEMORY, "[COW] PD marked as COW, for A1 - this should not happen!\n");

        /* Tutors input - we don't need cow for all levels for A1, only the last level is needed (the actual page)
         * this code will come in handy for A2 then :)
         *
        debug(A_MEMORY, "[COW] PT marked as cow and will be copied!\n");

        //TODO need to check the reference count of this level
        //TODO if its the last cow reference set the two lines below
        //debug(A_MEMORY, "[COW] Last reference of PT -> will be writable from now on!\n");
        //mapping.pd[mapping.pdi].pt.cow = 0;
        //mapping.pd[mapping.pdi].pt.writeable = 1;

        //TODO else copy the pt?
        debug(A_MEMORY, "[COW] PT is not the last reference, writable stays 0, page is copied!\n");
        auto page_pt = (++iterator)->first;
        alloc_pages->at(iterator->first) = true;

        // copying the page
        size_t cow_pt = getIdentAddressOfPPN(mapping.pd[mapping.pdi].pt.page_ppn);
        memcpy((void*) getIdentAddressOfPPN(page_pt), (void*)cow_pt, PAGE_SIZE);

        mapping.pd[mapping.pdi].pt.page_ppn = page_pt;
        mapping.pd[mapping.pdi].pt.writeable = 1;
        mapping.pd[mapping.pdi].pt.cow = 0;

        mapping = resolveMapping(page_map_level_4_, virt_addresss / PAGE_SIZE);
        */
    }

    size_t cow_page = getIdentAddressOfPPN(mapping.pt[mapping.pti].page_ppn);
//------------------------------------------PT--------------------------------------------------------------------------
    if(mapping.pt[mapping.pti].cow)
    {
        debug(A_MEMORY, "[COW] Page marked as cow and will be copied!\n");

        //TODO need to check the reference count of this level
        //TODO if its the last cow reference set the two lines below
        //debug(A_MEMORY, "[COW] Last reference of Page -> will be writable from now on!\n");
        //mapping.pt[mapping.pti].cow = 0;
        //mapping.pt[mapping.pti].writeable = 1;

        //TODO else delete the current reference to the current page, memcpy here?
        debug(A_MEMORY, "[COW] Page is not the last reference, writable stays 0, page is copied!\n");
        auto page_cow = alloc_pages->front().first;
        alloc_pages->front().second = true;

        // copying the page
        memcpy((void*) getIdentAddressOfPPN(page_cow), (void*) cow_page, PAGE_SIZE);

        mapping.pt[mapping.pti].page_ppn = page_cow;
        mapping.pt[mapping.pti].writeable = 1;
        mapping.pt[mapping.pti].cow = 0;
    }
}
