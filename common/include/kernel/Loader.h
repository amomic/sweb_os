#pragma once

#include "types.h"
#include "Mutex.h"
#include "ArchMemory.h"
#include "ElfFormat.h"
#include <uvector.h>
#include <ulist.h>

class Stabs2DebugInfo;

class Loader
{
  public:
    Loader(ssize_t fd);

    Loader(Loader &loader, ssize_t fd, UserProcess* child);

    ~Loader();

    /**
     * Loads the ehdr and phdrs from the executable and (optionally) loads debug info.
     * @return true if this was successful, false otherwise
     */
    bool loadExecutableAndInitProcess();

    /**
     * Loads one binary page by its virtual address:
     * Gets a free physical page, copies the contents of the binary to it, and then maps it.
     * @param virtual_address virtual address where to find the page to load
     */
    void loadPage(pointer virtual_address, ustl::map<size_t, bool> *pMap);

    Stabs2DebugInfo const* getDebugInfos() const;

    void* getEntryFunction() const;

    ArchMemory arch_memory_;
    Mutex heap_mutex_;
    ustl::vector<size_t> vpns_of_heap_;

    size_t start_break_;
    size_t current_break_;

private:

    /**
     *reads ELF-headers from the executable
     * @return true if this was successful, false otherwise
     */
    bool readHeaders();


    /**
     * clean up and sort the elf headers for faster access.
     * @return true in case the headers could be prepared
     */
    bool prepareHeaders();


    bool loadDebugInfoIfAvailable();


    bool readFromBinary (char* buffer, l_off_t position, size_t length);


    size_t fd_;
    Elf::Ehdr *hdr_;
    ustl::list<Elf::Phdr> phdrs_;
    Mutex program_binary_lock_;

    Stabs2DebugInfo *userspace_debug_info_;

    size_t getHeapStart();
};

