//
// Created by amar on 1/18/24.
//
#include "pthread.h"
#include "stdio.h"
#include "sched.h"

#define MB 1024ULL * 1024ULL
#define SIZE 4096

size_t global[10*MB];

int main()
{
    int array[SIZE];

    //sleep(2);
    printf("Yielding!\n");
    sched_yield();

    for(int i = 0; i < SIZE; ++i)
    {
        if(i == SIZE/2)
        {
            array[i] = 5;
        }

        printf("%d", array[i]);
    }
    //sleep(2);

    for(size_t i = 0; i < 10*MB; ++i)
    {
        if(!(i % 4096))
        {
            global[i] = i - 1;
        }
    }

    printf("\nDone\n");



    return 0;
}