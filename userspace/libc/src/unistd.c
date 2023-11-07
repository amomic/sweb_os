#include "unistd.h"
#include "syscall.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int brk(void *end_data_segment)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
void* sbrk(intptr_t increment)
{
  return (void*) -1;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
unsigned int sleep(unsigned int seconds)
{
  return (unsigned int)__syscall(sc_sleep, (size_t)seconds, 0x0, 0x0, 0x0, 0x0);;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int ftruncate(int fildes, off_t length)
{
    return -1;
}
