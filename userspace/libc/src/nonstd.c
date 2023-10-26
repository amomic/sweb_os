#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"
#include "stdio.h"

int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

int signal(void *handler)
{

    return __syscall(sc_sc_signal, (size_t) handler,0x0,0x0,0x0,0x0);
}

extern int main();

void _start()
{
  exit(main());
}
