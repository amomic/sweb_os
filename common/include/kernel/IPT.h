
#pragma once

#include "ArchMemory.h"
#include "Mutex.h"
#include "Loader.h"
#include "types.h"
#include "debug.h"
#include "IPTEntry.h"

class IPTEntry;

class IPT {
private:
public:
    static IPT *instance_;

    IPT();
    static IPT *instance();

    Mutex ipt_lock_;

    [[maybe_unused]] void addReference(size_t ppn, ArchMemory *memory, size_t vpn, PageType type, size_t dirty = 0);

    [[maybe_unused]]  void addSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]]  void deleteReference(size_t ppn, ArchMemory *memory);

    [[maybe_unused]]  void deleteSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]]  size_t getRefCount(size_t page_nr);

    [[maybe_unused]]  size_t getSwappedIPTRefCount(size_t page_nr);

    [[maybe_unused]]  size_t swapOutRef(size_t ppn, size_t block_number);

    [[maybe_unused]]  size_t swapInRef(size_t page_nr, size_t block_number);

    IPTEntry *GetIPT(size_t ppn);

    ustl::map<size_t, IPTEntry*> ipt_;
    ustl::map<size_t, IPTEntry*> sipt_;
    size_t dirty_swaps_;
    size_t clean_swaps_;
};








