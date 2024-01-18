#include "stdlib.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "pthread.h"
#include "unistd.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"

struct block_node {
    size_t allocated_size;
    struct block_node *next_block;
    int is_allocated;
};

static struct block_node *base = NULL;
static pthread_spinlock_t allocation_lock;

void *malloc(size_t size) {
    if (size <= 0)
        return NULL;

    pthread_spin_lock(&allocation_lock);

    size_t alignment = 16;
    size = (size + sizeof(struct block_node) + (alignment - 1)) & ~(alignment - 1);

    if (!base) {
        base = (struct block_node *) sbrk(size + sizeof(struct block_node));
        if ((size_t) base == (size_t) NULL) {
            pthread_spin_unlock(&allocation_lock);
            return NULL;
        }

        base->allocated_size = size + sizeof(struct block_node);
        base->next_block = NULL;
        base->is_allocated = 1;

        struct block_node *user_space = base;
        pthread_spin_unlock(&allocation_lock);
        return (void *) (user_space + 1);
    } else {
        struct block_node *current = base;

        while (current->next_block &&
               !(current->allocated_size >= size + sizeof(struct block_node) && !current->is_allocated)) {
            current = current->next_block;
        }

        if (current && current->next_block) {
            if (current->allocated_size > size + sizeof(struct block_node)) {
                size_t current_size = size + sizeof(struct block_node);
                size_t new_size = current->allocated_size - current_size;

                void *new_block_addr = (void *) current + current_size;
                struct block_node *new_block = (struct block_node *) new_block_addr;

                new_block->next_block = current->next_block;
                current->next_block = new_block;

                new_block->allocated_size = new_size;
                current->allocated_size = current_size;

                current->is_allocated = 1;
                new_block->is_allocated = 0;

                pthread_spin_unlock(&allocation_lock);
                return (void *) (current + 1);
            } else if (current->allocated_size == size + sizeof(struct block_node)) {
                current->is_allocated = 1;
                pthread_spin_unlock(&allocation_lock);
                return (void *) (current + 1);
            }
        } else if (current->next_block == NULL) {
            sbrk(size + sizeof(struct block_node));
            void *new_block_start = (void *) current + current->allocated_size;
            struct block_node *new_block = (struct block_node *) new_block_start;

            current->next_block = new_block;
            new_block->allocated_size = size + sizeof(struct block_node);
            new_block->next_block = NULL;
            new_block->is_allocated = 1;

            pthread_spin_unlock(&allocation_lock);
            return (void *) (new_block + 1);
        }
    }

    pthread_spin_unlock(&allocation_lock);
    return NULL;
}


void free(void *ptr) {
    pthread_spin_lock(&allocation_lock);

    if (!ptr || (size_t)ptr < 4096) {
        pthread_spin_unlock(&allocation_lock);
        return;
    }

    struct block_node *block_to_free = (struct block_node *)ptr - 1;

    assert(block_to_free->is_allocated && "Block is already free");

    if (!block_to_free->is_allocated) {
        printf("Double free detected\n");
        pthread_spin_unlock(&allocation_lock);
        exit(EXIT_FAILURE);
    }

    struct block_node *prev_block = base;
    while (prev_block && prev_block->next_block != block_to_free) {
        prev_block = prev_block->next_block;
    }

    if (!block_to_free->next_block) {
        if (prev_block) {
            prev_block->next_block = NULL;
        }
        brk(block_to_free);
    } else {
        block_to_free->is_allocated = 0;
    }

    pthread_spin_unlock(&allocation_lock);
}

int atexit(void (*function)(void))
{
  return -1;
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;

    if (size != 0 && total_size / size != nmemb) {
        // Overflow occurred
        return NULL;
    }

    void *ptr = malloc(total_size);
    if (!ptr) {
        // Allocation failed
        return NULL;
    }

    memset(ptr, 0, total_size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    pthread_spin_lock(&allocation_lock);

    struct block_node *current_block = (struct block_node *)ptr - 1;

    if (current_block->allocated_size >= size + sizeof(struct block_node)) {
        pthread_spin_unlock(&allocation_lock);
        return ptr;
    }

    pthread_spin_unlock(&allocation_lock);
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;  // malloc failed
    }

    size_t min_size = current_block->allocated_size - sizeof(struct block_node);
    memcpy(new_ptr, ptr, min_size);

    free(ptr);

    return new_ptr;
}

void checkInitAndLock(pthread_spinlock_t *spinlock){
    pthread_spin_init(spinlock, 0);
    pthread_spin_lock(spinlock);
}

extern size_t get_dirty()
{
    return __syscall(sc_dirty, 0x00, 0x00, 0x00, 0x00, 0x00);

}

extern size_t get_clean()
{
    return __syscall(sc_clean, 0x00, 0x00, 0x00, 0x00, 0x00);

}

extern size_t forceSwap(size_t ppn)
{
    return __syscall(sc_dbg_swout, ppn, 0x00, 0x00, 0x00, 0x00);
}