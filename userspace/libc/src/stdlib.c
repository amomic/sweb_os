#include "stdlib.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
void *malloc(size_t size)
{
  return 0;
}

void free(void *ptr)
{
}

int atexit(void (*function)(void))
{
  return -1;
}

void *calloc(size_t nmemb, size_t size)
{
  return 0;
}

void *realloc(void *ptr, size_t size)
{
  return 0;
}

extern size_t get_dirty()
{
    return __syscall(sc_dirty, 0x00, 0x00, 0x00, 0x00, 0x00);

}

extern size_t get_clean()
{
    return __syscall(sc_clean, 0x00, 0x00, 0x00, 0x00, 0x00);

}