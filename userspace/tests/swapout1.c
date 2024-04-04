#include "pthread.h"
#include "stdio.h"
#define MB 1024ULL * 1024ULL

size_t var[10 * MB];

int main()
{

    for(size_t i = 0; i < 10*MB; ++i)
    {
        if(!(i % 4096))
        {
            var[i] = i - 1;
        }
    }

    printf("\nDone\n");


    return 0;
}
