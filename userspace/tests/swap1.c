#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#define MB 1024ULL * 1024ULL

intptr_t global[7*MB];

int main()
{


    for(size_t i = 0; i < 7*MB; i+= 7)
    {
        if(!(i % 4096))
        {
            global[i] = (intptr_t)&global[i];
        }
    }

    fork();

    for(size_t i = 0; i < 7*MB; i+=7)
    {
        if(!(i % 4096))
        {
            printf(" global id is: %ld, and we got %ld\n", global[i], (intptr_t)&global[i] );
            assert(global[i] == (intptr_t)&global[i] && "swapIn failed");
        }
    }

    printf("DONE");

    return 0;
}
