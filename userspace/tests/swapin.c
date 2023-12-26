#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#define SIZE ((500 * 1000)/sizeof(long))

intptr_t global[7*SIZE];

int main()
{

    for(size_t i = 0; i < 7*SIZE;++ i)
    {
         global[i] = (intptr_t)&global[i];
    }

    fork();

    for(size_t i = 0; i < 7*SIZE; ++i)
    {
        printf(" global id is: %ld, and we got %ld\n", global[i], (intptr_t)&global[i] );
        assert(global[i] == (intptr_t)&global[i]);
    }

    printf("SUCCESS\n");

    return 0;
}
