#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#define SIZE (1024ULL * 1024ULL)

intptr_t var[10 * SIZE];

int main()
{

    for(size_t i = 0; i < 10*SIZE;i+= 1024)
    {
        var[i] = (intptr_t)&var[i];
    }


    for(size_t i = 0; i < 10*SIZE; i+= 1024)
    {
        printf(" id is: %ld, and we got %ld\n", var[i], (intptr_t)&var[i] );
        assert(var[i] == (intptr_t)&var[i]);
    }

    printf("SUCCESS\n");

    return 0;
}
