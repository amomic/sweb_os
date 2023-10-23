#include "stdio.h"
#include "assert.h"
#include "pthread.h"
#include "nonstd.h"

#define SIGEGV -1
#define SIGNUM 999999999999
#define INFOS nullptr

int signum1;
void* infos1;

void* handler(int signum, void* infos)
{
    signum = signum1;
    infos = infos1;

    printf("%d - %p\n", signum, infos);
    exit(-SIGEGV);
    return 0;
}

int main(){

    printf("\n\n Ag0 test case! \n\n");
    signal(handler);
    printf("\n\n After signal is called\n\n");

    //crash
    *(int*)0x13 = 0;

    return 0;
}