#include "stdlib.h"
#include "stdio.h"
#include "assert.h"



int some_var[1000 * 4096*4];

int main()
{
    //fork();
    printf("\nGOING IN\n");
    sleep(1);
    size_t j = 0;
    for (size_t i = 0; i < 1000; i++)
    {
        some_var[j] = i;
        printf("Writing: index %ld value is %d\n", i, some_var[j]);
        j += 1024;
    }
    sleep(1);
    printf("\nTaking a break.. ufff..\n");
    sleep(2);
    j = 0;
    for (size_t i = 0; i < 1000; i++)
    {
        printf("Reading: index %ld value is %d\n", i, some_var[j]);
        assert(some_var[j] == i);
        j += 1024;
    }
    printf("\nGOING OUT\n");
    return 0;
}