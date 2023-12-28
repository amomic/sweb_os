#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include <time.h>


#define ITERATIONS 2000
int some_var[ITERATIONS * 4096*4];

int main()
{
    printf("\nGOING IN\n");
    size_t j = 0;

    for (size_t i = 0; i < ITERATIONS; i++)
    {
        some_var[j] = i;
        printf("Writing: index %ld value is %d\n", i, some_var[j]);
        j += 1024;
    }

    j = 0;
    for (size_t i = 0; i < ITERATIONS; i++)
    {
        assert(j < ITERATIONS * 4096 * 4);
        printf("Reading: index %ld value is %d\n", i, some_var[j]);
        assert(some_var[j] == i);
        j += 1024;
    }

    printf("\nGOING OUT\n");

    return 0;
}