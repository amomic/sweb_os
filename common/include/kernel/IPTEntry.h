#pragma once

#include "types.h"
#include "Mutex.h"
#include "umap.h"
#include "umultimap.h"
#include "uvector.h"
#include "UserProcess.h"
#include "ArchMemory.h"
#include "debug.h"
enum PageType {PAGE[[maybe_unused]], PAGE_TABLE[[maybe_unused]], PAGE_DIR[[maybe_unused]], PDPT [[maybe_unused]]};

class ArchMemory;
class IPTEntry {


    public:
    ustl::list<ArchMemory*> references_list_;
    ArchMemory* arch_mem_;
    size_t virt_page_num_;
    PageType type_;
    uint64 swap_bit :1;
    uint64 dirty :1;
    size_t block = 0;
    IPTEntry(ArchMemory* mem, size_t vpn, PageType type, uint64 swap):
            references_list_(0),arch_mem_(mem), virt_page_num_(vpn), type_(type), swap_bit(swap){
        //debug(A_MEMORY, "u iptentry bla \n");
    };
};
