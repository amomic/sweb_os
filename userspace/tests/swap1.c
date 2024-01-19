#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#define MB 1024ULL * 1024ULL

intptr_t var[MB];

int main()
{

    fork();

    for(size_t i = 0; i < MB; i+= 7)
    {

        var[i] = (intptr_t)&var[i];
    }


    for(size_t i = 0; i < MB; i+=7)
    {

            printf("id is: %ld, and we got %ld\n", var[i], (intptr_t)&var[i] );
            assert(var[i] == (intptr_t)&var[i] && "swapIn failed");
    }

    printf("DONE");

    return 0;
}
