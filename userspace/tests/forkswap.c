#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
int some_var[2000 * 4096*4];

int main()
{
    printf("\nstarting\n");
    size_t j = 0;
    fork();

    for (size_t i = 0; i < 2000; i++)
    {
        some_var[j] = i;
        printf("write: index %ld value is %d\n", i, some_var[j]);
        j += 1024;
    }

    fork();
    j = 0;
    for (size_t i = 0; i < 2000; i++)
    {
        assert(j < 2000 * 4096 * 4);
        printf("read: index %ld value is %d\n", i, some_var[j]);
        assert(some_var[j] == i);
        j += 1024;
    }
    printf("Dirty Swaps: %lu \n", get_dirty());
    printf("Clean Swaps; %lu \n", get_clean());
    printf("\nsuccess\n");

    return 0;
}