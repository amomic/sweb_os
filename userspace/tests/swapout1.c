#include "pthread.h"
#include "stdio.h"
#define MB 1024ULL * 1024ULL

size_t global[5*MB];

int main()
{

    for(size_t i = 0; i < 5*MB; ++i)
    {
        if(!(i % 4096))
        {
            global[i] = i - 1;
        }
    }

    printf("Done\n");


    return 0;
}
