
#pragma once

#include "ArchMemory.h"
#include "Mutex.h"
#include "Loader.h"
#include "types.h"
#include "debug.h"

enum PageType {PAGE[[maybe_unused]], PAGE_TABLE[[maybe_unused]], PAGE_DIR[[maybe_unused]], PDPT [[maybe_unused]]};


typedef struct IPTEntry {
    ustl::list<ArchMemory*> references_list_;
    ArchMemory* arch_mem_;
    size_t virt_page_num_;
    PageType type_;
    uint64 swap_bit :1;

    IPTEntry(ArchMemory* mem, size_t vpn, PageType type, uint64 swap):
        references_list_(0),arch_mem_(mem), virt_page_num_(vpn), type_(type), swap_bit(swap){
        debug(A_MEMORY, "u iptentry bla \n");
    };
} IPTEntry;



class IPT {
private:
    static IPT *instance_;
    IPT();

public:
    static IPT *instance();

    Mutex ipt_lock_;

    [[maybe_unused]] void addReference(size_t ppn, ArchMemory *memory, size_t vpn, PageType type);

    [[maybe_unused]] static void addSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]] static void deleteReference(size_t ppn, ArchMemory *memory);

    [[maybe_unused]] static void deleteSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]] static size_t getRefCount(size_t page_nr);

    [[maybe_unused]] static size_t getSwappedIPTRefCount(size_t page_nr);

    [[maybe_unused]] static size_t swapOutRef(size_t ppn, size_t block_number);

    [[maybe_unused]] static size_t swapInRef(size_t page_nr, size_t block_number);

    IPTEntry *GetIPT(size_t ppn);
};

extern ustl::map<size_t, IPTEntry*> ipt_;
extern ustl::map<size_t, IPTEntry*> sipt_;






