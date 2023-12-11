//
// Created by amar on 12/10/23.
//
#pragma once

#include "ArchMemory.h"
#include "Mutex.h"
#include "Loader.h"
#include "types.h"

enum PageType {PAGE[[maybe_unused]], PAGE_TABLE[[maybe_unused]], PAGE_DIR[[maybe_unused]], PDPT [[maybe_unused]]};

struct IPTEntry {
    ustl::list<ArchMemory*> references_list_;
    size_t virt_page_num_;
    PageType type_;
    uint64 swap_bit :1;
};

class IPT {
private:
    static IPT *instance_;

public:
    static IPT *instance();

    static Mutex ipt_lock_;

    [[maybe_unused]] static void addReference(size_t ppn, ArchMemory *memory, size_t vpn, PageType type);

    [[maybe_unused]] static void addSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]] static void deleteReference(size_t ppn, ArchMemory *memory);

    [[maybe_unused]] static void deleteSwappedRef(size_t block_number, ArchMemory *memory);

    [[maybe_unused]] static size_t getRefCount(size_t page_nr);

    [[maybe_unused]] static size_t getSwappedIPTRefCount(size_t page_nr);

    [[maybe_unused]] static size_t swapOutRef(size_t ppn, size_t block_number);

    [[maybe_unused]] static size_t swapInRef(size_t page_nr, size_t block_number);
};

extern ustl::map<size_t, IPTEntry*> ipt_;
extern ustl::map<size_t, IPTEntry*> sipt_;






