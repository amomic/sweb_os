#include "stdio.h"
#include "time.h"

int main ()
{
    char array[1000] = {'0'};
    clock_t start = clock();
    printf("Start time: %u\n", start);

    clock_t time_before_loop = clock();
    printf("Time before loop: %u\n", time_before_loop);
    for(size_t count = 0; count < 1000; count++)
    {y
        array[count + 1] = '1';
        array[count] = array[count + 1];
    }
    clock_t after = clock();
    printf("Time after loop: %u\n", after);

    clock_t total = (double)(after - start) / CLOCKS_PER_SEC;
    printf("Total time taken by CPU: %u\n", total);
    return 0;
}