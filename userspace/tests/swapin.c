#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#define WAITING_TIME 1024*1024*100
#define NUM_THREADS 100
#define MB 1024ULL * 1024ULL

intptr_t global[7*MB];

int main()
{

    for(size_t i = 0; i < 7*MB; ++i)
    {
        if(!(i % 4096))
        {
            global[i] = (intptr_t)&global[i];
        }
    }

    fork();

    for(size_t i = 0; i < 7*MB; ++i)
    {
        if(!(i % 4096))
        {
            assert(global[i] == (intptr_t)&global[i] && "swapIn failed");
        }
    }

    return 0;
}
