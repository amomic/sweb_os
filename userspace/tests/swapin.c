#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include <time.h>


#define ITERATIONS 2000


int main()
{
    int some_var[ITERATIONS * 4096*4];
    //fork();
    printf("\nGOING IN\n");
    //sleep(1);
    size_t j = 0;
    for (size_t i = 0; i < ITERATIONS; i++)
    {
        some_var[j] = i;
        //printf("Writing: index %ld value is %d\n", i, some_var[j]);
        j += 1024;
    }
    //sleep(1);
    printf("\nTaking a break.. ufff..\n");
   // sleep(2);
    j = 0;
    for (size_t i = 0; i < ITERATIONS; i++)
    {
        printf("Reading: index %ld value is %d\n", i, some_var[j]);
        assert(some_var[j] == i);
        j += 1024;
    }
    printf("\nGOING OUT\n");



    return 0;
}