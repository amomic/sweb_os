#include "time.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
clock_t clock(void)
{
  return -1;//(clock_t) __syscall(sc_clock, 0x00, 0x00, 0x00, 0x00, 0x00);
}
