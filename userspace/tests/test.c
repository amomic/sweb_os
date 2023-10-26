#include "../../common/include/kernel/syscall-definitions.h"
#include "stdio.h"
#include "nonstd.h"
#include "sys/syscall.h"
#define SIGNUM 11
#include "pthread.h"
#include "stdlib.h"

void handler(int signum, size_t infos)
{
    printf("Hello, signum is: %d and addr is %zu", signum, infos);
    exit(-SIGSEGV);
}


int  main()
{

    signal(handler);
    *(int*)0x42 = 0;
    __syscall(sc_sc_signal, (size_t)handler,0x0,0x0,0x0,0x0);
    return 0;
}